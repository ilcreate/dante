#!/usr/bin/env python3
"""Require JSON for every logger line after activation, including signals."""
import argparse
from contextlib import contextmanager
import json
import os
from pathlib import Path
import pwd
import resource
import signal
import socket
import subprocess
import tempfile
import time
import unittest

from logformat_iolog_integration_test import signal_group


def no_core():
    resource.setrlimit(resource.RLIMIT_CORE, (0, 0))


class SpecialIntegrationTest(unittest.TestCase):
    def config(self, port, fmt="json"):
        username = pwd.getpwuid(os.getuid()).pw_name
        return f"""logformat: {fmt}
logoutput: stderr
internal: 127.0.0.1 port = {port}
external: 127.0.0.1
clientmethod: none
socksmethod: none
user.privileged: {username}
user.unprivileged: {username}
client pass {{
 from: 127.0.0.1/32 to: 0.0.0.0/0
 log: connect disconnect error
}}
socks pass {{
 from: 127.0.0.1/32 to: 127.0.0.0/8
 log: connect disconnect error
}}
"""

    def json_tail(self, text):
        lines = text.splitlines()
        start = next((i for i, line in enumerate(lines) if line.startswith("{")), None)
        self.assertIsNotNone(start, text[-5000:])
        records = []
        for line in lines[start:]:
            self.assertTrue(line.startswith("{"), line)
            value = json.loads(line)
            self.assertEqual(value["schema_version"], 1)
            self.assertIs(type(value["truncated"]), bool)
            records.append(value)
        return records

    def wait_text(self, proc, path, marker, offset=0):
        deadline = time.monotonic() + 10
        while time.monotonic() < deadline:
            output = path.read_text()[offset:]
            if marker in output:
                return
            self.assertIsNone(proc.poll(), output[-5000:])
            time.sleep(0.05)
        self.fail(f"missing {marker!r}:\n{output[-5000:]}")

    @contextmanager
    def daemon(self, debug=1):
        with tempfile.TemporaryDirectory(prefix="dante-special-", dir="/tmp") as tmp:
            root = Path(tmp)
            with socket.socket() as reserve:
                reserve.bind(("127.0.0.1", 0))
                port = reserve.getsockname()[1]
            config = root / "sockd.conf"
            config.write_text(self.config(port))
            path = root / "daemon.log"
            with path.open("w") as output:
                proc = subprocess.Popen([BINARY, "-d", str(debug), "-f", str(config),
                                         "-p", str(root / "pid")],
                                        stdout=output, stderr=output,
                                        start_new_session=True, preexec_fn=no_core)
                try:
                    self.wait_text(proc, path, " running")
                    yield proc, path, config, port
                finally:
                    signal_group(proc.pid, signal.SIGTERM)
                    try:
                        proc.wait(timeout=5)
                    except subprocess.TimeoutExpired:
                        signal_group(proc.pid, signal.SIGKILL)
                        proc.wait(timeout=5)
                    signal_group(proc.pid, signal.SIGKILL)

    def test_start_info_reload_and_shutdown_are_json(self):
        with self.daemon() as (proc, path, config, port):
            # Create a live negotiate worker; info is forwarded by the mother.
            with socket.create_connection(("127.0.0.1", port), timeout=5) as client:
                client.sendall(b"\x05\x01\x00")
                self.assertEqual(client.recv(2), b"\x05\x00")
                offset = len(path.read_text())
                proc.send_signal(signal.SIGUSR1)
                self.wait_text(proc, path, "negotiators (", offset)
                offset = len(path.read_text())
                # Keep JSON enabled: every reload diagnostic must stay JSON.
                config.write_text(self.config(port))
                proc.send_signal(signal.SIGHUP)
                self.wait_text(proc, path, "logformat: json", offset)
            proc.send_signal(signal.SIGTERM)
            self.assertEqual(proc.wait(timeout=10), 0)
            records = self.json_tail(path.read_text())
            self.assertTrue(any("shutting down" in e["message"] for e in records))
            self.assertTrue(any("signal" in e["message"] for e in records))
            self.assertTrue(any(e["process"] != "mother" for e in records))

    def test_unexpected_signal_uses_json_before_termination(self):
        with self.daemon() as (proc, path, _, _):
            proc.send_signal(signal.SIGSEGV)
            self.assertEqual(proc.wait(timeout=10), -signal.SIGSEGV)
            records = self.json_tail(path.read_text())
            fatal = [e for e in records if "terminating on unexpected signal" in e["message"]]
            self.assertEqual(len(fatal), 1)
            self.assertEqual(fatal[0]["level"], "warning")

    def test_fatal_signal_flushes_live_debug_ring(self):
        if not LIVEDEBUG:
            self.skipTest("requires a --enable-livedebug build and --livedebug")
        with self.daemon(debug=0) as (proc, path, _, _):
            proc.send_signal(signal.SIGSEGV)
            self.assertEqual(proc.wait(timeout=10), -signal.SIGSEGV)
            records = self.json_tail(path.read_text())
            dumps = [e for e in records if e["message"].startswith("socks_flushrb():")]
            self.assertEqual(len(dumps), 1)
            self.assertIn("flushing log buffer", dumps[0]["message"])
            self.assertIn("\n", dumps[0]["message"])

    def test_parser_fatals_before_and_after_activation(self):
        with tempfile.TemporaryDirectory(prefix="dante-parser-", dir="/tmp") as tmp:
            path = Path(tmp) / "sockd.conf"
            for text, json_expected in (("unknownkeyword: 1\n", False),
                    ("logformat: json\nunknownkeyword: 1\n", True),
                    ("logformat: json\nlogformat: json\n", True),
                    ("logformat: json\nlogoutput: /nonexistent/dante/log\n", True)):
                with self.subTest(config=text):
                    path.write_text(text)
                    result = subprocess.run([BINARY, "-V", "-f", str(path)],
                                            capture_output=True, text=True, timeout=10)
                    self.assertNotEqual(result.returncode, 0, result.stderr)
                    if json_expected:
                        records = self.json_tail(result.stderr)
                        self.assertTrue(any(e["level"] in ("warning", "error", "alert")
                                            for e in records))
                    else:
                        self.assertFalse(any(line.startswith("{")
                                             for line in result.stderr.splitlines()))


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("binary", type=Path)
    parser.add_argument("--livedebug", action="store_true")
    args, remaining = parser.parse_known_args()
    BINARY = str(args.binary.resolve())
    LIVEDEBUG = args.livedebug
    unittest.main(argv=[__file__] + remaining)
