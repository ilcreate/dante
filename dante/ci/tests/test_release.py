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
            for name in [f"dante-server_1.4.4-1_debian12_{arch}.deb",
                         f"dante-server-1.4.4-1.el9.{rpm_arch}.rpm",
                         f"dante-server-1.4.4-1-macos15-{arch}.pkg"]:
                (self.root / name).write_bytes(b"test package")
                (self.root / (name + ".build-info.json")).write_text(json.dumps(self.metadata))

    def test_complete_matrix_is_accepted(self):
        _, assets = release.verify_assets(self.root)
        self.assertEqual(len(assets), 14)

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
            self.assertEqual(len((self.root / "SHA256SUMS").read_text().splitlines()), 14)


if __name__ == "__main__":
    unittest.main()
