import importlib.util
import json
import os
import subprocess
import sys
import tarfile
import tempfile
import unittest
from pathlib import Path
from unittest import mock

spec = importlib.util.spec_from_file_location("metadata", Path(__file__).parents[1] / "metadata.py")
metadata = importlib.util.module_from_spec(spec)
spec.loader.exec_module(metadata)


class IdentityTests(unittest.TestCase):
    def test_tag_preserves_upstream_and_packaging_revision(self):
        data = metadata.identity("1.4.4", "v1.4.4-2", "9", "a" * 40)
        self.assertEqual(data["package_version"], "1.4.4-2")

    def test_preview_sorts_before_first_release(self):
        data = metadata.identity("1.4.4", "", "27", "a" * 40)
        self.assertEqual(data["revision"], "0.ci.27.gaaaaaaaaaaaa")

    def test_bad_and_mismatched_tags_fail(self):
        for tag in ["v1.4.5-1", "v1.4.4", "v1.4.4-0", "v1.4.4-01", "v1.4.4-1;id", "../../bad"]:
            with self.subTest(tag=tag), self.assertRaises(ValueError):
                metadata.identity("1.4.4", tag, "1", "a" * 40)

    def test_bad_run_number_fails(self):
        with self.assertRaises(ValueError):
            metadata.identity("1.4.4", "", "1\nextra", "a" * 40)

    def test_debian_package_version_has_target_suffix(self):
        data = metadata.identity("1.4.4", "v1.4.4-2", "9", "a" * 40)
        self.assertEqual(
            metadata.package_version_for_target(data, "ubuntu22.04"),
            "1.4.4-2+ubuntu22.04",
        )
        self.assertEqual(
            metadata.package_version_for_target(data, "debian12"),
            "1.4.4-2+debian12",
        )
        with self.assertRaisesRegex(ValueError, "unknown Debian target"):
            metadata.package_version_for_target(data, "ubuntu-latest")


class SourceExportTests(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.addCleanup(self.tmp.cleanup)
        self.root = Path(self.tmp.name) / "repository"
        (self.root / "dante").mkdir(parents=True)
        (self.root / "debian").mkdir()
        (self.root / "dante/configure.ac").write_text("version=1.4.4\n")
        (self.root / "dante/VERSION").write_text("committed source\n")
        (self.root / "dante/configure").write_text("#!/bin/sh\n")
        (self.root / "dante/configure").chmod(0o755)
        os.symlink("VERSION", self.root / "dante/VERSION.link")
        (self.root / "debian/rules").write_text("committed rules\n")
        subprocess.run(["git", "init", "-q", str(self.root)], check=True)
        subprocess.run(["git", "-C", str(self.root), "add", "dante", "debian"], check=True)
        subprocess.run(
            ["git", "-C", str(self.root), "-c", "user.name=CI", "-c", "user.email=ci@example.invalid",
             "commit", "-qm", "fixture"],
            check=True,
            cwd=self.tmp.name,
        )

    def test_export_uses_committed_upstream_and_debian_trees(self):
        # Dirty tracked and untracked packaging files must never leak into a release.
        (self.root / "dante/VERSION").write_text("dirty source\n")
        (self.root / "debian/rules").write_text("dirty rules\n")
        (self.root / "debian/local-secret").write_text("untracked\n")
        output = Path(self.tmp.name) / "output"
        with mock.patch.object(
            sys,
            "argv",
            ["metadata.py", "--repository", str(self.root), "--output", str(output),
             "--run-number", "7"],
        ):
            metadata.main()
        data = json.loads((output / "build-metadata.json").read_text())
        with tarfile.open(output / data["source"], "r:gz") as archive:
            source = archive.extractfile("dante-1.4.4/VERSION").read().decode()
            rules = archive.extractfile("dante-1.4.4/debian/rules").read().decode()
            names = archive.getnames()
            configure = archive.getmember("dante-1.4.4/configure")
            version_link = archive.getmember("dante-1.4.4/VERSION.link")
        self.assertEqual(source, "committed source\n")
        self.assertEqual(rules, "committed rules\n")
        self.assertNotIn("dante-1.4.4/debian/local-secret", names)
        self.assertEqual(configure.mode & 0o111, 0o111)
        self.assertTrue(version_link.issym())
        self.assertEqual(version_link.linkname, "VERSION")


class MatrixTests(unittest.TestCase):
    def test_pull_requests_build_all_debian_targets(self):
        targets = json.loads((Path(__file__).parents[1] / "targets.json").read_text())
        selected = metadata.select_targets(targets, full_matrix=False)
        self.assertEqual(
            {(target["target"], target["arch"]) for target in selected},
            {
                ("ubuntu22.04", "amd64"),
                ("ubuntu22.04", "arm64"),
                ("debian12", "amd64"),
                ("debian12", "arm64"),
            },
        )

    def test_debian_images_are_target_specific_and_digest_pinned(self):
        targets = json.loads((Path(__file__).parents[1] / "targets.json").read_text())
        debian = [target for target in targets if target["format"] == "deb"]
        self.assertEqual(len(debian), 4)
        for target in debian:
            with self.subTest(target=target):
                self.assertIn("@sha256:", target["image"])
                self.assertTrue(target["image"].startswith(
                    "ubuntu:22.04" if target["target"] == "ubuntu22.04" else "debian:12"
                ))


if __name__ == "__main__":
    unittest.main()
