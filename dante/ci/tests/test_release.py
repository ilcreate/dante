import importlib.util
import json
import tempfile
import unittest
from unittest import mock
from pathlib import Path

spec = importlib.util.spec_from_file_location("release", Path(__file__).parents[1] / "release.py")
release = importlib.util.module_from_spec(spec)
spec.loader.exec_module(release)


class ReleaseAssetsTests(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.addCleanup(self.tmp.cleanup)
        self.root = Path(self.tmp.name)
        self.metadata = {"version": "1.4.4", "revision": "1", "tag": "v1.4.4-1",
                         "package_version": "1.4.4-1", "commit": "a" * 40,
                         "source": "dante-1.4.4-1.tar.gz"}
        (self.root / "build-metadata.json").write_text(json.dumps(self.metadata))
        (self.root / self.metadata["source"]).write_bytes(b"test source")
        for arch, rpm_arch in [("amd64", "x86_64"), ("arm64", "aarch64")]:
            artifacts = {
                f"dante-server_1.4.4-1+ubuntu22.04_{arch}.deb": "1.4.4-1+ubuntu22.04",
                f"dante-server_1.4.4-1+debian12_{arch}.deb": "1.4.4-1+debian12",
                f"dante-server-1.4.4-1.el9.{rpm_arch}.rpm": "1.4.4-1",
                f"dante-server-1.4.4-1-macos15-{arch}.pkg": "1.4.4-1",
            }
            for name, package_version in artifacts.items():
                (self.root / name).write_bytes(b"test package")
                info = dict(self.metadata, package_version=package_version)
                if name.endswith(".deb"):
                    target = "ubuntu22.04" if "+ubuntu22.04_" in name else "debian12"
                    info.update({
                        "base_package_version": self.metadata["package_version"],
                        "format": "deb",
                        "target": target,
                        "architecture": arch,
                    })
                (self.root / (name + ".build-info.json")).write_text(json.dumps(info))

    def test_complete_matrix_is_accepted(self):
        _, assets = release.verify_assets(self.root)
        self.assertEqual(len(assets), 18)

    def test_missing_architecture_blocks_release(self):
        (self.root / "dante-server-1.4.4-1.el9.aarch64.rpm").unlink()
        with self.assertRaisesRegex(ValueError, "missing"):
            release.verify_assets(self.root)

    def test_empty_package_blocks_release(self):
        (self.root / "dante-server-1.4.4-1.el9.aarch64.rpm").write_bytes(b"")
        with self.assertRaisesRegex(ValueError, "empty"):
            release.verify_assets(self.root)

    def test_mixed_commits_block_release(self):
        other = dict(self.metadata, commit="b" * 40)
        next(self.root.glob("*.build-info.json")).write_text(json.dumps(other))
        with self.assertRaisesRegex(ValueError, "inconsistent commit"):
            release.verify_assets(self.root)

    def test_extra_files_block_release(self):
        (self.root / "secret.env").write_text("not a release asset")
        with self.assertRaisesRegex(ValueError, "extra"):
            release.verify_assets(self.root)

    def test_preview_cannot_be_published(self):
        self.metadata["tag"] = ""
        (self.root / "build-metadata.json").write_text(json.dumps(self.metadata))
        with self.assertRaisesRegex(ValueError, "preview"):
            release.verify_assets(self.root)

    def test_published_release_cannot_be_overwritten(self):
        environment = {"GITHUB_SHA": self.metadata["commit"],
                       "GITHUB_REF_NAME": self.metadata["tag"],
                       "GITHUB_REPOSITORY": "example/dante"}
        with mock.patch.dict("os.environ", environment), \
             mock.patch("sys.argv", ["release.py", str(self.root)]), \
             mock.patch.object(release.subprocess, "check_output", return_value='["v1.4.4-1",false]\n'), \
             mock.patch.object(release.subprocess, "run") as write:
            with self.assertRaisesRegex(RuntimeError, "published release"):
                release.main()
            write.assert_not_called()

    def test_new_release_is_draft_then_uploads_complete_set(self):
        environment = {"GITHUB_SHA": self.metadata["commit"],
                       "GITHUB_REF_NAME": self.metadata["tag"],
                       "GITHUB_REPOSITORY": "example/dante"}
        with mock.patch.dict("os.environ", environment), \
             mock.patch("sys.argv", ["release.py", str(self.root)]), \
             mock.patch.object(release.subprocess, "check_output", return_value=""), \
             mock.patch.object(release.subprocess, "run") as write:
            release.main()
            self.assertEqual(write.call_count, 2)
            self.assertIn("--draft", write.call_args_list[0].args[0])
            self.assertIn("--verify-tag", write.call_args_list[0].args[0])
            self.assertEqual(write.call_args_list[1].args[0][1:3], ["release", "upload"])
            self.assertEqual(len((self.root / "SHA256SUMS").read_text().splitlines()), 18)

    def test_target_package_version_mismatch_blocks_release(self):
        name = "dante-server_1.4.4-1+ubuntu22.04_amd64.deb.build-info.json"
        (self.root / name).write_text(json.dumps(self.metadata))
        with self.assertRaisesRegex(ValueError, "package_version"):
            release.verify_assets(self.root)

    def test_deb_requires_explicit_base_package_version(self):
        name = "dante-server_1.4.4-1+ubuntu22.04_amd64.deb.build-info.json"
        info = json.loads((self.root / name).read_text())
        del info["base_package_version"]
        (self.root / name).write_text(json.dumps(info))
        with self.assertRaisesRegex(ValueError, "base_package_version"):
            release.verify_assets(self.root)

    def test_tampered_deb_identity_blocks_release(self):
        name = "dante-server_1.4.4-1+ubuntu22.04_amd64.deb.build-info.json"
        original = json.loads((self.root / name).read_text())
        for key, bad_value in [
            ("format", "rpm"),
            ("target", "debian12"),
            ("architecture", "arm64"),
        ]:
            with self.subTest(key=key):
                info = dict(original, **{key: bad_value})
                (self.root / name).write_text(json.dumps(info))
                with self.assertRaisesRegex(ValueError, key):
                    release.verify_assets(self.root)
        (self.root / name).write_text(json.dumps(original))


if __name__ == "__main__":
    unittest.main()
