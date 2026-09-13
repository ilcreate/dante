import importlib.util
import json
import tempfile
import unittest
from pathlib import Path


spec = importlib.util.spec_from_file_location(
    "smoke", Path(__file__).parents[1] / "smoke.py")
smoke = importlib.util.module_from_spec(spec)
spec.loader.exec_module(smoke)


def record(event, **fields):
    value = {
        "schema_version": 1,
        "timestamp": "1789257600.123456",
        "level": "info",
        "pid": 1234,
        "process": "request",
        "program": "sockd",
        "event": event,
        "message": event,
        "truncated": False,
    }
    value.update(fields)
    return value


class JsonLoggingSmokeTests(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.addCleanup(self.tmp.cleanup)
        self.path = Path(self.tmp.name) / "events.log"

    def write(self, records):
        self.path.write_text("".join(json.dumps(value) + "\n" for value in records))

    def relay_records(self):
        endpoint = lambda port: {"peer": {"type": "ipv4", "address": "127.0.0.1",
                                           "port": port}}
        return [
            record("accept", command="accept", protocol="tcp", verdict="pass",
                   rule={"number": 1, "type": "client-rule"}),
            record("connect", command="connect", protocol="tcp", verdict="pass",
                   rule={"number": 2, "type": "socks-rule"},
                   source=endpoint(41000), destination=endpoint(42000)),
            record("session_close", command="connect", protocol="tcp", verdict="pass",
                   scope="session", reason="closed", side="client",
                   source=endpoint(41000), destination=endpoint(42000),
                   client_bytes_read=19, client_bytes_written=8,
                   target_bytes_read=8, target_bytes_written=19),
            record("error", command="connect", protocol="tcp", verdict="pass",
                   rule={"number": 1, "type": "client-rule"}),
        ]

    def test_every_complete_line_must_be_json(self):
        self.path.write_text(json.dumps(record("accept")) + "\nnot-json\n")
        with self.assertRaisesRegex(AssertionError, "line 2"):
            smoke.read_json_log(self.path)

    def test_unicode_line_separator_stays_inside_json_record(self):
        expected = record("message", message="one\u2028two\u2029three\u0085four")
        self.path.write_text(json.dumps(expected, ensure_ascii=False) + "\n")
        self.assertEqual(smoke.read_json_log(self.path), [expected])

    def test_relay_and_failed_connect_events_are_required(self):
        records = self.relay_records()
        self.write(records)
        smoke.validate_json_log(self.path, relay=True, client_port=41000,
                                target_port=42000)

        self.write(records[:-1])
        with self.assertRaisesRegex(AssertionError, "structured error/connect"):
            smoke.validate_json_log(self.path, relay=True, client_port=41000,
                                    target_port=42000)

    def test_relay_byte_counters_are_checked(self):
        records = self.relay_records()
        records[2]["target_bytes_written"] = 18
        self.write(records)
        with self.assertRaisesRegex(AssertionError, "target_bytes_written"):
            smoke.validate_json_log(self.path, relay=True, client_port=41000,
                                    target_port=42000)


if __name__ == "__main__":
    unittest.main()
