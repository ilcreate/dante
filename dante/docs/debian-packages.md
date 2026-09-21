# Debian and Ubuntu packages

The DEB build uses the repository's `debian/rules` through
`dpkg-buildpackage -b -us -uc`. The package retains the distribution interface:

| Item | Installed interface |
| --- | --- |
| Package | `dante-server` |
| Executable | `/usr/sbin/danted` |
| Configuration | `/etc/danted.conf` |
| Service | `danted.service` |
| Vendor unit | `/lib/systemd/system/danted.service` |
| PID file | `/run/danted.pid` (`/var/run/danted.pid` on the configured path) |

PAM and TCP Wrappers (`libwrap`) are required at build time. GSSAPI remains
disabled, matching the distribution packaging used as the baseline. Existing
PAM policies are not replaced. In particular, the PAM service name is selected
by `pam.servicename` in Dante rules; its default is `sockd`, independently of
the executable's name.

## Choose the correct artifact

Use `dpkg --print-architecture` and `/etc/os-release` to select a package:

- Ubuntu 22.04: `dante-server_<version>-<revision>+ubuntu22.04_<arch>.deb`.
- Debian 12: `dante-server_<version>-<revision>+debian12_<arch>.deb`.
- Supported architectures: `amd64` and `arm64`.

Do not use the Debian 12 binary as a substitute for the Ubuntu 22.04 build.
The actual package version includes the distribution suffix. Its accompanying
`.build-info.json` records that version, the base release version, and the
source commit. Verify the artifact against the release's `SHA256SUMS`.

CI builds the full Debian package family, preserving the native packaging
rules. Only `dante-server` is published in the release; the other generated
packages, `.changes`, `.buildinfo`, and diagnostic logs remain CI artifacts.

## Upgrade a distribution installation

Before upgrading, save the installed package version, an available copy of
the old DEB, `/etc/danted.conf`, relevant PAM policies, and any systemd
overrides. Record `systemctl is-enabled danted` and `systemctl is-active danted`.
Keep backup files containing local policy or credentials private.

Inspect the candidate and install it using APT, so dependencies are resolved:

```sh
dpkg-deb --info ./dante-server_<version>-<revision>+ubuntu22.04_amd64.deb
sudo apt install ./dante-server_<version>-<revision>+ubuntu22.04_amd64.deb
/usr/sbin/danted -vv
sudo /usr/sbin/danted -V -f /etc/danted.conf
systemctl status danted.service
journalctl -u danted.service -n 100 --no-pager
```

Replace the example filename with the downloaded artifact. Retain your local
configuration if dpkg requests a conffile decision; do not use `--force-confnew`.
The package uses debhelper's service lifecycle and retains the existing
service name, enablement, and overrides. A service restart can disconnect
existing SOCKS sessions; schedule the upgrade accordingly. A disabled or
masked service must not be enabled or unmasked by the upgrade.

Verify a successful PAM login, a rejected login, and a proxied connection
using your existing client and policy before rolling out to additional hosts.
An obsolete `/etc/sockd.conf` may remain from an earlier distribution package;
the new service continues to use `/etc/danted.conf`.

## Opt in to the statistics API

The distribution unit does not enable the API automatically. Its absence is
intentional: updating a package must not silently change the startup command.
The binary supports `-S` and the existing JSON API without changing its schema.

Create a dedicated override, for example
`/etc/systemd/system/danted.service.d/statistics.conf`:

```ini
[Service]
ExecStartPre=/usr/bin/install -d -o proxy -g proxy -m 0750 /run/danted-stats
ExecStart=
ExecStart=/usr/sbin/danted -S /run/danted-stats/stats.sock
```

Replace the `-o proxy -g proxy` values with the actual account and group corresponding to
`user.unprivileged` in `/etc/danted.conf`. Do not assume it is `nobody`.
The snippet appends its preparation to the vendor unit's existing
`ExecStartPre` commands, retaining PID-file preparation. Review and merge with
any existing `ExecStart` override rather than discarding its arguments.
The directory is prepared on every start, including after reboot. Do not add
`RuntimeDirectory=` for this path: the unit starts as root, while the daemon
uses its configured unprivileged account for the socket.

```sh
sudo systemctl daemon-reload
sudo systemctl restart danted.service
sudo curl --noproxy '*' --unix-socket /run/danted-stats/stats.sock \
  http://localhost/v1/stats
```

The API socket is mode `0600`. Query as root or the socket owner; do not make
it world-readable. To disable the API, remove only the statistics override
you created, reload systemd, and restart the service. Keep unrelated overrides.

## Rollback

Before reverting to a distribution binary that lacks `-S`, remove only the
statistics override and reload systemd. Install the saved distribution DEB
using `apt install --allow-downgrades ./<saved-package>.deb`, retaining your
local configuration. Restore saved configuration or policy only if it changed,
then restore the previously recorded service state. Validate configuration,
PAM authentication, and relay traffic again. Downgrades also interrupt sessions.

Upgrade and rollback checks in a disposable VM with real systemd are optional
manual checks via `dante/ci/deb_vm.py`; they are not run by the CI or release
workflows. Releases wait for validation and package builds. Consult the task's
TDD evidence report for environments actually executed during development.

## Previously published custom sockd packages

Automatic `sockd` to `danted` migration is not supported. Handle these hosts
in a maintenance window:

1. Save the old DEB, both possible configuration files, PAM policies, service
   definitions, and their enablement states.
2. Validate and explicitly select the configuration to retain; do not merge
   `/etc/sockd.conf` and `/etc/danted.conf` automatically.
3. Stop and disable the old `sockd.service` before installing the replacement.
4. Install the target-specific package and place the selected configuration
   at `/etc/danted.conf`, preserving its intended ownership and permissions.
5. Adapt any old overrides to `danted.service`; enable/start it according to
   the recorded operational intent and verify PAM and relay traffic.

Keep the old configuration as a backup until validation is complete. Do not
run both services on the same listener or use a symlink as a substitute for
reviewing the configuration and service arguments.
