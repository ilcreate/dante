import re
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[3]
DEBIAN = ROOT / "debian"


def read_debian_file(name):
    return (DEBIAN / name).read_text(encoding="utf-8")


class DebianPackagingContractTests(unittest.TestCase):
    """Static contracts that must hold before dpkg-buildpackage is invoked."""

    def test_uses_modern_debhelper_and_mandatory_auth_build_dependencies(self):
        control = read_debian_file("control")

        self.assertRegex(control, r"(?im)^Build-Depends:.*debhelper-compat\s*\(=\s*12\)")
        self.assertRegex(control, r"(?im)^Build-Depends:(?:[^\n]|\n[ \t])+\blibpam0g-dev\b")
        self.assertRegex(control, r"(?im)^Build-Depends:(?:[^\n]|\n[ \t])+\blibwrap0-dev\b")
        self.assertFalse((DEBIAN / "compat").exists())

    def test_rules_configures_distro_paths_and_fails_closed_on_features(self):
        rules = read_debian_file("rules")

        self.assertIn("--with-socks-conf=/etc/dante.conf", rules)
        self.assertIn("--with-sockd-conf=/etc/danted.conf", rules)
        self.assertRegex(rules, r"--with-pidfile=/(?:var/)?run/danted\.pid")
        self.assertIn("--without-gssapi", rules)
        self.assertNotIn("--without-pam", rules)
        self.assertNotIn("--without-libwrap", rules)
        configure_block = rules.split("override_dh_auto_configure:", 1)[1].split(
            "\noverride_", 1
        )[0]
        self.assertRegex(configure_block, r"(?m)^\s*grep.*HAVE_COND_PAM")
        self.assertRegex(configure_block, r"(?m)^\s*grep.*HAVE_COND_LIBWRAP")

    def test_server_payload_uses_the_distro_names_only(self):
        install = read_debian_file("dante-server.install")

        self.assertRegex(install, r"(?m)^usr/sbin/danted\s+usr/sbin$")
        self.assertRegex(install, r"(?m)^usr/share/man/man8/danted\.8\s+usr/share/man/man8$")
        self.assertRegex(install, r"(?m)^usr/share/man/man5/danted\.conf\.5\s+usr/share/man/man5$")
        self.assertNotRegex(install, r"(?m)(?:^|/)sockd(?:\.service|\.conf|\.8)?(?:\s|$)")

        service = read_debian_file("dante-server.danted.service")
        self.assertIn("ExecStart=/usr/sbin/danted", service)
        self.assertIn("Documentation=man:danted(8) man:danted.conf(5)", service)
        self.assertNotIn("/usr/sbin/sockd", service)
        self.assertNotIn("/etc/sockd.conf", service)
        self.assertNotRegex(service, r"(?m)^ExecStart=.*(?:^|\s)-S(?:\s|$)")

    def test_server_keeps_distribution_config_and_native_service_helpers(self):
        rules = read_debian_file("rules")
        server_config = read_debian_file("dante-server.install")

        self.assertRegex(server_config, r"(?m)^example/renamed/danted\.conf\s+etc$")
        self.assertRegex(rules, r"\bdh_installinit\b.*--name=danted")
        self.assertRegex(rules, r"\bdh_installsystemd\b.*--name=danted")

    def test_package_build_runs_statistics_regression_tests(self):
        rules = read_debian_file("rules")

        self.assertRegex(rules, r"(?m)^override_dh_auto_test:")
        self.assertIn("bash sockd/tests/run-stats-api-tests.sh", rules)

    def test_pam_service_documentation_keeps_the_compiled_default(self):
        config_header = (ROOT / "dante/include/config.h").read_text(encoding="latin-1")
        server_default = re.search(
            r'#if SOCKS_SERVER\s*\n#define DEFAULT_PAMSERVICENAME\s+"([^"]+)"',
            config_header,
        )
        self.assertIsNotNone(server_default)

        expected = server_default.group(1)
        server_manpage = (ROOT / "dante/doc/sockd.conf.5").read_text(encoding="latin-1")
        documented = re.findall(
            r"pam\.servicename\\fP\s*\n(?:Which|What) servicename.*Default is \"([^\"]+)\"",
            server_manpage,
        )
        self.assertEqual(documented, [expected, expected])

        rename_patch = read_debian_file("patches/rename-programs.patch")
        renamed_defaults = re.findall(
            r'^\+(?:Which|What) servicename.*Default is "([^"]+)"',
            rename_patch,
            re.MULTILINE,
        )
        self.assertTrue(all(value == expected for value in renamed_defaults))

    def test_upstream_packaging_provenance_is_machine_verifiable(self):
        provenance = read_debian_file("upstream-packaging.sha256")
        records = [line for line in provenance.splitlines() if line and not line.startswith("#")]

        self.assertGreaterEqual(len(records), 2)
        for record in records:
            self.assertRegex(record, r"^[0-9a-f]{64}\s{2}\S+$")
        self.assertIn("1.4.2+dfsg-7", provenance)
        self.assertIn("1.4.2+dfsg-7build4", provenance)


if __name__ == "__main__":
    unittest.main()
