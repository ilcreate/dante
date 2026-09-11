import importlib.util
import unittest
from pathlib import Path

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


if __name__ == "__main__":
    unittest.main()
