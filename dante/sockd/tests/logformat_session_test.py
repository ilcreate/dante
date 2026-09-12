#!/usr/bin/env python3
"""Exercise real session-close and snapshot producers with native counters."""
import json
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

BINARY = str(Path(sys.argv.pop(1)).resolve())

def counters(event):
    return {k: v for k, v in event.items() if k == "duration_us" or "_bytes_" in k or "_packets_" in k}


class SessionTest(unittest.TestCase):
    def output(self, case, fmt="json"):
        with tempfile.TemporaryFile() as output:
            p = subprocess.run([BINARY, fmt, case], stdout=output,
                               stderr=subprocess.PIPE, timeout=15)
            self.assertEqual(p.returncode, 0, p.stderr)
            output.seek(0)
            return output.read()

    def events(self, case):
        return [e for line in self.output(case).splitlines()
                if (e := json.loads(line))["event"] != "message"]

    def test_close_reasons_and_counters(self):
        for suffix, reason in [("", "closed"), ("block", "blocked"),
                               ("ioerror", "io_error"), ("error", "error"),
                               ("timeout", "timeout"), ("admin", "administrative")]:
            with self.subTest(reason=reason):
                events = self.events("tcp" + suffix)
                self.assertEqual(len(events), 1)
                e = events[0]
                self.assertEqual(e["event"], "session_close")
                self.assertEqual(e["reason"], reason)
                self.assertEqual(e["side"], "target")
                self.assertEqual(e["scope"], "session")
                self.assertEqual(counters(e), dict(duration_us=10500000,
                    client_bytes_read=11, client_bytes_written=12,
                    target_bytes_read=21, target_bytes_written=22))
                if "error" in suffix:
                    self.assertGreater(e["error"]["code"], 0)
                else:
                    self.assertNotIn("error", e)

    def test_timeout_types_and_missing_errno(self):
        for case, kind in [("tcptimeout", "io"), ("tcpconnecttimeout", "connect"),
                           ("tcpfintimeout", "tcp_fin_wait")]:
            self.assertEqual(self.events(case)[0]["timeout"], kind)
        self.assertNotIn("error", self.events("tcperrorzeroerrno")[0])
        self.assertEqual(self.events("tcpclient")[0]["side"], "client")
        self.assertEqual(self.events("tcpsession")[0]["side"], "session")
        self.assertEqual(counters(self.events("tcpmax")[0])["client_bytes_read"], 2**64 - 1)

    def test_rule_filtering_and_bind_reversal(self):
        self.assertEqual(self.events("tcpquiet"), [])
        self.assertEqual(self.events("tcperroronly"), [])
        self.assertEqual(len(self.events("tcpioerroronly")), 1)
        events = self.events("bind")
        self.assertEqual(len(events), 2)
        self.assertEqual(counters(events[0])["client_bytes_read"], 21)
        self.assertEqual(counters(events[0])["target_bytes_read"], 11)
        self.assertEqual(events[0]["side"], "target")
        self.assertEqual(events[0]["source"]["peer"]["address"], "203.0.113.1")
        self.assertEqual(events[0]["destination"]["peer"]["address"], "198.51.100.1")
        self.assertEqual(events[1]["scope"], "bind_listener")
        self.assertEqual(counters(events[1]), {"duration_us": 10500000})
        self.assertEqual(len(self.events("bindquiet")), 1)

    def test_udp_slots_and_control(self):
        events = self.events("udpioerror")
        self.assertEqual(len(events), 3)
        for n, e in enumerate(events[:2]):
            self.assertEqual(e["scope"], "udp_target")
            self.assertEqual(e["side"], "target")
            self.assertEqual(counters(e), dict(duration_us=10500000,
                client_bytes_read=100 + n, client_bytes_written=200 + n,
                target_bytes_read=300 + n, target_bytes_written=400 + n,
                client_packets_read=10 + n, client_packets_written=20 + n,
                target_packets_read=30 + n, target_packets_written=40 + n))
        self.assertEqual(events[2]["scope"], "control")
        self.assertEqual(counters(events[2]), dict(duration_us=10500000,
            client_bytes_read=31, client_bytes_written=32))
        self.assertEqual(self.events("udpcontrol")[0]["side"], "control")

    def test_snapshots_do_not_close_or_mutate(self):
        for case, count in [("tcpsnapshot", 1), ("bindsnapshot", 1),
                            ("udpemptysnapshot", 1), ("udpsnapshot", 2),
                            ("tcpquietsnapshot", 1)]:
            with self.subTest(case=case):
                events = self.events(case)
                self.assertEqual(len(events), count)
                for e in events:
                    self.assertEqual(e["event"], "session_snapshot")
                    self.assertNotIn("reason", e)
                    self.assertNotIn("side", e)
                    self.assertNotIn("error", e)
                    self.assertGreater(e["idle_us"], 0)
                if case == "udpemptysnapshot":
                    self.assertEqual(counters(events[0])["target_bytes_read"], 0)
                    self.assertNotIn("destination", events[0])
                if case == "udpsnapshot":
                    self.assertEqual(counters(events[0])["duration_us"], 8750000)
                    self.assertEqual(counters(events[1])["client_bytes_read"], 101)
                if case == "tcpsnapshot":
                    self.assertEqual(events[0]["idle_us"], 2250000)

    def test_native_tcp_info_is_complete_and_rule_controlled(self):
        native = 'native first line\nsecond "quoted" line'
        for case in ["tcptcpinfo", "tcptcpinfosnapshot", "bindtcpinfo"]:
            for event in self.events(case):
                self.assertEqual(event["tcp_info"], native)
        for event in self.events("tcptcpinfolonginfo"):
            self.assertEqual(event["tcp_info"], "x" * 3000)
            self.assertFalse(event["truncated"])
        for case in ["tcp", "tcpsnapshot", "bind"]:
            for event in self.events(case):
                self.assertNotIn("tcp_info", event)
        events = self.events("bindtcpinfolisteneroff")
        self.assertEqual(events[0]["tcp_info"], native)
        self.assertNotIn("tcp_info", events[1])
        for case in ["tcptcpinfo", "tcptcpinfosnapshot", "bindtcpinfo",
                     "bindtcpinfolisteneroff", "tcptcpinfolonginfo"]:
            raw = self.output(case, "raw").decode()
            records = [json.loads(line) for line in self.output(case).splitlines()]
            for event in records:
                self.assertIn(event["message"], raw)

    def test_reset_propagation_and_cleanup(self):
        for case, side in [("tcpreset", "target"), ("tcpclientreset", "client"),
                           ("bindreset", "target"), ("bindclientreset", "client")]:
            e = self.events(case)[0]
            self.assertEqual(e["reason"], "io_error")
            self.assertEqual(e["side"], side)
            self.assertGreater(e["error"]["code"], 0)

    def test_unknown_times_are_omitted_and_zero_elapsed_is_known(self):
        for case in ["tcp", "bind", "udp", "tcpsnapshot", "udpsnapshot"]:
            for e in self.events(case + "unknowntime"):
                self.assertNotIn("duration_us", e)
                self.assertNotIn("idle_us", e)
            for e in self.events(case + "zerotime"):
                self.assertEqual(e["duration_us"], 0)
                if "snapshot" in case:
                    self.assertEqual(e["idle_us"], 0)

    def test_raw_message_and_event_count_preserved(self):
        for case in ["tcp", "tcpioerror", "tcptimeout", "bind", "udpioerror",
                     "tcpsnapshot", "udpsnapshot", "udpemptysnapshot"]:
            raw = self.output(case, "raw").decode()
            structured = [json.loads(line) for line in self.output(case).splitlines()]
            messages = [e["message"] for e in structured]
            # Raw timestamps/prefixes differ; every old message remains verbatim.
            for message in messages:
                self.assertIn(message, raw)
            self.assertEqual(len(raw.splitlines()), len(structured))

if __name__ == "__main__":
    unittest.main()
