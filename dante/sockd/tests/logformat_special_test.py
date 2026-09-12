#!/usr/bin/env python3
"""Signal safety, routing and ring retention through the production logger."""
import json
import os
import subprocess
import sys
import tempfile
import unittest

BINARY = os.path.abspath(sys.argv.pop(1))


class SpecialLoggerTest(unittest.TestCase):
    def run_case(self, case, fmt="json", regular=False):
        if regular:
            with tempfile.TemporaryFile() as output:
                p = subprocess.run([BINARY, fmt, case], stdout=output,
                                   stderr=subprocess.PIPE, timeout=10)
                output.seek(0)
                data = output.read()
        else:
            p = subprocess.run([BINARY, fmt, case], capture_output=True, timeout=10)
            data = p.stdout
        self.assertEqual(p.returncode, 0, p.stderr)
        return data, p.stderr

    def objects(self, data):
        return [json.loads(line) for line in data.splitlines()]

    def test_signal_escaping_and_errno(self):
        out, err = self.run_case("signal")
        self.assertEqual(err, b"")
        event, = self.objects(out)
        self.assertEqual(event["message"], 'quote" slash\\\ntab\t é😀 ÿÀ\x80')
        self.assertEqual(event["level"], "warning")
        self.assertEqual(event["timestamp"], "1789257600.123456")
        self.assertEqual(event["pid"], 1234)
        self.assertFalse(event["truncated"])

    def test_signal_write_failures_and_short_writes(self):
        normal, _ = self.run_case("signal")
        retried, err = self.run_case("eintr-short")
        self.assertEqual(retried, normal)
        self.assertEqual(err, b"")
        out, err = self.run_case("write-fail")
        self.assertEqual((out, err), (b"", b""))

    def test_empty_signal_message(self):
        out, _ = self.run_case("empty")
        event, = self.objects(out)
        self.assertEqual(event["message"], "")
        self.assertFalse(event["truncated"])

    def test_signal_long_is_complete_bounded_object(self):
        for regular in (False, True):
            out, _ = self.run_case("long", regular=regular)
            event, = self.objects(out)
            self.assertTrue(event["truncated"])
            self.assertTrue(event["message"].startswith("xxx"))
            self.assertLessEqual(len(out), 16384 if regular else os.pathconf("/tmp", "PC_PIPE_BUF"))

    def test_signal_routing_and_fallback(self):
        out, err = self.run_case("routing")
        self.assertEqual(out, err)
        self.objects(out)
        out, err = self.run_case("fallback")
        self.assertEqual(out, b"")
        self.objects(err)

    def test_signal_syslog_critical_exception(self):
        out, err = self.run_case("syslog")
        self.assertEqual(err, b"")
        lines = out.splitlines()
        self.assertEqual(len(lines), 2)
        for line, facility in zip(lines, (17 << 3, 16 << 3)):
            prefix, level, payload = line.split(b" ", 2)
            self.assertEqual(prefix, b"SYSLOG")
            self.assertEqual(int(level), facility | 2)
            self.assertEqual(json.loads(payload)["level"], "critical")

    def test_ring_dump_is_one_event_and_does_not_mutate_ring(self):
        out, err = self.run_case("ring", regular=True)
        self.assertEqual(err, b"")
        first, second = self.objects(out)
        self.assertEqual(first, second)
        self.assertIn("suppressed ordinary", first["message"])
        self.assertIn("first\\n", first["message"])
        self.assertIn("flushing log buffer", first["message"])
        self.assertLess(first["message"].index("suppressed ordinary"),
                        first["message"].index("first"))

    def test_ring_wrap_truncates_validly(self):
        out, _ = self.run_case("ring-wrap", regular=True)
        first, second = self.objects(out)
        self.assertEqual(first, second)
        self.assertTrue(first["truncated"])
        self.assertNotIn("old-0\\n", first["message"])

    def test_signal_messages_are_retained_in_ring(self):
        out, _ = self.run_case("ring-emitted", regular=True)
        ordinary, signal, dump = self.objects(out)
        self.assertNotIn(ordinary["message"], dump["message"])
        self.assertIn("first\\n", dump["message"])

    def test_stack_ordinary_and_signal_context(self):
        features, _ = self.run_case("features")
        if features.strip() != b"1":
            self.skipTest("build has no backtrace support")
        out, _ = self.run_case("stack", regular=True)
        records = self.objects(out)
        self.assertTrue(records)
        self.assertTrue(all("stackframe #" in e["message"] for e in records))
        out, _ = self.run_case("stack-signal")
        records = self.objects(out)
        self.assertTrue(records)
        for event in records:
            self.assertIn("signal", event["message"])

    def test_raw_ring_retains_existing_multiline_layout(self):
        out, err = self.run_case("ring", "raw", regular=True)
        self.assertEqual(err, b"")
        self.assertEqual(out.count(b"flushing log buffer"), 2)
        self.assertEqual(out.count(b"suppressed ordinary\n"), 2)
        self.assertEqual(out.count(b'first\nsecond"\n'), 2)
        self.assertEqual(out.count(b'\n"""'), 4)

    def test_raw_signal_remains_text(self):
        out, err = self.run_case("signal", "raw")
        self.assertEqual(err, b"")
        self.assertIn(b'(1789257600.123456) danted[1234]: warning: quote" slash\\\n', out)
        self.assertTrue(out.endswith(b"\xff\xc0\x80\n"))


if __name__ == "__main__":
    unittest.main()
