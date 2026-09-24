import hashlib
import importlib.util
import json
import tempfile
import unittest
from pathlib import Path
from unittest import mock
import subprocess


CI = Path(__file__).parents[1]
spec = importlib.util.spec_from_file_location("deb_vm", CI / "deb_vm.py")
deb_vm = importlib.util.module_from_spec(spec)
spec.loader.exec_module(deb_vm)


class ImageManifestTests(unittest.TestCase):
    def test_all_supported_images_are_immutable_and_checksummed(self):
        manifest = deb_vm.load_manifest(CI / "deb_vm_images.json")
        self.assertEqual(
            set(manifest),
            {"ubuntu22.04/amd64", "ubuntu22.04/arm64", "debian12/amd64", "debian12/arm64"},
        )
        for key, image in manifest.items():
            with self.subTest(key=key):
                self.assertRegex(image.url, r"/(?:release-\d{8}|\d{8}-\d+)/")
                self.assertNotIn("/current/", image.url)
                expected_length = 64 if image.algorithm == "sha256" else 128
                self.assertEqual(len(image.checksum), expected_length)
                int(image.checksum, 16)

    def test_checksum_mismatch_is_rejected_without_replacing_cache(self):
        with tempfile.TemporaryDirectory() as tmp:
            cached = Path(tmp) / "image.qcow2"
            cached.write_bytes(b"corrupt")
            image = deb_vm.Image(
                url="https://example.invalid/image.qcow2",
                algorithm="sha256",
                checksum=hashlib.sha256(b"expected").hexdigest(),
            )
            with mock.patch.object(deb_vm, "download", side_effect=RuntimeError("download attempted")):
                with self.assertRaisesRegex(RuntimeError, "download attempted"):
                    deb_vm.ensure_image(image, cached)
            self.assertEqual(cached.read_bytes(), b"corrupt")

    def test_valid_cached_image_is_reused(self):
        with tempfile.TemporaryDirectory() as tmp:
            cached = Path(tmp) / "image.qcow2"
            cached.write_bytes(b"expected")
            image = deb_vm.Image(
                "https://example.invalid/image.qcow2", "sha256",
                hashlib.sha256(b"expected").hexdigest(),
            )
            with mock.patch.object(deb_vm, "download") as download:
                self.assertEqual(deb_vm.ensure_image(image, cached), cached)
            download.assert_not_called()

    def test_bad_download_is_removed(self):
        with tempfile.TemporaryDirectory() as tmp:
            cached = Path(tmp) / "image.qcow2"
            image = deb_vm.Image(
                "https://example.invalid/image.qcow2", "sha256",
                hashlib.sha256(b"expected").hexdigest(),
            )
            def fake_download(_url, destination):
                destination.write_bytes(b"wrong")
            with mock.patch.object(deb_vm, "download", side_effect=fake_download):
                with self.assertRaisesRegex(RuntimeError, "checksum mismatch"):
                    deb_vm.ensure_image(image, cached)
            self.assertFalse(cached.exists())

    def test_unknown_digest_algorithm_is_rejected(self):
        with tempfile.TemporaryDirectory() as tmp:
            manifest = Path(tmp) / "images.json"
            manifest.write_text(json.dumps({"x/y": {
                "url": "https://example.invalid/x", "algorithm": "not-a-hash", "checksum": "00"
            }}))
            with self.assertRaisesRegex(ValueError, "unsupported checksum"):
                deb_vm.load_manifest(manifest)

    def test_download_is_atomic(self):
        class Response:
            def __init__(self): self.blocks = [b"one", b"two", b""]
            def __enter__(self): return self
            def __exit__(self, *_args): return None
            def read(self, _size): return self.blocks.pop(0)
        with tempfile.TemporaryDirectory() as tmp:
            destination = Path(tmp) / "image.qcow2"
            with mock.patch.object(deb_vm.urllib.request, "urlopen", return_value=Response()):
                deb_vm.download("https://example.invalid/image", destination)
            self.assertEqual(destination.read_bytes(), b"onetwo")
            self.assertFalse(destination.with_suffix(".qcow2.part").exists())


class QemuCommandTests(unittest.TestCase):
    def test_amd64_command_uses_loopback_user_network_and_tcg_fallback(self):
        command = deb_vm.qemu_command(
            arch="amd64",
            qemu="/usr/bin/qemu-system-x86_64",
            overlay=Path("/tmp/overlay.qcow2"),
            seed=Path("/tmp/seed.img"),
            console=Path("/tmp/console.log"),
            ssh_port=43210,
            acceleration="tcg",
            firmware=None,
        )
        rendered = " ".join(command)
        self.assertIn("accel=tcg", rendered)
        self.assertIn("hostfwd=tcp:127.0.0.1:43210-:22", rendered)
        self.assertIn("restrict=off", rendered)
        self.assertNotIn("tap", rendered)
        self.assertNotIn("bridge", rendered)

    def test_arm64_command_requires_read_only_firmware(self):
        command = deb_vm.qemu_command(
            arch="arm64",
            qemu="/usr/bin/qemu-system-aarch64",
            overlay=Path("/tmp/overlay.qcow2"),
            seed=Path("/tmp/seed.img"),
            console=Path("/tmp/console.log"),
            ssh_port=43210,
            acceleration="kvm",
            firmware=Path("/usr/share/AAVMF/AAVMF_CODE.fd"),
        )
        self.assertIn("if=pflash,format=raw,readonly=on,file=/usr/share/AAVMF/AAVMF_CODE.fd", command)
        self.assertIn("host", command)

    def test_arm64_command_rejects_missing_firmware(self):
        with self.assertRaisesRegex(ValueError, "firmware"):
            deb_vm.qemu_command(
                arch="arm64", qemu="qemu", overlay=Path("overlay"), seed=Path("seed"),
                console=Path("console"), ssh_port=22, acceleration="tcg", firmware=None,
            )

    def test_acceleration_uses_kvm_only_for_native_accessible_arch(self):
        with mock.patch.object(deb_vm.platform, "machine", return_value="aarch64"), \
             mock.patch.object(deb_vm.os, "access", return_value=True):
            self.assertEqual(deb_vm.acceleration_for("arm64"), "kvm")
            self.assertEqual(deb_vm.acceleration_for("amd64"), "tcg")


class CloudInitTests(unittest.TestCase):
    def test_seed_marks_disposable_vm_and_installs_only_ephemeral_key(self):
        user_data = deb_vm.cloud_init_user_data("ssh-ed25519 AAAATEST dante-ci")
        self.assertIn("/etc/dante-ci-disposable", user_data)
        self.assertIn("ssh-ed25519 AAAATEST dante-ci", user_data)
        self.assertIn("lock_passwd: true", user_data)
        self.assertNotIn("password:", user_data)


class ToolValidationTests(unittest.TestCase):
    def test_missing_host_tool_is_actionable(self):
        with mock.patch.object(deb_vm.shutil, "which", return_value=None):
            with self.assertRaisesRegex(RuntimeError, "qemu-img"):
                deb_vm.resolve_tools("amd64")

    def test_present_host_tools_are_returned(self):
        with mock.patch.object(deb_vm.shutil, "which", side_effect=lambda name: "/usr/bin/" + name):
            tools = deb_vm.resolve_tools("arm64")
        self.assertEqual(tools["qemu"], "/usr/bin/qemu-system-aarch64")

    def test_arm_firmware_search_and_error(self):
        with mock.patch.object(deb_vm.Path, "is_file", side_effect=[False, True]):
            self.assertEqual(str(deb_vm.firmware_for_arm64()), "/usr/share/qemu-efi-aarch64/QEMU_EFI.fd")
        with mock.patch.object(deb_vm.Path, "is_file", return_value=False):
            with self.assertRaisesRegex(RuntimeError, "qemu-efi-aarch64"):
                deb_vm.firmware_for_arm64()


class VmLifecycleTests(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.addCleanup(self.tmp.cleanup)
        self.work = Path(self.tmp.name)
        self.tools = {name: f"/{name}" for name in (
            "qemu", "qemu-img", "cloud-localds", "ssh", "scp", "ssh-keygen"
        )}
        with mock.patch.object(deb_vm, "free_port", return_value=43210):
            self.vm = deb_vm.VM(self.tools, "amd64", self.work, self.work / "base.qcow2")

    def test_prepare_creates_overlay_and_locked_cloud_init_seed(self):
        def command_side_effect(command, **_kwargs):
            if command[0] == "/ssh-keygen":
                self.vm.key.with_suffix(".pub").write_text("ssh-ed25519 AAAATEST")
            return subprocess.CompletedProcess(command, 0)
        with mock.patch.object(deb_vm.subprocess, "run", side_effect=command_side_effect) as run:
            self.vm.prepare()
        user_data = (self.work / "user-data").read_text()
        self.assertIn("lock_passwd: true", user_data)
        self.assertTrue(any(call.args[0][0] == "/qemu-img" for call in run.call_args_list))

    def test_phase_failure_points_to_phase_log(self):
        self.vm.key.touch()
        with mock.patch.object(deb_vm.subprocess, "run", return_value=subprocess.CompletedProcess([], 7)):
            with self.assertRaisesRegex(RuntimeError, "upgrade.*failed"):
                self.vm.run_phase("upgrade", "debian12")
        self.assertTrue((self.work / "upgrade.log").exists())

    def test_phase_success_and_copy_from(self):
        with mock.patch.object(deb_vm.subprocess, "run", return_value=subprocess.CompletedProcess([], 0)) as run:
            self.vm.run_phase("rollback", "debian12")
            self.vm.copy_from("/var/log/dante-ci", self.work / "logs")
        self.assertEqual(run.call_count, 2)
        self.assertIn("-r", run.call_args.args[0])

    def test_phase_invokes_copied_script_through_explicit_shell(self):
        with mock.patch.object(deb_vm.subprocess, "run", return_value=subprocess.CompletedProcess([], 0)) as run:
            self.vm.run_phase("upgrade", "ubuntu22.04")
        command = run.call_args.args[0]
        script_index = command.index("/bin/sh")
        self.assertEqual(command[script_index:script_index + 2], ["/bin/sh", "/tmp/deb_upgrade.sh"])

    def test_start_writes_command_and_waits_for_guest(self):
        process = mock.Mock()
        with mock.patch.object(deb_vm, "qemu_command", return_value=["qemu", "safe"]), \
             mock.patch.object(deb_vm, "acceleration_for", return_value="tcg"), \
             mock.patch.object(deb_vm.subprocess, "Popen", return_value=process), \
             mock.patch.object(self.vm, "wait_ready") as wait:
            self.vm.start()
        self.assertEqual(json.loads((self.work / "qemu-command.json").read_text()), ["qemu", "safe"])
        wait.assert_called_once_with(deb_vm.BOOT_TIMEOUT)
        self.vm.qemu_log.close()

    def test_wait_ready_reports_early_qemu_exit_and_accepts_ssh(self):
        self.vm.process = mock.Mock()
        self.vm.process.poll.return_value = 9
        self.vm.process.returncode = 9
        with self.assertRaisesRegex(RuntimeError, "QEMU exited"):
            self.vm.wait_ready(1)
        self.vm.process.poll.return_value = None
        with mock.patch.object(deb_vm.subprocess, "run", return_value=subprocess.CompletedProcess([], 0)):
            self.vm.wait_ready(1)

    def test_wait_ready_polls_cloud_init_without_orphaning_wait_processes(self):
        self.vm.process = mock.Mock()
        self.vm.process.poll.return_value = None
        with mock.patch.object(deb_vm.subprocess, "run", return_value=subprocess.CompletedProcess([], 0)) as run:
            self.vm.wait_ready(1)
        remote_command = run.call_args.args[0][-1]
        self.assertIn("cloud-init status", remote_command)
        self.assertIn("status: done", remote_command)
        self.assertNotIn("--wait", remote_command)

    def test_reboot_waits_for_disconnect_then_new_readiness(self):
        outcomes = [subprocess.CompletedProcess([], 0), subprocess.CompletedProcess([], 255)]
        with mock.patch.object(deb_vm.subprocess, "run", side_effect=outcomes), \
             mock.patch.object(self.vm, "wait_ready") as wait, \
             mock.patch.object(deb_vm.time, "sleep"):
            self.vm.reboot()
        wait.assert_called_once_with(deb_vm.BOOT_TIMEOUT)

    def test_stop_requests_poweroff_and_closes_log(self):
        self.vm.process = mock.Mock()
        self.vm.process.poll.return_value = None
        self.vm.qemu_log = (self.work / "qemu.log").open("wb")
        with mock.patch.object(deb_vm.subprocess, "run", return_value=subprocess.CompletedProcess([], 0)):
            self.vm.stop()
        self.vm.process.wait.assert_called_once_with(timeout=30)
        self.assertTrue(self.vm.qemu_log.closed)

    def test_copy_uses_uppercase_scp_port_option(self):
        source = self.work / "candidate.deb"
        source.touch()
        with mock.patch.object(deb_vm.subprocess, "run") as run:
            self.vm.copy_to(source, "/tmp/candidate.deb")
        command = run.call_args.args[0]
        self.assertIn("-P", command)
        self.assertNotIn("-p", command)

    def test_sensitive_vm_files_are_removed(self):
        for path in (
            self.vm.key, self.vm.key.with_suffix(".pub"), self.vm.seed, self.vm.overlay,
            self.work / "user-data", self.work / "meta-data",
        ):
            path.touch()
        self.vm.cleanup_sensitive()
        self.assertFalse(any(path.exists() for path in self.work.iterdir()))


class MainFlowTests(unittest.TestCase):
    def test_main_runs_every_phase_and_records_success(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            package = root / "candidate.deb"
            package.write_bytes(b"deb")
            output = root / "output"
            events = []

            class FakeVM:
                def __init__(self, *_args): pass
                def prepare(self): events.append("prepare")
                def start(self): events.append("start")
                def copy_to(self, _source, remote): events.append(f"copy:{remote}")
                def run_phase(self, phase, _target): events.append(phase)
                def reboot(self): events.append("reboot")
                def copy_from(self, *_args): events.append("logs")
                def stop(self): events.append("stop")
                def cleanup_sensitive(self): events.append("cleanup")

            image = deb_vm.Image("https://example.invalid/pinned.qcow2", "sha256", "0" * 64)
            with mock.patch.object(deb_vm, "resolve_tools", return_value={}), \
                 mock.patch.object(deb_vm, "load_manifest", return_value={"debian12/amd64": image}), \
                 mock.patch.object(deb_vm, "ensure_image", return_value=root / "base.qcow2"), \
                 mock.patch.object(deb_vm, "VM", FakeVM):
                result = deb_vm.main([
                    "--target", "debian12", "--arch", "amd64", "--package", str(package),
                    "--output", str(output),
                ])
            self.assertEqual(result, 0)
            staged = [
                "copy:/tmp/dante-server.deb",
                "copy:/tmp/deb_upgrade.sh",
                "copy:/tmp/deb_probe.py",
            ]
            self.assertEqual(
                events[:-3],
                ["prepare", "start", *staged, "upgrade", "reboot", *staged,
                 "post-reboot", "rollback", "clean-install"],
            )
            for artifact in staged:
                self.assertEqual(events.count(artifact), 2)
            report = json.loads((output / "debian12-amd64/result.json").read_text())
            self.assertEqual(report["status"], "passed")
            self.assertEqual(events[-2:], ["stop", "cleanup"])


if __name__ == "__main__":
    unittest.main()
