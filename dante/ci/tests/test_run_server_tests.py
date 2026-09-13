import os
import shutil
import subprocess
import tempfile
import unittest
from pathlib import Path


SCRIPT = Path(__file__).parents[1] / "run-server-tests.sh"


class ServerTestRunnerTests(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.addCleanup(self.tmp.cleanup)
        self.root = Path(self.tmp.name)
        project = self.root / "project"
        (project / "ci").mkdir(parents=True)
        self.script = project / "ci/run-server-tests.sh"
        shutil.copyfile(SCRIPT, self.script)
        for name in (
                "configure", "include/logjson.h", "lib/config_parse.c",
                "lib/config_scan.c", "lib/logjson.c",
                "sockd/tests/logformat-tests.mk",
                "sockd/tests/logformat_runtime_integration_test.py"):
            path = project / name
            path.parent.mkdir(parents=True, exist_ok=True)
            path.touch()
        self.commands = self.root / "commands"
        bindir = self.root / "bin"
        bindir.mkdir()
        command = "#!/bin/sh\nprintf '%s' \"$(basename \"$0\")\" >> \"$COMMAND_LOG\"\nprintf ' <%s>' \"$@\" >> \"$COMMAND_LOG\"\nprintf '\\n' >> \"$COMMAND_LOG\"\n"
        for name in ("make", "bash", "python3"):
            path = bindir / name
            path.write_text(command)
            path.chmod(0o755)
        self.environment = dict(os.environ, COMMAND_LOG=str(self.commands),
                                PATH=str(bindir) + os.pathsep + os.environ["PATH"])

    def run_script(self, *arguments):
        subprocess.run(["/bin/bash", self.script, *arguments],
                       env=self.environment, check=True)
        return self.commands.read_text().splitlines()

    def test_default_runs_all_server_logging_stats_and_smoke(self):
        lines = self.run_script()
        self.assertEqual(len(lines), 3)
        make = lines[0]
        for target in (
                "check-logformat", "check-logformat-logger", "check-logjson",
                "check-logformat-iolog", "check-logformat-iolog-integration",
                "check-logformat-session", "check-logformat-session-integration",
                "check-logformat-special", "check-logformat-diagnostic",
                "check-logformat-special-integration", "check-logformat-integration"):
            self.assertIn(f" <{target}>", make)
        self.assertEqual(lines[1], "bash <sockd/tests/run-stats-api-tests.sh>")
        self.assertEqual(lines[2], "python3 <ci/smoke.py> <./sockd/sockd> <--relay>")

    def test_livedebug_executes_non_skipping_ring_test(self):
        lines = self.run_script("--livedebug")
        self.assertEqual(lines[-1],
                         "python3 <sockd/tests/logformat_special_integration_test.py>"
                         " <./sockd/sockd> <--livedebug> <-v>")

    def test_missing_generated_source_blocks_suite(self):
        (self.script.parents[1] / "lib/config_parse.c").unlink()
        result = subprocess.run(["/bin/bash", self.script], env=self.environment,
                                capture_output=True, text=True)
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("release source is missing lib/config_parse.c", result.stderr)
        self.assertFalse(self.commands.exists())


if __name__ == "__main__":
    unittest.main()
