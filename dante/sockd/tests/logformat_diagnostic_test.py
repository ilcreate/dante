#!/usr/bin/env python3
"""Check real warning/fatal diagnostics and flex input errors in subprocesses."""
import json
from pathlib import Path
import subprocess
import sys
import unittest

BINARY = str(Path(sys.argv.pop(1)).resolve())
MESSAGE = 'diagnostic quote" slash\\ line\nnext'


class DiagnosticTest(unittest.TestCase):
    def run_case(self, case, fmt="json"):
        result = subprocess.run([BINARY, fmt, case], capture_output=True,
                                text=True, timeout=10)
        self.assertEqual(result.stdout, "")
        return result

    def records(self, result):
        self.assertTrue(result.stderr.startswith("{"), result.stderr)
        values = [json.loads(line) for line in result.stderr.splitlines()]
        self.assertTrue(values, result.stderr)
        for value in values:
            self.assertEqual(value["event"], "message")
            self.assertEqual(value["process"], "io")
            self.assertFalse(value["truncated"])
        return values

    def test_warnings_and_errno(self):
        result = self.run_case("warnings")
        self.assertEqual(result.returncode, 0, result.stderr)
        records = self.records(result)
        self.assertEqual(len(records), 2)
        self.assertEqual(records[0]["level"], "warning")
        self.assertTrue(records[0]["message"].startswith(MESSAGE + ": "))
        self.assertEqual(records[1]["message"], MESSAGE)

    def test_fatal_diagnostics_and_fallback(self):
        for case in ("serr", "serrx", "serrfallback", "serrxfallback"):
            with self.subTest(case=case):
                result = self.run_case(case)
                self.assertEqual(result.returncode, 1, result.stderr)
                records = self.records(result)
                self.assertEqual(records[0]["level"], "error")
                self.assertTrue(records[0]["message"].startswith(MESSAGE))

    def test_scanner_fatal_uses_logger(self):
        for case in ("scanner", "scannerfallback"):
            with self.subTest(case=case):
                result = self.run_case(case)
                self.assertEqual(result.returncode, 2, result.stderr)
                records = self.records(result)
                self.assertEqual(len(records), 1)
                self.assertEqual(records[0]["level"], "error")
                self.assertIn("input in flex scanner failed", records[0]["message"])

    def test_raw_diagnostics_remain_text(self):
        for case, code in (("warnings", 0), ("serr", 1), ("serrx", 1),
                           ("scanner", 2)):
            with self.subTest(case=case):
                result = self.run_case(case, "raw")
                self.assertEqual(result.returncode, code, result.stderr)
                self.assertFalse(result.stderr.startswith("{"))
                self.assertIn("input in flex scanner failed" if case == "scanner"
                              else MESSAGE, result.stderr)


if __name__ == "__main__":
    unittest.main()
