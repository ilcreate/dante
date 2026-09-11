#!/usr/bin/env python3
"""Create native server packages from a built source tree (no third-party packager)."""

import json
import platform
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path

CI = Path(__file__).resolve().parent


def run(*args, **kwargs):
    return subprocess.run([str(arg) for arg in args], check=True, **kwargs)


def copy(src, root, dest, mode=0o644):
    target = root / dest.lstrip("/")
    target.parent.mkdir(parents=True, exist_ok=True)
    shutil.copyfile(src, target)
    target.chmod(mode)


def write(path, text, mode=0o644):
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(text)
    path.chmod(mode)


def main():
    kind, source, metadata, output = sys.argv[1:]
    source, output = Path(source).resolve(), Path(output).resolve()
    data = json.loads(Path(metadata).read_text())
    machine = platform.machine()
    arch = {"x86_64": "amd64", "aarch64": "arm64", "arm64": "arm64"}[machine]
    version, revision = data["version"], data["revision"]
    if (source / "VERSION").read_text().strip() != f"Dante {version}":
        raise ValueError("configured version differs from release metadata; regenerate configure after a version bump")
    package_version = data["package_version"]
    prefix = "usr/local" if kind == "macos" else "usr"
    with tempfile.TemporaryDirectory(prefix="dante-native-") as tmp:
        work = Path(tmp)
        payload = work / "payload"
        copy(source / "sockd/sockd", payload, f"{prefix}/sbin/sockd", 0o755)
        for name in ["sockd.8", "sockd.conf.5"]:
            copy(source / "doc" / name, payload, f"{prefix}/share/man/man{name[-1]}/{name}")
        docs = f"{prefix}/share/doc/dante-server"
        for name in ["LICENSE", "CREDITS"]:
            copy(source / name, payload, f"{docs}/{name}")
        copy(CI / "packaging/sockd.conf", payload, f"{docs}/sockd.conf.example")
        for path in (source / "docs/features/statistics-api").glob("*.md"):
            if not path.name.endswith(".local.md"):
                copy(path, payload, f"{docs}/statistics-api/{path.name}")
        data.update({"format": kind, "architecture": arch,
                     "build_platform": platform.platform(),
                     "features": "server; native password auth; no optional auth modules"})
        write(payload / docs / "build-info.json", json.dumps(data, indent=2) + "\n")

        if kind == "deb":
            copy(CI / "packaging/sockd.conf", payload, "etc/sockd.conf")
            copy(CI / "packaging/sockd.service", payload, "usr/lib/systemd/system/sockd.service")
            # Resolve ABI dependencies from the actual ELF binary on Debian 12.
            write(work / "debian/control", "Source: dante-server\nMaintainer: Dante package maintainers\n\nPackage: dante-server\nArchitecture: any\nDescription: Dante SOCKS server\n")
            deps = subprocess.check_output(
                ["dpkg-shlibdeps", "-O", "-e" + str(payload / "usr/sbin/sockd")],
                cwd=work, text=True).strip().removeprefix("shlibs:Depends=")
            write(payload / "DEBIAN/control", f"Package: dante-server\nVersion: {package_version}\nArchitecture: {arch}\nMaintainer: Dante package maintainers\nSection: net\nPriority: optional\nDepends: {deps}\nConflicts: dante-server-stats\nHomepage: https://www.inet.no/dante/\nDescription: Dante SOCKS server with local statistics API\n This product includes software developed by Inferno Nettverk A/S, Norway.\n")
            write(payload / "DEBIAN/conffiles", "/etc/sockd.conf\n")
            write(payload / "DEBIAN/postinst", "#!/bin/sh\nset -e\nif [ \"$1\" = configure ] && [ -d /run/systemd/system ]; then\n  systemctl daemon-reload\nfi\n", 0o755)
            write(payload / "DEBIAN/prerm", "#!/bin/sh\nset -e\nif [ \"$1\" = remove ] && [ -d /run/systemd/system ]; then\n  systemctl stop sockd.service || true\nfi\n", 0o755)
            write(payload / "DEBIAN/postrm", "#!/bin/sh\nset -e\nif [ -d /run/systemd/system ]; then\n  systemctl daemon-reload\nfi\n", 0o755)
            artifact = output / f"dante-server_{package_version}_debian12_{arch}.deb"
            run("dpkg-deb", "--root-owner-group", "--build", payload, artifact)
            run("dpkg-deb", "--info", artifact)
            # Install with the real package manager, then test the installed binary.
            run("dpkg", "-i", artifact)
            run("/usr/sbin/sockd", "-V", "-f", "/etc/sockd.conf")
            run(sys.executable, CI / "smoke.py", "/usr/sbin/sockd", "--relay")
        elif kind == "rpm":
            copy(CI / "packaging/sockd.conf", payload, "etc/sockd.conf")
            copy(CI / "packaging/sockd.service", payload, "usr/lib/systemd/system/sockd.service")
            for directory in ["BUILD", "BUILDROOT", "RPMS", "SOURCES", "SPECS", "SRPMS"]:
                (work / directory).mkdir()
            spec = work / "SPECS/dante-server.spec"
            write(spec, f"""Name: dante-server
Version: {version}
Release: {revision}.el9
Summary: Dante SOCKS server with local statistics API
License: BSD-4-Clause
URL: https://www.inet.no/dante/
Conflicts: dante
%description
This product includes software developed by Inferno Nettverk A/S, Norway.
SOCKS v4/v5 server with a local statistics API.
%install
mkdir -p %{{buildroot}}
cp -a {payload}/. %{{buildroot}}/
%post
if [ -d /run/systemd/system ]; then systemctl daemon-reload; fi
%preun
if [ "$1" -eq 0 ] && [ -d /run/systemd/system ]; then systemctl stop sockd.service || :; fi
%postun
if [ -d /run/systemd/system ]; then systemctl daemon-reload; fi
%files
%config(noreplace) /etc/sockd.conf
/usr/sbin/sockd
/usr/lib/systemd/system/sockd.service
/usr/share/man/man5/sockd.conf.5*
/usr/share/man/man8/sockd.8*
%doc /usr/share/doc/dante-server
""")
            run("rpmbuild", "-bb", "--define", f"_topdir {work}",
                "--define", "debug_package %{nil}", spec)
            rpms = list((work / "RPMS").glob("*/*.rpm"))
            if len(rpms) != 1:
                raise RuntimeError(f"expected one RPM, got {rpms}")
            artifact = output / rpms[0].name
            shutil.copyfile(rpms[0], artifact)
            run("rpm", "-qip", artifact)
            run("dnf", "install", "-y", artifact)
            run("/usr/sbin/sockd", "-V", "-f", "/etc/sockd.conf")
            run(sys.executable, CI / "smoke.py", "/usr/sbin/sockd", "--relay")
        elif kind == "macos":
            # Config and launchd template stay in docs: installer cannot overwrite
            # local config or silently enable a daemon on the next system boot.
            copy(CI / "packaging/org.dante.sockd.plist", payload, f"{docs}/org.dante.sockd.plist")
            copy(CI / "packaging/dante-start", payload, "usr/local/libexec/dante-start", 0o755)
            linked = subprocess.check_output(["otool", "-L", str(payload / prefix / "sbin/sockd")], text=True)
            for line in linked.splitlines()[1:]:
                dependency = line.strip().split(" ")[0]
                if not dependency.startswith(("/usr/lib/", "/System/Library/")):
                    raise RuntimeError(f"non-system macOS dependency: {dependency}")
            artifact = output / f"dante-server-{package_version}-macos15-{arch}.pkg"
            run("pkgbuild", "--root", payload, "--identifier", "org.dante.sockd",
                "--version", package_version, "--ownership", "recommended",
                "--install-location", "/", artifact)
            # Expansion checks the actual archive without changing the local OS.
            run("pkgutil", "--expand-full", artifact, work / "expanded")
            binaries = list((work / "expanded").rglob("sbin/sockd"))
            if len(binaries) != 1:
                raise RuntimeError("package must contain exactly one sockd")
            run(sys.executable, CI / "smoke.py", binaries[0])
        else:
            raise ValueError(f"unknown package format: {kind}")
        shutil.copyfile(payload / docs / "build-info.json", output / (artifact.name + ".build-info.json"))


if __name__ == "__main__":
    main()
