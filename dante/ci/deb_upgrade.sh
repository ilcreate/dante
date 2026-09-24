#!/bin/sh
set -eu

phase=${1:-}
candidate=${2:-}
target=${3:-}
probe=${4:-}
state_dir=/var/lib/dante-ci
log_dir=/var/log/dante-ci

fail() {
    echo "FAIL: $*" >&2
    exit 1
}

guard_vm() {
    [ "${DANTE_CI_VM:-}" = 1 ] || fail "DANTE_CI_VM marker is missing"
    [ -f /etc/dante-ci-disposable ] || fail "disposable VM file marker is missing"
    [ "$(cat /proc/1/comm)" = systemd ] || fail "PID 1 is not systemd"
    virt=$(systemd-detect-virt --vm 2>/dev/null || true)
    if [ -z "$virt" ] || [ "$virt" = none ]; then
        fail "guest is not a virtual machine"
    fi
}

record() {
    printf '%s\n' "PASS: $*" | tee -a "$log_dir/results.log"
}

assert_equal() {
    [ "$1" = "$2" ] || fail "$3: expected '$1', got '$2'"
}

config_fingerprint() {
    sha256sum /etc/danted.conf | awk '{print $1}'
    stat -c '%U:%G:%a' /etc/danted.conf
}

pam_policy_fingerprint() {
    sha256sum /etc/pam.d/sockd | awk '{print $1}'
    stat -c '%U:%G:%a' /etc/pam.d/sockd
}

collect_failure_diagnostics() {
    status=$1
    trap - 0
    if [ "$status" -ne 0 ]; then
        {
            echo "phase=$phase status=$status"
            systemctl status danted.service --no-pager || true
            journalctl -u danted.service --no-pager -n 300 || true
            dpkg-query -W -f='${Status} ${Version} ${Architecture}\n' dante-server || true
            systemctl show danted.service -p ActiveState -p UnitFileState -p FragmentPath || true
            stat -c '%n %U:%G:%a' /etc/danted.conf /etc/pam.d/sockd \
                /etc/systemd/system/danted.service.d/ci-preserve.conf 2>/dev/null || true
            sha256sum /etc/danted.conf /etc/pam.d/sockd \
                /etc/systemd/system/danted.service.d/ci-preserve.conf 2>/dev/null || true
        } > "$log_dir/failure-${phase}.log" 2>&1
    fi
    exit "$status"
}

assert_preserved() {
    current=$(config_fingerprint)
    expected=$(cat "$state_dir/config.fingerprint")
    assert_equal "$expected" "$current" "danted config changed"
    current=$(sha256sum /etc/systemd/system/danted.service.d/ci-preserve.conf | awk '{print $1}')
    expected=$(cat "$state_dir/override.sha256")
    assert_equal "$expected" "$current" "systemd override changed"
    current=$(pam_policy_fingerprint)
    expected=$(cat "$state_dir/pam-policy.fingerprint")
    assert_equal "$expected" "$current" "PAM sockd policy changed"
    if dpkg-query -S /etc/pam.d/sockd >/dev/null 2>&1; then
        fail "dante-server unexpectedly owns the host PAM sockd policy"
    fi
    [ "$(cat /etc/sockd.conf)" = obsolete-sockd-must-not-be-used ] || fail "obsolete sockd.conf changed"
}

assert_layout() {
    [ -x /usr/sbin/danted ] || fail "/usr/sbin/danted is missing"
    [ ! -e /usr/sbin/sockd ] || fail "legacy /usr/sbin/sockd was installed"
    [ ! -e /lib/systemd/system/sockd.service ] || fail "legacy sockd.service was installed"
    [ ! -e /usr/lib/systemd/system/sockd.service ] || fail "legacy sockd.service was installed"
    dpkg-query -L dante-server | grep -qx /etc/danted.conf || fail "danted conffile is missing"
    dpkg-query -L dante-server | grep -qx /lib/systemd/system/danted.service || \
        dpkg-query -L dante-server | grep -qx /usr/lib/systemd/system/danted.service || \
        fail "danted.service is missing"
    unit=$(systemctl show -p FragmentPath --value danted.service)
    [ -f "$unit" ] || fail "systemd vendor unit cannot be resolved"
    if [ -f "$state_dir/distro-unit.sha256" ]; then
        current=$(sha256sum "$unit" | awk '{print $1}')
        expected=$(cat "$state_dir/distro-unit.sha256")
        assert_equal "$expected" "$current" "vendor danted.service differs from distro package"
    fi
    if [ -f "$state_dir/distro-conffile.metadata" ]; then
        dpkg-query -W -f='${Conffiles}\n' dante-server | \
            awk '$1 == "/etc/danted.conf" { print $1, $2 }' > "$state_dir/current-conffile.metadata"
        cmp -s "$state_dir/distro-conffile.metadata" "$state_dir/current-conffile.metadata" || \
            fail "danted.conf package metadata differs from distro package"
    fi
    build=$(/usr/sbin/danted -vv 2>&1)
    printf '%s\n' "$build" > "$log_dir/danted-vv.log"
    printf '%s\n' "$build" | grep -Eq '^build:.*\bpam\b' || fail "PAM is not compiled in"
    printf '%s\n' "$build" | grep -Eq '^build:.*\blibwrap\b' || fail "libwrap is not compiled in"
    record "binary-unit-config-layout pam libwrap"
}

install_candidate() {
    DEBIAN_FRONTEND=noninteractive apt-get \
        -o Dpkg::Options::=--force-confold \
        -o Dpkg::Options::=--force-confdef \
        install -y --allow-downgrades "$candidate"
}

write_test_config() {
    getent passwd proxy >/dev/null || fail "distribution proxy user is missing"
    cat > /etc/danted.conf <<'EOF'
logoutput: syslog
internal: 127.0.0.1 port = 1080
external: 127.0.0.1
clientmethod: none
socksmethod: pam.username
user.privileged: root
user.unprivileged: nobody
user.libwrap: nobody
client pass {
    from: 127.0.0.1/32 to: 127.0.0.0/8
}
socks pass {
    from: 127.0.0.1/32 to: 127.0.0.0/8
    command: connect udpassociate udpreply
    protocol: tcp udp
    socksmethod: pam.username
    pam.servicename: dante-ci-sockd
}
EOF
    chown root:proxy /etc/danted.conf
    chmod 0640 /etc/danted.conf
    printf '%s\n' obsolete-sockd-must-not-be-used > /etc/sockd.conf
    mkdir -p /etc/systemd/system/danted.service.d
    cat > /etc/systemd/system/danted.service.d/ci-preserve.conf <<'EOF'
[Service]
Environment=DANTE_CI_PRESERVED_OVERRIDE=1
EOF
    systemctl daemon-reload
    config_fingerprint > "$state_dir/config.fingerprint"
    sha256sum /etc/systemd/system/danted.service.d/ci-preserve.conf | awk '{print $1}' > "$state_dir/override.sha256"
}

setup_pam() {
    id dantepam >/dev/null 2>&1 || useradd --create-home --shell /bin/bash dantepam
    id dantedeny >/dev/null 2>&1 || useradd --create-home --shell /bin/bash dantedeny
    printf '%s\n' 'dantepam:Dante-CI-pass1!' 'dantedeny:Dante-CI-pass2!' | chpasswd
    usermod --expiredate -1 dantepam
    usermod --expiredate 1 dantedeny
    cat > /etc/pam.d/dante-ci-sockd <<'EOF'
@include common-auth
@include common-account
EOF
    cat > /etc/pam.d/sockd <<'EOF'
# Existing host policy sentinel: the package must not replace this file.
auth required pam_deny.so
account required pam_deny.so
EOF
    chown root:root /etc/pam.d/sockd
    chmod 0644 /etc/pam.d/sockd
    pam_policy_fingerprint > "$state_dir/pam-policy.fingerprint"
}

start_echo_services() {
    systemctl stop dante-ci-tcp.service dante-ci-udp.service 2>/dev/null || true
    systemd-run --unit=dante-ci-tcp --collect --property=Restart=on-failure \
        /usr/bin/python3 "$probe" echo-tcp 18080 >/dev/null
    systemd-run --unit=dante-ci-udp --collect --property=Restart=on-failure \
        /usr/bin/python3 "$probe" echo-udp 18081 >/dev/null
    sleep 1
}

run_proxy_probes() {
    start_echo_services
    /usr/bin/python3 "$probe" tcp --username dantepam --password 'Dante-CI-pass1!' --target-port 18080
    record "pam-success tcp-relay"
    set +e
    /usr/bin/python3 "$probe" tcp --username dantepam --password wrong --target-port 18080
    rc=$?
    set -e
    [ "$rc" -eq 2 ] || fail "pam-wrong-password returned $rc instead of authentication rejection"
    record "pam-wrong-password"
    set +e
    /usr/bin/python3 "$probe" tcp --username dantedeny --password 'Dante-CI-pass2!' --target-port 18080
    rc=$?
    set -e
    [ "$rc" -eq 2 ] || fail "pam-account-denied returned $rc instead of authentication rejection"
    record "pam-account-denied"
    /usr/bin/python3 "$probe" udp --username dantepam --password 'Dante-CI-pass1!' --target-port 18081
    record "udp-relay"
}

assert_unit_state() {
    expected=$1
    case "$expected" in
        enabled) systemctl is-enabled --quiet danted.service ;;
        disabled) [ "$(systemctl is-enabled danted.service 2>/dev/null || true)" = disabled ] ;;
        masked) [ "$(systemctl is-enabled danted.service 2>/dev/null || true)" = masked ] ;;
        *) fail "unknown unit state $expected" ;;
    esac || fail "$expected unit state was not preserved"
    record "$expected unit-state"
}

assert_unit_activity() {
    expected=$1
    case "$expected" in active|inactive) ;; *) fail "unknown activity state $expected" ;; esac
    current=$(systemctl is-active danted.service 2>/dev/null || true)
    assert_equal "$expected" "$current" "danted.service activity differs"
    record "$expected unit-activity"
}

assert_rollback_identity() {
    current=$(dpkg-query -W -f='${Version}' dante-server)
    expected=$(cat "$state_dir/distro-version")
    assert_equal "$expected" "$current" "rollback package version differs"
    current=$(sha256sum /usr/sbin/danted | awk '{print $1}')
    expected=$(cat "$state_dir/distro-binary.sha256")
    assert_equal "$expected" "$current" "rollback danted binary differs"
    unit=$(systemctl show -p FragmentPath --value danted.service)
    current=$(sha256sum "$unit" | awk '{print $1}')
    expected=$(cat "$state_dir/distro-unit.sha256")
    assert_equal "$expected" "$current" "rollback vendor unit differs"
    record "rollback package-binary-unit-identity"
}

prepare_upgrade() {
    mkdir -p "$state_dir" "$log_dir"
    export DEBIAN_FRONTEND=noninteractive
    apt-get update
    apt-get install -y dante-server python3 passwd
    mkdir -p "$state_dir/distro"
    (cd "$state_dir/distro" && apt-get download dante-server)
    distro_deb=$(find "$state_dir/distro" -maxdepth 1 -name 'dante-server_*.deb' -print -quit)
    [ -n "$distro_deb" ] || fail "could not save distro dante-server package for rollback"
    printf '%s\n' "$distro_deb" > "$state_dir/distro.path"
    unit=$(systemctl show -p FragmentPath --value danted.service)
    [ -f "$unit" ] || fail "distro package did not install danted.service"
    sha256sum "$unit" | awk '{print $1}' > "$state_dir/distro-unit.sha256"
    dpkg-query -W -f='${Version}' dante-server > "$state_dir/distro-version"
    sha256sum /usr/sbin/danted | awk '{print $1}' > "$state_dir/distro-binary.sha256"
    dpkg-query -W -f='${Conffiles}\n' dante-server | \
        awk '$1 == "/etc/danted.conf" { print $1, $2 }' > "$state_dir/distro-conffile.metadata"
    [ -s "$state_dir/distro-conffile.metadata" ] || fail "distro package does not register /etc/danted.conf"
    setup_pam
    write_test_config
    systemctl enable danted.service
    systemctl restart danted.service
    systemctl is-active --quiet danted.service || fail "distro danted service did not start"
    run_proxy_probes
    record "distro-baseline"
    install_candidate
    assert_unit_state enabled
    assert_unit_activity active
    assert_layout
    assert_preserved
    run_proxy_probes

    restore_distro
    systemctl disable --now danted.service
    install_candidate
    assert_unit_state disabled
    assert_unit_activity inactive
    assert_preserved

    restore_distro
    systemctl mask --now danted.service
    install_candidate
    assert_unit_state masked
    assert_unit_activity inactive
    assert_preserved

    systemctl unmask danted.service
    systemctl enable --now danted.service
    systemctl is-active --quiet danted.service || fail "danted service did not start after upgrade"
    run_proxy_probes
    touch "$state_dir/reboot-required"
    record "upgrade ready-for-reboot"
}

restore_distro() {
    distro_deb=$(cat "$state_dir/distro.path")
    DEBIAN_FRONTEND=noninteractive apt-get \
        -o Dpkg::Options::=--force-confold \
        -o Dpkg::Options::=--force-confdef \
        install -y --reinstall --allow-downgrades "$distro_deb"
    assert_preserved
}

post_reboot() {
    [ -f "$state_dir/reboot-required" ] || fail "reboot phase marker is missing"
    systemctl is-active --quiet danted.service || fail "danted is not active after reboot"
    assert_unit_state enabled
    assert_layout
    assert_preserved
    run_proxy_probes
    record "reboot service-and-config"
}

rollback_package() {
    distro_deb=$(cat "$state_dir/distro.path")
    [ -f "$distro_deb" ] || fail "saved distro DEB is missing"
    DEBIAN_FRONTEND=noninteractive apt-get \
        -o Dpkg::Options::=--force-confold \
        -o Dpkg::Options::=--force-confdef \
        install -y --allow-downgrades "$distro_deb"
    systemctl unmask danted.service
    systemctl enable --now danted.service
    assert_rollback_identity
    assert_preserved
    systemctl is-active --quiet danted.service || fail "danted did not start after rollback"
    run_proxy_probes
    record "rollback"
}

clean_install() {
    systemctl unmask danted.service 2>/dev/null || true
    DEBIAN_FRONTEND=noninteractive apt-get purge -y dante-server
    rm -f /etc/danted.conf /etc/sockd.conf
    rm -rf /etc/systemd/system/danted.service.d
    systemctl daemon-reload
    install_candidate
    setup_pam
    write_test_config
    sed -i 's/^user\.unprivileged: nobody$/user.unprivileged: proxy/' /etc/danted.conf
    mkdir -p /etc/systemd/system/danted.service.d
    cat > /etc/systemd/system/danted.service.d/statistics.conf <<'EOF'
[Service]
ExecStartPre=/usr/bin/install -d -o proxy -g proxy -m 0750 /run/danted-stats
ExecStart=
ExecStart=/usr/sbin/danted -S /run/danted-stats/stats.sock
EOF
    systemctl daemon-reload
    systemctl enable --now danted.service
    systemctl is-active --quiet danted.service || fail "clean install did not start"
    assert_layout
    run_proxy_probes
    [ "$(stat -c %U /run/danted-stats/stats.sock)" = proxy ] || fail "statistics socket owner is not proxy"
    runuser -u proxy -- /usr/bin/python3 "$probe" stats /run/danted-stats/stats.sock > "$log_dir/stats.json"
    record "clean-install stats-api non-nobody-user"
}

guard_vm
[ -f "$candidate" ] || fail "candidate DEB is missing"
[ -f "$probe" ] || fail "guest probe is missing"
case "$target" in ubuntu22.04|debian12) ;; *) fail "unsupported target $target" ;; esac
mkdir -p "$log_dir"
chmod 0755 "$probe"
trap 'collect_failure_diagnostics "$?"' 0

case "$phase" in
    upgrade) prepare_upgrade ;;
    post-reboot) post_reboot ;;
    rollback) rollback_package ;;
    clean-install) clean_install ;;
    *) fail "unknown phase $phase (expected upgrade, post-reboot, rollback, or clean-install)" ;;
esac
