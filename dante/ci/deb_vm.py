#!/usr/bin/env python3
"""Verify a dante-server DEB by upgrading a real disposable systemd VM."""

from __future__ import annotations

import argparse
import hashlib
import json
import os
import platform
import shutil
import signal
import socket
import subprocess
import sys
import time
import urllib.parse
import urllib.request
from pathlib import Path
from typing import NamedTuple


CI = Path(__file__).resolve().parent
MANIFEST = CI / "deb_vm_images.json"
SSH_USER = "dante-ci"
BOOT_TIMEOUT = 600
PHASE_TIMEOUT = 900


class Image(NamedTuple):
    url: str
    algorithm: str
    checksum: str


def load_manifest(path: Path) -> dict[str, Image]:
    raw = json.loads(path.read_text())
    result = {}
    for key, value in raw.items():
        algorithm = value["algorithm"]
        if algorithm not in hashlib.algorithms_available:
            raise ValueError(f"unsupported checksum algorithm for {key}: {algorithm}")
        result[key] = Image(value["url"], algorithm, value["checksum"].lower())
    return result


def digest(path: Path, algorithm: str) -> str:
    checksum = hashlib.new(algorithm)
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            checksum.update(block)
    return checksum.hexdigest()


def download(url: str, destination: Path) -> None:
    temporary = destination.with_suffix(destination.suffix + ".part")
    temporary.unlink(missing_ok=True)
    try:
        with urllib.request.urlopen(url, timeout=60) as response, temporary.open("wb") as output:
            while block := response.read(1024 * 1024):
                output.write(block)
        temporary.replace(destination)
    finally:
        temporary.unlink(missing_ok=True)


def ensure_image(image: Image, destination: Path) -> Path:
    destination.parent.mkdir(parents=True, exist_ok=True)
    if destination.exists() and digest(destination, image.algorithm) == image.checksum:
        return destination
    download(image.url, destination)
    actual = digest(destination, image.algorithm)
    if actual != image.checksum:
        destination.unlink(missing_ok=True)
        raise RuntimeError(
            f"cloud image checksum mismatch: expected {image.checksum}, got {actual}"
        )
    return destination


def resolve_tools(arch: str) -> dict[str, str]:
    names = {
        "qemu": "qemu-system-x86_64" if arch == "amd64" else "qemu-system-aarch64",
        "qemu-img": "qemu-img",
        "cloud-localds": "cloud-localds",
        "ssh": "ssh",
        "scp": "scp",
        "ssh-keygen": "ssh-keygen",
    }
    tools = {key: shutil.which(name) for key, name in names.items()}
    missing = [names[key] for key, value in tools.items() if value is None]
    if missing:
        raise RuntimeError("missing VM host tools: " + ", ".join(missing))
    return {key: value for key, value in tools.items() if value is not None}


def firmware_for_arm64() -> Path:
    candidates = [
        Path("/usr/share/AAVMF/AAVMF_CODE.fd"),
        Path("/usr/share/qemu-efi-aarch64/QEMU_EFI.fd"),
        Path("/usr/share/edk2/aarch64/QEMU_EFI.fd"),
    ]
    for candidate in candidates:
        if candidate.is_file():
            return candidate
    raise RuntimeError("arm64 QEMU needs qemu-efi-aarch64 (AAVMF_CODE.fd or QEMU_EFI.fd)")


def acceleration_for(arch: str) -> str:
    machine = platform.machine().lower()
    native = (arch == "amd64" and machine in {"x86_64", "amd64"}) or (
        arch == "arm64" and machine in {"aarch64", "arm64"}
    )
    return "kvm" if native and os.access("/dev/kvm", os.R_OK | os.W_OK) else "tcg"


def cloud_init_user_data(public_key: str) -> str:
    return f"""#cloud-config
users:
  - name: {SSH_USER}
    lock_passwd: true
    groups: [sudo]
    shell: /bin/bash
    sudo: ALL=(ALL) NOPASSWD:ALL
    ssh_authorized_keys:
      - {public_key.strip()}
ssh_pwauth: false
disable_root: true
runcmd:
  - [sh, -c, 'printf disposable > /etc/dante-ci-disposable']
  - [systemctl, enable, --now, ssh.service]
final_message: dante-ci-ready
"""


def qemu_command(
    *, arch: str, qemu: str, overlay: Path, seed: Path, console: Path,
    ssh_port: int, acceleration: str, firmware: Path | None,
) -> list[str]:
    machine = "q35" if arch == "amd64" else "virt"
    command = [
        qemu, "-name", f"dante-ci-{arch}", "-machine", f"{machine},accel={acceleration}",
        "-cpu", "host" if acceleration == "kvm" else "max", "-smp", "2", "-m", "2048",
        "-drive", f"if=virtio,format=qcow2,file={overlay}",
        "-drive", f"if=virtio,format=raw,readonly=on,file={seed}",
        "-netdev", f"user,id=net0,restrict=off,hostfwd=tcp:127.0.0.1:{ssh_port}-:22",
        "-device", "virtio-net-pci,netdev=net0", "-display", "none", "-monitor", "none",
        "-serial", f"file:{console}",
    ]
    if arch == "arm64":
        if firmware is None:
            raise ValueError("arm64 firmware is required")
        command.extend(["-drive", f"if=pflash,format=raw,readonly=on,file={firmware}"])
    return command


def free_port() -> int:
    with socket.socket() as listener:
        listener.bind(("127.0.0.1", 0))
        return listener.getsockname()[1]


class VM:
    def __init__(self, tools: dict[str, str], arch: str, work: Path, base: Path):
        self.tools = tools
        self.arch = arch
        self.work = work
        self.base = base
        self.port = free_port()
        self.key = work / "id_ed25519"
        self.overlay = work / "overlay.qcow2"
        self.seed = work / "seed.img"
        self.console = work / "console.log"
        self.process: subprocess.Popen | None = None
        self.qemu_log = None

    @property
    def ssh_options(self) -> list[str]:
        return [
            "-i", str(self.key), "-p", str(self.port), "-o", "BatchMode=yes",
            "-o", "StrictHostKeyChecking=no", "-o", "UserKnownHostsFile=/dev/null",
            "-o", "ConnectTimeout=10", "-o", "LogLevel=ERROR",
        ]

    @property
    def scp_options(self) -> list[str]:
        options = self.ssh_options
        options[options.index("-p")] = "-P"
        return options

    def prepare(self) -> None:
        subprocess.run(
            [self.tools["ssh-keygen"], "-q", "-t", "ed25519", "-N", "", "-C", "dante-ci", "-f", str(self.key)],
            check=True,
        )
        user_data = self.work / "user-data"
        metadata = self.work / "meta-data"
        user_data.write_text(cloud_init_user_data(self.key.with_suffix(".pub").read_text()))
        metadata.write_text("instance-id: dante-ci\nlocal-hostname: dante-ci\n")
        subprocess.run([self.tools["cloud-localds"], str(self.seed), str(user_data), str(metadata)], check=True)
        subprocess.run(
            [self.tools["qemu-img"], "create", "-q", "-f", "qcow2", "-F", "qcow2", "-b", str(self.base), str(self.overlay), "12G"],
            check=True,
        )

    def start(self) -> None:
        firmware = firmware_for_arm64() if self.arch == "arm64" else None
        command = qemu_command(
            arch=self.arch, qemu=self.tools["qemu"], overlay=self.overlay, seed=self.seed,
            console=self.console, ssh_port=self.port, acceleration=acceleration_for(self.arch),
            firmware=firmware,
        )
        (self.work / "qemu-command.json").write_text(json.dumps(command, indent=2) + "\n")
        self.qemu_log = (self.work / "qemu.log").open("wb")
        self.process = subprocess.Popen(command, stdout=self.qemu_log, stderr=subprocess.STDOUT)
        self.wait_ready(BOOT_TIMEOUT)

    def wait_ready(self, timeout: int) -> None:
        deadline = time.monotonic() + timeout
        while time.monotonic() < deadline:
            if self.process is not None and self.process.poll() is not None:
                raise RuntimeError(f"QEMU exited with {self.process.returncode}; see {self.console}")
            try:
                result = subprocess.run(
                    [self.tools["ssh"], *self.ssh_options, f"{SSH_USER}@127.0.0.1",
                     "test -f /etc/dante-ci-disposable && "
                     "cloud-init status 2>/dev/null | grep -q '^status: done$'"],
                    stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, timeout=15,
                )
            except subprocess.TimeoutExpired:
                time.sleep(2)
                continue
            if result.returncode == 0:
                return
            time.sleep(2)
        raise TimeoutError(f"VM did not become ready in {timeout}s; see {self.console}")

    def copy_to(self, source: Path, remote: str) -> None:
        subprocess.run(
            [self.tools["scp"], *self.scp_options, str(source), f"{SSH_USER}@127.0.0.1:{remote}"],
            check=True,
        )

    def copy_from(self, remote: str, destination: Path) -> None:
        subprocess.run(
            [self.tools["scp"], *self.scp_options, "-r", f"{SSH_USER}@127.0.0.1:{remote}", str(destination)],
            check=True,
        )

    def run_phase(self, phase: str, target: str) -> None:
        log = self.work / f"{phase}.log"
        command = [
            self.tools["ssh"], *self.ssh_options, f"{SSH_USER}@127.0.0.1",
            "sudo", "env", "DANTE_CI_VM=1", "/bin/sh", "/tmp/deb_upgrade.sh", phase,
            "/tmp/dante-server.deb", target, "/tmp/deb_probe.py",
        ]
        with log.open("wb") as stream:
            result = subprocess.run(command, stdout=stream, stderr=subprocess.STDOUT, timeout=PHASE_TIMEOUT)
        if result.returncode != 0:
            raise RuntimeError(f"VM phase {phase!r} failed ({result.returncode}); see {log}")

    def reboot(self) -> None:
        subprocess.run(
            [self.tools["ssh"], *self.ssh_options, f"{SSH_USER}@127.0.0.1", "sudo", "systemctl", "reboot"],
            stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL,
        )
        deadline = time.monotonic() + 90
        while time.monotonic() < deadline:
            try:
                result = subprocess.run(
                    [self.tools["ssh"], *self.ssh_options, f"{SSH_USER}@127.0.0.1", "true"],
                    stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, timeout=15,
                )
            except subprocess.TimeoutExpired:
                result = subprocess.CompletedProcess([], 255)
            if result.returncode != 0:
                break
            time.sleep(1)
        else:
            raise TimeoutError("VM did not disconnect for reboot")
        self.wait_ready(BOOT_TIMEOUT)

    def stop(self) -> None:
        if self.process is None:
            return
        if self.process.poll() is None:
            try:
                subprocess.run(
                    [self.tools["ssh"], *self.ssh_options, f"{SSH_USER}@127.0.0.1", "sudo", "poweroff"],
                    stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, timeout=15,
                )
            except subprocess.TimeoutExpired:
                pass
            try:
                self.process.wait(timeout=30)
            except subprocess.TimeoutExpired:
                self.process.send_signal(signal.SIGTERM)
                try:
                    self.process.wait(timeout=10)
                except subprocess.TimeoutExpired:
                    self.process.kill()
                    self.process.wait(timeout=10)
        if self.qemu_log is not None:
            self.qemu_log.close()

    def cleanup_sensitive(self) -> None:
        for path in (
            self.key, self.key.with_suffix(".pub"), self.seed, self.overlay,
            self.work / "user-data", self.work / "meta-data",
        ):
            path.unlink(missing_ok=True)


def parse_args(argv: list[str] | None = None) -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--target", required=True, choices=("ubuntu22.04", "debian12"))
    parser.add_argument("--arch", required=True, choices=("amd64", "arm64"))
    parser.add_argument("--package", required=True, type=Path)
    parser.add_argument("--output", required=True, type=Path)
    return parser.parse_args(argv)


def main(argv: list[str] | None = None) -> int:
    args = parse_args(argv)
    package = args.package.resolve()
    if not package.is_file() or package.suffix != ".deb":
        raise ValueError(f"candidate package is not a readable .deb: {package}")
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=True)
    tools = resolve_tools(args.arch)
    image = load_manifest(MANIFEST)[f"{args.target}/{args.arch}"]
    default_cache = Path.home() / ".cache" / "dante-vm-images"
    cache = Path(os.environ.get("DANTE_VM_CACHE", default_cache)).resolve()
    filename = Path(urllib.parse.urlparse(image.url).path).name
    base = ensure_image(image, cache / filename)
    work = output / f"{args.target}-{args.arch}"
    work.mkdir(parents=True, exist_ok=True)
    vm = VM(tools, args.arch, work, base)
    result = {"target": args.target, "arch": args.arch, "image": image.url, "status": "failed"}
    try:
        vm.prepare()
        vm.start()
        vm.copy_to(package, "/tmp/dante-server.deb")
        vm.copy_to(CI / "deb_upgrade.sh", "/tmp/deb_upgrade.sh")
        vm.copy_to(CI / "deb_probe.py", "/tmp/deb_probe.py")
        vm.run_phase("upgrade", args.target)
        vm.reboot()
        vm.copy_to(package, "/tmp/dante-server.deb")
        vm.copy_to(CI / "deb_upgrade.sh", "/tmp/deb_upgrade.sh")
        vm.copy_to(CI / "deb_probe.py", "/tmp/deb_probe.py")
        vm.run_phase("post-reboot", args.target)
        vm.run_phase("rollback", args.target)
        vm.run_phase("clean-install", args.target)
        result["status"] = "passed"
        return 0
    finally:
        try:
            vm.copy_from("/var/log/dante-ci", work / "guest-logs")
        except (subprocess.CalledProcessError, OSError):
            pass
        vm.stop()
        vm.cleanup_sensitive()
        (work / "result.json").write_text(json.dumps(result, indent=2) + "\n")


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (OSError, RuntimeError, ValueError, subprocess.SubprocessError) as exc:
        print(f"deb-vm: {exc}", file=sys.stderr)
        raise SystemExit(1)
