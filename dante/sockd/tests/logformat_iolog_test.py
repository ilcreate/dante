#!/usr/bin/env python3
"""Check typed iolog events through the production formatter and serializer."""
import json
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

BINARY = str(Path(sys.argv.pop(1)).resolve())


class IologTest(unittest.TestCase):
    def run_case(self, case, format="json"):
        with tempfile.TemporaryFile() as output:
            p = subprocess.run([BINARY, format, case], stdout=output,
                               stderr=subprocess.PIPE, timeout=15)
            self.assertEqual(p.returncode, 0, p.stderr)
            output.seek(0)
            return output.read()

    def events(self, case):
        return [json.loads(line) for line in self.run_case(case).splitlines()]

    def test_operations_and_rule(self):
        events = self.events("all")
        self.assertEqual([e["event"] for e in events], ["accept", "block", "temporary_block",
            "connect", "disconnect", "error", "temporary_error", "hostid", "io"])
        for e in events:
            self.assertEqual(e["rule"], {"number": 7, "type": "socks-rule"})
            self.assertEqual(e["protocol"], "tcp")
            self.assertEqual(e["command"], "connect")
            self.assertEqual(e["verdict"], "block" if e["event"] in ("block", "temporary_block") else "pass")
            self.assertNotIn("error", e)  # Explicit diagnostic is not proof of current errno.
            self.assertEqual(e["tcp_info"], 'first\nsecond "info"\\')
            if e["event"] != "io":
                self.assertEqual(e["detail"], 'detail "quoted"\nsecond line')
                self.assertNotIn("payload", e)

    def test_endpoints_proxies_and_authentication(self):
        e = self.events("all")[0]
        self.assertEqual(e["source"]["local"], dict(type="ipv4", address="127.0.0.1", port=1080))
        self.assertEqual(e["source"]["peer"], dict(type="ipv4", address="198.51.100.1", port=12345))
        self.assertEqual(e["source"]["authentication"], dict(method="username", user='alice"\\'))
        self.assertEqual(e["destination"]["local"], dict(type="ipv6", address="fe80::1", port=0, scope_id=3))
        self.assertEqual(e["destination"]["peer"], dict(type="domain", address="target.example", port=443))
        self.assertEqual(e["destination"]["authentication"], dict(method="none"))
        self.assertEqual(e["source_proxy"]["peer"]["address"], "192.0.2.1")
        self.assertEqual(e["destination_proxy"]["local"]["port"], 1082)
        self.assertNotIn("never-log-this-password", json.dumps(e))

    def test_unknown_and_zero(self):
        e, = self.events("missing")
        self.assertEqual(e["rule"]["number"], 0)
        for field in ("source", "destination", "source_proxy", "destination_proxy", "command", "detail"):
            self.assertNotIn(field, e)

    def test_ipv6_zero_scope_is_preserved(self):
        e = self.events("scopezero")[0]
        self.assertEqual(e["destination"]["local"]["scope_id"], 0)

    def test_known_error(self):
        import errno
        e, = self.events("error")
        self.assertEqual(e["error"], {"code": errno.ECONNREFUSED})
        self.assertEqual(e["event"], "error")
        self.assertEqual(e["verdict"], "pass")

    def test_payload_gating_and_zero(self):
        e, = self.events("payload")
        self.assertEqual(e["payload"], 'quote"\\\n\0ÿé')
        self.assertEqual(e["io_bytes"], 12)
        e, = self.events("ioonly")
        self.assertNotIn("payload", e)
        self.assertNotIn('quote', e["message"])
        self.assertEqual(e["io_bytes"], 12)
        e, = self.events("empty-payload")
        self.assertEqual(e["payload"], "")
        self.assertEqual(e["io_bytes"], 0)

    def test_filter_matrix_matches_raw(self):
        events = self.events("matrix")
        self.assertEqual(len(events), 18)
        raw = self.run_case("matrix", "raw").decode()
        # Remove exactly the stable raw prefix; message can itself be multiline.
        import re
        bodies = re.split(r"[^\n]*\(1789257600\.123456\) danted\[1234\]: info: ", raw)[1:]
        self.assertEqual([e["message"] for e in events], [s.removesuffix("\n") for s in bodies])
        self.assertTrue(all("tcp_info" not in e for e in events))

    def test_protocol_and_command(self):
        for case, protocol, command in (("udp", "udp", "udpassociate"), ("bind", "tcp", "bind")):
            for e in self.events(case):
                self.assertEqual((e["protocol"], e["command"]), (protocol, command))

    def test_long_payload_and_allocation_failure(self):
        for case in ("long", "alloc"):
            events = self.events(case)
            e = [e for e in events if e["event"] == "io"][0]
            self.assertTrue(e["truncated"])
            self.assertEqual(e["io_bytes"], 100000)
            self.assertLessEqual(len(self.run_case(case)), 65535 * 2)


if __name__ == "__main__":
    unittest.main()
