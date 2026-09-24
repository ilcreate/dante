# Debian package compatibility: evidence

Source: [approved implementation plan](debian-package-compatibility.md).
Operational instructions: [Debian and Ubuntu packages](../debian-packages.md).

Subsequent CI policy change: at the owner's request, `verify-deb` and its VM
matrix were removed from the shared workflow to shorten releases. VM tools
remain available for manual checks. The evidence below records the original
implementation; VM execution is no longer a release requirement.

## Scope and user journeys

- Upgrade a distribution `dante-server` without changing its binary, service,
  configuration paths, local PAM policy, or administrative overrides.
- Require PAM and libwrap in every DEB; preserve the distribution's disabled
  GSSAPI setting and opt-in statistics API.
- Build Ubuntu 22.04 and Debian 12 packages for amd64 and arm64 from one
  committed source tree; gate draft releases on package and VM verification.
- Verify authentication, TCP/UDP relay, reboot, rollback, and clean installation
  in disposable machines rather than treating filesystem checks as runtime proof.

The plan was read as task data. Its validation intent was translated to the
repository's Python unittest suite, native Debian builds, C statistics harness,
linters, and isolated QEMU guests. No production deployment is part of this task.

## RED and GREEN checkpoints

These checkpoints were created during implementation on branch `systemd`.
The user requested one squash commit; the table preserves the original
RED/GREEN evidence even though these hashes are no longer in the linear history.

| Guarantee | RED evidence | GREEN evidence |
| --- | --- | --- |
| Native packaging, distro paths, mandatory authentication dependencies | `7aea2d2`: five packaging tests, three failures and two errors against old packaging | `bdfea64`: seven packaging tests passed; native build verified below |
| Statistics regression harness runs during package build | `f2651dc`: missing `override_dh_auto_test` failed the packaging test | Native build executes the harness and reports `all stats API tests passed` |
| Source archive includes root `debian/`; four DEB targets and target-specific release identity | `71a151c`: metadata, release, and DEB routing tests failed against the custom path | `704bbdc`: 21 focused tests passed |
| Disposable VM orchestration and authentication/relay probes | `09e36ff`: intended missing VM implementation import failure | `6aaddc7`: combined 66 tests passed |
| Release rejects incomplete or inconsistent DEB metadata | `4b0502a`: four failing metadata cases | `fe468fb`: 16 focused tests passed |
| Upgrade checks activity before manual recovery; PAM policy preservation and exact rollback | `76f69bf`: six failing guest contract tests | `c05c868`: six contract tests passed; ShellCheck passed |
| Installed manual documents the actual default PAM service `sockd` | `f04967c`: one failing regression against the inherited rename patch | `bdfea64`: regression passes; misleading PAM-default renames removed |
| Readiness checks do not leave remote cloud-init wait processes behind | `c5bec63`: regression exposed blocking polling observed during TCG boot | `19352da`: 43 VM/probe tests passed |
| Debug packages referenced by `.changes` remain available for diagnostics | `94f876a`: routing contract failed after a real build exposed discarded `.ddeb` files | `c63c40f`: five routing tests passed; repeated Ubuntu build preserved all three `.ddeb` files |
| UDP fixture permits response traffic as well as association | `0443282`: stock Debian VM passed PAM/TCP but UDP timed out; regression found missing `udpreply` | `7381a7e`: 44 VM/probe tests passed after correcting the fixture; full VM result recorded separately |
| Reboot can clear `/tmp`; clean installation exercises the documented API override | `737874d`: real post-reboot phase could not find its script; two contracts exposed missing restaging and a different override | `c177d57`: 45 VM/probe tests passed; all three guest inputs are restaged and the override matches the committed documentation |
| CI fixture prepares the socket directory for the daemon's configured user | `db73d70`: regression fails on root-owned `RuntimeDirectory` setup after a real clean-install socket permission failure | Final check: 77 tests passed after using `install -d -o proxy -g proxy`; full VM rerun not performed |

## Test specification

| Guarantee | Test target | Level |
| --- | --- | --- |
| Packaging uses native helpers, distro paths, PAM/libwrap and provenance | `dante/ci/tests/test_debian_packaging.py` | Packaging contracts |
| Archive includes both committed trees; target matrix and release metadata agree | `test_metadata.py`, `test_release.py`, `test_ci_debian_route.py` in `dante/ci/tests/` | Unit/integration |
| Cloud image checksum, QEMU isolation, cleanup, phase failures and lifecycle | `dante/ci/tests/test_deb_vm.py` | Unit/integration |
| SOCKS authentication, TCP/UDP relay, statistics schema, guest safety and lifecycle assertions | `dante/ci/tests/test_deb_upgrade.py` | Unit/integration |
| Native compilation and statistics regressions | `dpkg-buildpackage -b -us -uc`; `bash sockd/tests/run-stats-api-tests.sh` via `debian/rules` | Native build/C harness |
| Actual distribution upgrade, reboot, rollback and clean install | `python3 dante/ci/deb_vm.py --target <target> --arch <arch> --package <deb> --output <directory>` | QEMU/systemd end to end |

Contract tests inspect packaging and guest-script requirements; they do not
establish that a real package upgrade succeeds. The native and VM results below
are recorded separately for that reason.

## Execution results

### Native builds

Both native arm64 builds used the archive exported by:

```sh
python3 dante/ci/metadata.py --output /private/tmp/dante-integrated-source --run-number 1
```

The source commit was `bdfea64ce653b0c219b75715502270b1325dc89a`.
Inside the corresponding Linux build containers, both commands completed with
exit status 0:

```sh
bash dante/ci/build-package.sh deb INPUT OUTPUT debian12
bash dante/ci/build-package.sh deb INPUT OUTPUT ubuntu22.04
```

The resulting server versions were
`1.4.4-0.ci.1.gbdfea64ce653+debian12` and
`1.4.4-0.ci.1.gbdfea64ce653+ubuntu22.04`.
Both native builds executed the C statistics harness successfully and passed
`lintian --fail-on error`. Warnings remain for the existing preview revision
format, inherited NEWS metadata, and legacy file encoding. Both binaries
reported `build: libwrap mon-data mon-disconnect pam preload sess2`; dependencies
include `libpam0g` and `libwrap0`. Payload, architecture, version, and conffile
checks passed. Debian's installed config, unit, and SysV init script were also
compared byte for byte with the official Debian 12 package and matched.

### Unit tests, coverage, and lint

```sh
python3 -m unittest discover -s dante/ci/tests
coverage run --branch --source=dante/ci -m unittest discover -s dante/ci/tests
coverage report --include='*/metadata.py,*/release.py,*/deb_vm.py,*/deb_probe.py' --fail-under=80
shellcheck dante/ci/*.sh dante/ci/packaging/dante-start dante/sockd/tests/run-stats-api-tests.sh
actionlint -shellcheck /private/tmp/dante-ci-tools/bin/shellcheck
git diff --check
```

The coverage run used `/private/tmp/dante-ci-tools/bin/python` and
`COVERAGE_FILE=/private/tmp/dante-final-coverage`. All 77 tests passed.
Changed Python modules have 87% combined statement/branch coverage:
`deb_probe.py` 85%, `deb_vm.py` 86%, `metadata.py` 89%, `release.py` 92%.
ShellCheck and actionlint passed. Python coverage does not measure shell,
Debian rules, or C execution; their native and runtime evidence is separate.

### Runtime verification

Both pinned arm64 cloud images booted successfully under QEMU TCG with systemd,
cloud-init, and SSH. The Debian 12 arm64 run passed:

- Stock-to-candidate upgrade with enabled, disabled, and masked service states.
- Configuration, permissions, PAM policy, and override preservation.
- Successful PAM login, rejected password, rejected expired account, TCP and UDP.
- Reboot and repeated authentication/relay checks.
- Rollback with exact distro version, binary, and vendor-unit checks.
- Clean-install binary layout and PAM/TCP/UDP checks.

The final statistics socket check failed because the CI fixture used
`RuntimeDirectory=` with a root-started unit and a socket created by the
unprivileged daemon. The final fixture and matching instructions use
`ExecStartPre=install -d` with explicit ownership instead. The regression suite
passes; this last fixture adjustment has not been rerun in a full VM.

At the user's request, further runtime investigation stopped and work was
concluded with CI/CD validation and a squash commit. No C runtime sources changed.
The complete Ubuntu VM upgrade flow, amd64 native/VM targets, RPM/macOS builds,
and the remote GitHub Actions matrix were not executed locally. The release
workflow now requires validation and package builds only; VM checks are manual.
