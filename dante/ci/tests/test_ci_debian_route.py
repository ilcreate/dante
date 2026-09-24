import json
import unittest
from pathlib import Path


CI = Path(__file__).parents[1]
ROOT = CI.parents[1]


class DebianBuildRouteTests(unittest.TestCase):
    def test_debian_build_has_dedicated_native_route(self):
        script = (CI / "build-package.sh").read_text()
        self.assertIn('exec bash "$ci_dir/build-deb.sh"', script)
        native = (CI / "build-deb.sh").read_text()
        self.assertIn("dpkg-buildpackage", native)
        self.assertIn("-b -us -uc", native)
        self.assertIn('"$build_dir"/*.ddeb', native)

    def test_generic_packager_does_not_construct_deb_payload(self):
        packager = (CI / "package.py").read_text()
        self.assertNotIn('kind == "deb"', packager)
        self.assertNotIn("dpkg-deb", packager)
        self.assertNotIn("DEBIAN/postinst", packager)

    def test_workflow_keeps_diagnostics_out_of_release_assets(self):
        workflow = (ROOT / ".github/workflows/packages.yml").read_text()
        self.assertIn("dist/diagnostics/", workflow)
        self.assertIn("dist/diagnostics/**", workflow)

    def test_release_asset_names_do_not_collide(self):
        targets = json.loads((CI / "targets.json").read_text())
        names = []
        for target in targets:
            if target["format"] == "deb":
                names.append(f'dante-server_1.4.4-1+{target["target"]}_{target["arch"]}.deb')
        self.assertEqual(len(names), len(set(names)))

    def test_release_waits_for_package_build_without_vm_job(self):
        packages = (ROOT / ".github/workflows/packages.yml").read_text()
        release = (ROOT / ".github/workflows/release.yml").read_text()
        self.assertNotIn("  verify-deb:\n", packages)
        self.assertNotIn("deb_matrix", packages)
        self.assertNotIn("python3 dante/ci/deb_vm.py", packages)
        self.assertIn("  package:\n    needs: source\n", packages)
        self.assertIn("  draft:\n    needs: build\n", release)


if __name__ == "__main__":
    unittest.main()
