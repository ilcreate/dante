#!/usr/bin/env python3
"""Run with a compiled logjson_test driver as the only argument."""
import json
from pathlib import Path
import random
import subprocess
import sys
import unittest

DRIVER = str(Path(sys.argv.pop(1)).resolve())
COMMON = {"schema_version", "timestamp", "level", "pid", "process",
          "program", "event", "message", "truncated"}
COUNTERS = {"duration_us", "client_bytes_read", "client_bytes_written",
            "target_bytes_read", "target_bytes_written", "client_packets_read",
            "client_packets_written", "target_packets_read", "target_packets_written"}


def serialize(message=b"", capacity=65536, mode="message"):
    result = subprocess.run([DRIVER, str(capacity), mode], input=message,
                            stdout=subprocess.PIPE, check=True).stdout
    if capacity < 512:
        assert result == b""
        return None
    assert len(result) < min(capacity, 65536)
    assert result.endswith(b"\n") and result.count(b"\n") == 1
    value = json.loads(result.decode("utf-8"))
    assert COMMON <= value.keys()
    assert type(value["truncated"]) is bool
    return value


class LogJsonTest(unittest.TestCase):
    def test_connection_fields(self):
        value = serialize(b'hello\0\n"\\' + "é€😀".encode(), mode="connection")
        self.assertEqual(value["rule"], {"number": 18446744073709551615, "type": "socks"})
        self.assertEqual(value["verdict"], "pass")
        self.assertEqual(value["protocol"], "tcp")
        self.assertEqual(value["command"], "connect")
        self.assertEqual(value["source"], {
            "local": {"type": "ipv4", "address": "192.0.2.3", "port": 1080},
            "peer": {"type": "ipv6", "address": "fe80::1", "port": 0, "scope_id": 9},
            "authentication": {"method": "username", "user": 'é"\nÿ'},
            "hostids": ["192.0.2.8", 'é"\nÿ']})
        self.assertEqual(value["destination"], {
            "peer": {"type": "domain", "address": "例え.example", "port": 443}})
        self.assertEqual(value["source_proxy"], value["destination"])
        self.assertEqual(value["destination_proxy"], value["source"])
        self.assertEqual(value["error"], {"code": -123})
        self.assertEqual(value["io_bytes"], 18446744073709551615)
        self.assertEqual(value["detail"], value["message"])
        self.assertEqual(value["payload"], value["message"])
        self.assertEqual(value["tcp_info"], 'rtt: 3\nquote: "\\')
        self.assertFalse(value["truncated"])

    def test_connection_known_zero_and_unknown_omission(self):
        value = serialize(mode="connection_zero")
        self.assertEqual(value["rule"], {"number": 0})
        self.assertEqual(value["error"], {"code": 0})
        self.assertEqual(value["io_bytes"], 0)
        self.assertEqual(value["payload"], "")
        self.assertEqual(value["detail"], "")
        self.assertEqual(value["source"], {
            "peer": {"type": "ipv6", "address": "fe80::1", "port": 0, "scope_id": 0},
            "authentication": {"method": "none"}})
        self.assertEqual(set(value), COMMON | {
            "rule", "source", "error", "io_bytes", "payload", "detail"})
        self.assertFalse(value["truncated"])

    def test_connection_binary_fields(self):
        for raw in [bytes(range(256)), b"\xc0\xaf", b"\xed\xa0\x80",
                    b"\xf4\x90\x80\x80", b"\xe2\x82", b"\xf0\x90\x80",
                    b"\xc2", b"\xe0\x80\x80", b"\xf0\x80\x80\x80",
                    b"\xf0A\x80\x80", b"\xe2\0\x80", b"\xff"]:
            value = serialize(raw, mode="connection")
            self.assertEqual(value["payload"], raw.decode("latin1"))
            self.assertEqual(value["detail"], value["payload"])

    def test_connection_sparse_omits_unknown_objects_and_fields(self):
        value = serialize(mode="connection_sparse")
        self.assertEqual(set(value), COMMON | {"rule", "source"})
        self.assertEqual(value["rule"], {"number": 0})
        self.assertEqual(value["source"], {"authentication": {"user": "user-only"}})
        self.assertFalse(value["truncated"])

    def test_connection_capacity_boundaries(self):
        for mode in ["connection", "connection_pressure"]:
            full = serialize(b'hello\0\n"\\' + "é€😀".encode(), mode=mode)
            for capacity in list(range(512, 1600)) + [2048, 4096, 8192, 65536, 65560]:
                value = serialize(b'hello\0\n"\\' + "é€😀".encode(), capacity, mode)
                for key in ["level", "process", "program"]:
                    self.assertEqual(value[key], full[key])
                for key in value.keys() - COMMON:
                    self.assertEqual(value[key], full[key])
                self.assertEqual(value["truncated"], value != full)
        for capacity in [512, 513, 1024, 4096, 65536, 65560]:
            value = serialize(b"\0" * 20000, capacity, "connection")
            self.assertNotIn("detail", value)
            self.assertNotIn("payload", value)
            self.assertTrue(value["truncated"])
            self.assertTrue(("\0" * 20000).startswith(value["message"]))

    def test_common_fields_and_escaping(self):
        message = b'quote" backslash\\ tab\t CR\r LF\n NUL\0 end'
        self.assertEqual(serialize(message), {
            "schema_version": 1, "timestamp": "1789257600.123456",
            "level": "info", "pid": 1234, "process": "mother",
            "program": "danted", "event": "message",
            "message": message.decode(), "truncated": False})

    def test_all_bytes(self):
        self.assertEqual(serialize(bytes(range(256)))["message"],
                         "".join(chr(i) for i in range(256)))

    def test_unicode_and_invalid_sequences(self):
        for text in ["ASCII", "é€😀", "\u0080\u07ff\u0800\ud7ff\ue000\uffff",
                     "\U00010000\U0010ffff"]:
            self.assertEqual(serialize(text.encode())["message"], text)
        for raw in [b"\xc0\xaf", b"\xed\xa0\x80", b"\xf4\x90\x80\x80",
                    b"\xf5\x80\x80\x80", b"\xe2\x82", b"\xf0\x90\x80",
                    b"\xc2", b"\xe0\x80\x80", b"\xf0\x80\x80\x80",
                    b"\xf0A\x80\x80", b"\xe2\0\x80", b"\xff"]:
            self.assertEqual(serialize(raw)["message"], raw.decode("latin1"))

    def test_capacity_and_truncation_boundaries(self):
        for capacity in [0, 1, 100, 511]:
            serialize(b"test", capacity)
        for unit in [b"A", b'"', b"\0", "é€😀".encode(), b"\xff"]:
            expected = (unit.decode("latin1") if unit == b"\xff"
                        else unit.decode()) * 10000
            for capacity in list(range(512, 560)) + [1024, 4096, 65536, 65560]:
                value = serialize(unit * 10000, capacity)
                self.assertTrue(expected.startswith(value["message"]))
                self.assertEqual(value["truncated"], value["message"] != expected)
        for size in range(320, 370):
            value = serialize(b"x" * size, 512)
            self.assertEqual(value["truncated"], len(value["message"]) != size)

    def test_metadata_pressure(self):
        full = serialize(b"hello", mode="metadata")
        for key in ["level", "program", "process"]:
            self.assertEqual(full[key], "M" * 4096)
        self.assertFalse(full["truncated"])
        for capacity in [512, 513, 1024, 4096]:
            value = serialize(b"hello", capacity, "metadata")
            self.assertTrue(value["truncated"])
            for key in ["level", "program", "process"]:
                self.assertTrue(("M" * 4096).startswith(value[key]))

    def test_metadata_uses_safe_string_encoding(self):
        value = serialize(mode="metadata_unicode")
        self.assertEqual(value["level"], 'info"\nÿ')
        self.assertEqual(value["process"], "ð")
        self.assertEqual(value["program"], "é€😀")

    def test_mixed_binary_messages(self):
        rng = random.Random(7391)
        for _ in range(80):
            raw = bytes(rng.randrange(256) for _ in range(rng.randrange(2000)))
            # Python independently identifies malformed UTF-8 bytes.
            expected = "".join(chr(ord(c) - 0xdc00)
                               if 0xdc80 <= ord(c) <= 0xdcff else c
                               for c in raw.decode("utf-8", "surrogateescape"))
            self.assertEqual(serialize(raw)["message"], expected)
            value = serialize(raw, rng.randrange(512, 1200))
            self.assertTrue(expected.startswith(value["message"]))
            self.assertEqual(value["truncated"], value["message"] != expected)

    def test_optional_counters(self):
        value = serialize(b"closed", mode="counters")
        self.assertEqual(value["event"], "disconnect")
        self.assertEqual(set(value), COMMON | COUNTERS)
        for name in COUNTERS:
            self.assertEqual(value[name], 18446744073709551615)
            self.assertIs(type(value[name]), int)
        value = serialize(mode="zero")
        self.assertEqual(set(value), COMMON | {"duration_us", "target_bytes_written"})
        self.assertEqual(value["duration_us"], 0)
        self.assertEqual(value["target_bytes_written"], 0)
        self.assertEqual(value["event"], "session_snapshot")
        value = serialize(b"closed", 512, "counters")
        self.assertTrue(value["truncated"])
        for name in COUNTERS & value.keys():
            self.assertEqual(value[name], 18446744073709551615)

    def test_explicit_truncation_null_and_signed_limits(self):
        self.assertTrue(serialize(b"short", mode="truncated")["truncated"])
        value = serialize(mode="negative")
        self.assertEqual(value["timestamp"], "-9223372036854775808.000001")
        self.assertEqual(value["pid"], -9223372036854775808)
        value = serialize(mode="null")
        for key in ["level", "process", "program", "message"]:
            self.assertEqual(value[key], "")

    def test_event_names(self):
        for number, name in enumerate(["message", "accept", "hostid", "connect",
                                       "block", "temporary_block", "disconnect",
                                       "error", "temporary_error", "io",
                                       "session_snapshot"]):
            self.assertEqual(serialize(mode="event" + str(number))["event"], name)
        self.assertEqual(serialize(mode="event99")["event"], "message")


if __name__ == "__main__":
    unittest.main()
