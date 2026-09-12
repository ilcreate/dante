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
