#!/usr/bin/env python3
"""Check logformat parsing and reloads using a built sockd binary."""

import argparse
import json
import os
from pathlib import Path
import pwd
import signal
import socket
import subprocess
import tempfile
import time
import unittest


class LogformatConfigTest(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory(prefix="dante-logformat-", dir="/tmp")
        self.addCleanup(self.tmp.cleanup)
        self.root = Path(self.tmp.name)
        self.config = self.root / "sockd.conf"
        username = pwd.getpwuid(os.getuid()).pw_name
        unprivileged = "nobody" if os.getuid() == 0 else username
        with socket.socket() as reserve:
            reserve.bind(("127.0.0.1", 0))
            self.port = reserve.getsockname()[1]
        self.globals = f"""logoutput: stderr
internal: 127.0.0.1 port = {self.port}
external: 127.0.0.1
clientmethod: none
socksmethod: none
user.privileged: {username}
user.unprivileged: {unprivileged}
"""
        self.rules = """client pass {
 from: 127.0.0.1/32 to: 0.0.0.0/0
}
socks pass {
 from: 127.0.0.1/32 to: 127.0.0.0/8
}
"""

    def verify(self, text):
        self.config.write_text(text)
        return subprocess.run(
            [BINARY, "-V", "-d", "1", "-f", str(self.config)],
            capture_output=True, text=True, timeout=15,
        )

    def test_formats_and_default(self):
        for directive, expected in (("", "raw"), ("logformat: raw\n", "raw"),
                                    ("logformat: json\n", "json")):
            with self.subTest(directive=directive):
                result = self.verify(directive + self.globals + self.rules)
                self.assertEqual(result.returncode, 0, result.stderr)
                self.assertIn("logformat: " + expected, result.stderr)

    def test_json_diagnostics_after_activation(self):
        result = self.verify("logformat: json\n" + self.globals + self.rules)
        self.assertEqual(result.returncode, 0, result.stderr)
        lines = [line for line in result.stderr.splitlines() if "logformat: json" in line]
        self.assertTrue(lines, result.stderr)
        for line in lines:
            event = json.loads(line)
            self.assertEqual(event["event"], "message")
            self.assertEqual(event["schema_version"], 1)
            self.assertEqual(event["process"], "mother")
            self.assertRegex(event["timestamp"], r"^\d+\.\d{6}$")
            self.assertFalse(event["truncated"])

    def test_invalid_values(self):
        for value in ("xml", "JSON", "0", "", "json raw"):
            with self.subTest(value=value):
                result = self.verify(f"logformat: {value}\n" + self.globals + self.rules)
                self.assertNotEqual(result.returncode, 0, result.stderr)

    def test_duplicate_directives(self):
        for second in ("raw", "json"):
            with self.subTest(second=second):
                result = self.verify("logformat: json\nlogformat: " + second
                                     + "\n" + self.globals + self.rules)
                self.assertNotEqual(result.returncode, 0, result.stderr)
                self.assertIn("duplicate logformat", result.stderr)

    def test_rule_scope_is_rejected(self):
        for rule in ("client pass", "socks pass"):
            with self.subTest(rule=rule):
                result = self.verify(self.globals + rule + " {\n"
                                     "from: 127.0.0.1/32 to: 0.0.0.0/0\n"
                                     "logformat: json\n}\n")
                self.assertNotEqual(result.returncode, 0, result.stderr)

    def test_lifecycle_and_config_copy(self):
        if LIFECYCLE_BINARY is None:
            self.skipTest("use check-logformat make target to build the lifecycle test")
        self.config.write_text("logformat: json\n" + self.globals + self.rules)
        result = subprocess.run(
            [LIFECYCLE_BINARY, "-V", "-f", str(self.config)],
            capture_output=True, text=True, timeout=15,
        )
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn("logformat lifecycle and config copy tests passed", result.stdout)

    def test_client_config_rejects_logformat(self):
        if CLIENT_BINARY is None:
            self.skipTest("use check-logformat-client to test the client library")
        env = {key: value for key, value in os.environ.items()
               if not key.startswith("SOCKS_")}
        env["SOCKS_CONF"] = str(self.config)
        for directive, accepted in (("", True), ("logformat: raw\n", False),
                                     ("logformat: json\n", False)):
            with self.subTest(directive=directive):
                self.config.write_text(directive + "logoutput: stderr\n")
                result = subprocess.run([CLIENT_BINARY], env=env, capture_output=True,
                                        text=True, timeout=15)
                if accepted:
                    self.assertEqual(result.returncode, 0, result.stderr)
                    self.assertIn("client config parsed", result.stdout)
                else:
                    self.assertNotEqual(result.returncode, 0, result.stderr)

    def test_sighup_resets_seen_flag_and_defaults(self):
        self.config.write_text("logformat: json\n" + self.globals + self.rules)
        logpath = self.root / "daemon.log"
        with logpath.open("w") as log:
            daemon = subprocess.Popen(
                [BINARY, "-d", "1", "-f", str(self.config),
                 "-p", str(self.root / "pid")],
                stdout=log, stderr=log, start_new_session=True,
            )
            try:
                self.wait_for_log(daemon, logpath, "logformat: json", 0)
                self.wait_for_log(daemon, logpath, " running", 0)
                for directive, expected in (("json", "json"), ("raw", "raw"),
                                            ("json", "json"), (None, "raw")):
                    offset = logpath.stat().st_size
                    prefix = "" if directive is None else f"logformat: {directive}\n"
                    self.config.write_text(prefix + self.globals + self.rules)
                    daemon.send_signal(signal.SIGHUP)
                    self.wait_for_log(daemon, logpath, "logformat: " + expected, offset)
            finally:
                if daemon.poll() is None:
                    os.killpg(daemon.pid, signal.SIGTERM)
                try:
                    daemon.wait(timeout=10)
                except subprocess.TimeoutExpired:
                    os.killpg(daemon.pid, signal.SIGKILL)
                    daemon.wait(timeout=5)

    def wait_for_log(self, daemon, path, text, offset):
        deadline = time.monotonic() + 15
        while time.monotonic() < deadline:
            output = path.read_text(errors="replace")[offset:]
            self.assertIsNone(daemon.poll(), output[-6000:])
            if text in output:
                return
            time.sleep(0.05)
        self.fail(f"missing {text!r} in daemon log:\n{output[-6000:]}")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("binary", type=Path)
    parser.add_argument("--lifecycle-binary", type=Path)
    parser.add_argument("--client-binary", type=Path)
    args, remaining = parser.parse_known_args()
    BINARY = str(args.binary.resolve())
    LIFECYCLE_BINARY = (str(args.lifecycle_binary.resolve())
                        if args.lifecycle_binary is not None else None)
    CLIENT_BINARY = (str(args.client_binary.resolve())
                     if args.client_binary is not None else None)
    unittest.main(argv=[__file__] + remaining)
