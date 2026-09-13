#!/usr/bin/env python3
"""Exercise stage 4 session counters and snapshots through a real SOCKS server.

Run: python3 logformat_session_integration_test.py /path/to/sockd -v
Loopback sockets and permission to start workers and signal the mother are
required. Administrative/blocked/other-error close statuses have no controlled
network trigger in Dante and are covered by the session producer C driver.
Raw signal diagnostics remain permitted until stage 5.
"""

import argparse
from collections import Counter
from contextlib import contextmanager, ExitStack
import json
import os
from pathlib import Path
import pwd
import re
import signal
import socket
import struct
import tempfile
import time
import unittest

import logformat_iolog_integration_test as iolog_helpers
from logformat_iolog_integration_test import Daemon, TIMEOUT, address, exact, reply


BYTE_KEYS = ("client_bytes_read", "client_bytes_written",
             "target_bytes_read", "target_bytes_written")
PACKET_KEYS = ("client_packets_read", "client_packets_written",
               "target_packets_read", "target_packets_written")
REQUESTS = (b"a", bytes(range(256)) * 7, b"last-client-fragment")
RESPONSES = (b"target-first", bytes(range(255, -1, -1)) * 3, b"z")


class LogformatSessionIntegrationTest(unittest.TestCase):
    # Reuse exchange/address validation, without inheriting stage 3 test cases.
    client = iolog_helpers.LogformatIologIntegrationTest.client
    check_addresses = iolog_helpers.LogformatIologIntegrationTest.check_addresses

    @contextmanager
    def daemon(self, fmt="json", logs="connect disconnect error", ipv6=False,
               io_timeout=0):
        with tempfile.TemporaryDirectory(prefix="dante-session-", dir="/tmp") as tmp:
            root = Path(tmp)
            root.chmod(0o755)
            with socket.socket() as reserve:
                reserve.bind(("127.0.0.1", 0))
                port = reserve.getsockname()[1]
            username = pwd.getpwuid(os.getuid()).pw_name
            rulelog = f" log: {logs}\n" if logs else ""
            # Keep workers under the invoking user so the mother can propagate
            # SIGUSR1 on platforms enforcing process signal credentials.
            config = f"""logformat: {fmt}
logoutput: stderr
internal: 127.0.0.1 port = {port}
external: 127.0.0.1
{"external: ::1" if ipv6 else ""}
clientmethod: none
socksmethod: none
user.privileged: {username}
user.unprivileged: {username}
timeout.io: {io_timeout}
client pass {{
 from: 127.0.0.1/32 to: 0.0.0.0/0
{rulelog}}}
socks pass {{
 from: 127.0.0.1/32 to: 0.0.0.0/0
 protocol: tcp udp
{rulelog}}}
"""
            if ipv6:
                config += f"""socks pass {{
 from: 127.0.0.1/32 to: ::/0
 protocol: tcp udp
{rulelog}}}
socks pass {{
 from: ::1/128 to: 127.0.0.1/32
 protocol: udp
 command: udpreply
{rulelog}}}
"""
            daemon = Daemon(BINARY, root, config)
            try:
                daemon.ready()
                yield daemon, port
            finally:
                daemon.close()

    def records(self, daemon):
        output = daemon.text()
        self.assertIsNone(daemon.proc.poll(), output[-6000:])
        records = []
        for line in output.splitlines():
            if not line.startswith("{"):
                continue
            try:
                event = json.loads(line)
            except json.JSONDecodeError:
                # A read can race the final write. Only the last unfinished
                # line may be incomplete; a completed malformed line is a bug.
                if not output.endswith("\n") and line == output.splitlines()[-1]:
                    continue
                raise
            self.assertEqual(event["schema_version"], 1)
            self.assertIs(type(event["pid"]), int)
            self.assertIs(event["truncated"], False)
            records.append(event)
        return records

    def wait_events(self, daemon, event, count=1, **fields):
        deadline = time.monotonic() + TIMEOUT
        while time.monotonic() < deadline:
            matches = [record for record in self.records(daemon)
                       if record.get("event") == event
                       and all(record.get(key) == value for key, value in fields.items())]
            if len(matches) >= count:
                return matches
            time.sleep(0.05)
        self.fail(f"missing {count} {event} records {fields}:\n{daemon.text()[-8000:]}")

    def check_session(self, event, scope, command, protocol, reason=None):
        self.assertEqual(event["scope"], scope)
        self.assertEqual(event["command"], command)
        self.assertEqual(event["protocol"], protocol)
        self.assertEqual(event["verdict"], "pass")
        self.assertEqual(event["rule"]["type"],
                         "client-rule" if scope == "control" else "socks-rule")
        self.assertIs(type(event["rule"]["number"]), int)
        self.assertGreater(event["rule"]["number"], 0)
        self.assertIs(type(event["duration_us"]), int)
        self.assertGreaterEqual(event["duration_us"], 0)
        self.check_addresses(event)
        if reason is None:
            self.assertEqual(event["event"], "session_snapshot")
            self.assertNotIn("reason", event)
            self.assertNotIn("timeout", event)
            self.assertIs(type(event["idle_us"]), int)
            self.assertGreaterEqual(event["idle_us"], 0)
            self.assertLessEqual(event["idle_us"], event["duration_us"])
        else:
            self.assertEqual(event["event"], "session_close")
            self.assertEqual(event["reason"], reason)
            if reason != "timeout":
                self.assertNotIn("timeout", event)

    def check_counts(self, event, values, packets=None):
        for key, value in zip(BYTE_KEYS, values):
            self.assertIs(type(event[key]), int, key)
            self.assertEqual(event[key], value, key)
        for key in PACKET_KEYS:
            if packets is None:
                self.assertNotIn(key, event)
            else:
                self.assertIs(type(event[key]), int, key)
                self.assertEqual(event[key], packets, key)

    @contextmanager
    def tcp(self, port, bind=False):
        with socket.socket() as target, self.client(port) as proxy:
            target.settimeout(TIMEOUT)
            target.bind(("127.0.0.1", 0))
            target_port = target.getsockname()[1]
            if bind:
                proxy.sendall(b"\x05\x02\x00" + address("127.0.0.1", target_port))
                status, bound = reply(proxy)
                self.assertEqual(status, 0)
                target.connect(("127.0.0.1", bound[1]))
                self.assertEqual(reply(proxy)[0], 0)
                yield proxy, target, target_port
            else:
                target.listen(1)
                proxy.sendall(b"\x05\x01\x00" + address("127.0.0.1", target_port))
                self.assertEqual(reply(proxy)[0], 0)
                with target.accept()[0] as peer:
                    peer.settimeout(TIMEOUT)
                    yield proxy, peer, target_port

    def transfer(self, proxy, peer):
        # Receiving each fragment forces progress but does not assume the
        # relay's read boundaries equal the sender's write boundaries.
        for fragment in REQUESTS:
            proxy.sendall(fragment)
            self.assertEqual(exact(peer, len(fragment)), fragment)
        for fragment in RESPONSES:
            peer.sendall(fragment)
            self.assertEqual(exact(proxy, len(fragment)), fragment)
        return (sum(map(len, REQUESTS)), sum(map(len, RESPONSES)),
                sum(map(len, RESPONSES)), sum(map(len, REQUESTS)))

    def closed_tcp(self, daemon, command="connect", count=2):
        self.wait_events(daemon, "session_close", count=count)
        daemon.settled()
        closes = [e for e in self.records(daemon) if e["event"] == "session_close"]
        data = [e for e in closes if e.get("scope") == "session"
                and e.get("command") == command]
        self.assertEqual(len(data), 1, closes)
        controls = [e for e in closes if e.get("scope") == "control"]
        self.assertEqual(len(controls), 1, closes)
        self.check_session(controls[0], "control", "accept", "tcp", data[0]["reason"])
        for key in BYTE_KEYS[:2]:
            self.assertIs(type(controls[0][key]), int)
            self.assertGreaterEqual(controls[0][key], 0)
        for key in BYTE_KEYS[2:] + PACKET_KEYS:
            self.assertNotIn(key, controls[0])
        self.assertNotIn("destination", controls[0])
        for key in BYTE_KEYS[:2]:
            self.assertEqual(controls[0][key], data[0][key],
                             "TCP control counters must follow the actual client socket")
        return data[0], closes

    def test_tcp_close_known_bytes_and_zero(self):
        for transfer in (True, False):
            with self.subTest(transfer=transfer), self.daemon() as (daemon, port):
                with self.tcp(port) as (proxy, peer, target_port):
                    client_port = proxy.getsockname()[1]
                    counts = self.transfer(proxy, peer) if transfer else (0, 0, 0, 0)
                    proxy.shutdown(socket.SHUT_WR)
                    self.assertEqual(peer.recv(1), b"")
                event, closes = self.closed_tcp(daemon)
                self.assertEqual(len(closes), 2)
                self.check_session(event, "session", "connect", "tcp", "closed")
                self.check_counts(event, counts)
                self.assertEqual(event["source"]["peer"]["port"], client_port)
                self.assertEqual(event["destination"]["peer"]["port"], target_port)
                self.assertGreater(event["duration_us"], 0)

    def test_bind_reversed_counts_and_raw_close_parity(self):
        counts_by_format = {}
        for fmt in ("raw", "json"):
            with self.subTest(fmt=fmt), self.daemon(fmt=fmt) as (daemon, port):
                with self.tcp(port, bind=True) as (proxy, peer, target_port):
                    client_port = proxy.getsockname()[1]
                    counts = self.transfer(proxy, peer)
                    proxy.shutdown(socket.SHUT_WR)
                    self.assertEqual(peer.recv(1), b"")
                if fmt == "raw":
                    daemon.settled()
                    counts_by_format[fmt] = Counter(re.findall(
                        r"pass\(\d+\): tcp/(accept|bind|bindreply) \]:", daemon.text()))
                else:
                    event, closes = self.closed_tcp(daemon, "bindreply", count=3)
                    self.assertEqual(len(closes), 3)
                    self.check_session(event, "session", "bindreply", "tcp", "closed")
                    self.check_counts(event, counts)
                    self.assertEqual(event["source"]["peer"]["port"], client_port)
                    self.assertEqual(event["destination"]["peer"]["port"], target_port)
                    listeners = [e for e in closes if e.get("scope") == "bind_listener"]
                    self.assertEqual(len(listeners), 1)
                    self.check_session(listeners[0], "bind_listener", "bind", "tcp", "closed")
                    for key in BYTE_KEYS + PACKET_KEYS:
                        self.assertNotIn(key, listeners[0])
                    counts_by_format[fmt] = Counter(e["command"] for e in closes)
        if "raw" in counts_by_format:
            self.assertEqual(counts_by_format["raw"], Counter(accept=1, bind=1, bindreply=1))
        if len(counts_by_format) == 2:
            self.assertEqual(counts_by_format["raw"], counts_by_format["json"])

    def test_close_log_filter_and_raw_json_count_parity(self):
        for logs in ("disconnect", "error", "connect", ""):
            totals = {}
            for fmt in ("raw", "json"):
                with self.subTest(logs=logs, fmt=fmt), self.daemon(fmt=fmt, logs=logs) as (daemon, port):
                    with self.tcp(port) as (proxy, peer, _):
                        self.transfer(proxy, peer)
                        proxy.shutdown(socket.SHUT_WR)
                        self.assertEqual(peer.recv(1), b"")
                    if fmt == "json" and logs == "disconnect":
                        self.wait_events(daemon, "session_close", 2)
                    daemon.settled()
                    if fmt == "raw":
                        totals[fmt] = Counter(re.findall(
                            r"pass\(\d+\): tcp/(accept|connect) \]:", daemon.text()))
                    else:
                        records = self.records(daemon)
                        totals[fmt] = Counter(e["command"] for e in records
                                              if e["event"] == "session_close")
                        # Converting a producer must replace the old line,
                        # rather than leave a second untyped summary behind.
                        self.assertFalse([e for e in records if e["event"] == "message"
                                          and re.search(r"pass\(\d+\): tcp/\w+ \]:", e["message"])])
            if len(totals) == 2:
                self.assertEqual(totals["raw"], totals["json"])
                self.assertEqual(totals["json"], Counter(accept=1, connect=1)
                                 if logs == "disconnect" else Counter())

    def snapshots(self, daemon, scope, count=1, await_handoff=False):
        previous = len([e for e in self.records(daemon)
                        if e.get("event") == "session_snapshot" and e.get("scope") == scope])
        if await_handoff:
            # UDP's successful reply is sent by the request worker before the
            # IO worker receives the session. Observe a snapshot to establish
            # readiness without sending data (the no-target case needs zeros).
            deadline = time.monotonic() + TIMEOUT
            next_signal = 0
            while time.monotonic() < deadline:
                now = time.monotonic()
                if now >= next_signal:
                    os.kill(daemon.proc.pid, signal.SIGUSR1)
                    next_signal = now + 0.2
                records = [e for e in self.records(daemon)
                           if e.get("event") == "session_snapshot" and e.get("scope") == scope]
                if len(records) >= previous + count:
                    self.assertFalse([e for e in self.records(daemon) if e["event"] == "session_close"])
                    return records[previous:]
                time.sleep(0.025)
            self.fail("UDP session was not handed to an IO worker:\n" + daemon.text()[-8000:])
        os.kill(daemon.proc.pid, signal.SIGUSR1)
        records = self.wait_events(daemon, "session_snapshot", count=previous + count, scope=scope)
        self.assertFalse([e for e in self.records(daemon) if e["event"] == "session_close"])
        return records[previous:]

    def test_tcp_repeated_snapshots_do_not_close_or_change_counters(self):
        with self.daemon() as (daemon, port):
            with self.tcp(port) as (proxy, peer, _):
                counts = self.transfer(proxy, peer)
                snapshots = [self.snapshots(daemon, "session")[0] for _ in range(2)]
                for event in snapshots:
                    self.check_session(event, "session", "connect", "tcp")
                    self.check_counts(event, counts)
                self.assertGreaterEqual(snapshots[1]["duration_us"], snapshots[0]["duration_us"])
                self.assertGreaterEqual(snapshots[1]["idle_us"], snapshots[0]["idle_us"])
                proxy.sendall(b"still-open")
                self.assertEqual(exact(peer, 10), b"still-open")
            event, _ = self.closed_tcp(daemon)
            self.check_counts(event, (counts[0] + 10, counts[1], counts[2], counts[3] + 10))

    @contextmanager
    def udp(self, port):
        with socket.socket(type=socket.SOCK_DGRAM) as udp, self.client(port) as proxy:
            udp.settimeout(TIMEOUT)
            udp.bind(("127.0.0.1", 0))
            proxy.sendall(b"\x05\x03\x00" + address(*udp.getsockname()))
            status, relay = reply(proxy)
            self.assertEqual(status, 0)
            yield udp, ("127.0.0.1", relay[1])

    def require_ipv6(self):
        try:
            with socket.socket(socket.AF_INET6) as probe:
                probe.bind(("::1", 0))
        except OSError as error:
            self.skipTest(f"IPv6 loopback unavailable: {error}")

    def udp_targets(self, families):
        with self.daemon(ipv6=socket.AF_INET6 in families) as (daemon, port):
            with self.udp(port) as (udp, relay), ExitStack() as stack:
                expected = {}
                for family in families:
                    target = stack.enter_context(socket.socket(family, socket.SOCK_DGRAM))
                    target.settimeout(TIMEOUT)
                    target.bind(("::1" if family == socket.AF_INET6 else "127.0.0.1", 0))
                    host, target_port = target.getsockname()[:2]
                    header = b"\x00\x00\x00" + address(host, target_port)
                    requests = (b"first", b"second-udp-request", b"third")
                    responses = (b"one", b"two", b"longer-third-response")
                    for request, response in zip(requests, responses):
                        udp.sendto(header + request, relay)
                        try:
                            data, sender = target.recvfrom(65535)
                        except TimeoutError:
                            self.fail(f"UDP relay to {host} timed out:\n{daemon.text()[-8000:]}")
                        self.assertEqual(data, request)
                        target.sendto(response, sender)
                        try:
                            received = udp.recvfrom(65535)[0]
                        except TimeoutError:
                            self.fail(f"UDP relay response from {host} timed out:\n{daemon.text()[-8000:]}")
                        self.assertEqual(received, header + response)
                    request_size = sum(map(len, requests))
                    response_size = sum(map(len, responses))
                    expected[target_port] = (request_size + 3 * len(header),
                                             response_size + 3 * len(header),
                                             response_size, request_size)
                for _ in range(2):
                    snapshots = self.snapshots(daemon, "udp_target", len(families))
                    self.assertEqual(len(snapshots), len(families))
                    self.assertEqual({e["destination"]["peer"]["port"] for e in snapshots}, set(expected))
                    for event in snapshots:
                        self.check_session(event, "udp_target", "udpassociate", "udp")
                        self.check_counts(event, expected[event["destination"]["peer"]["port"]], packets=3)
            self.wait_events(daemon, "session_close", len(families) + 1)
            daemon.settled()
            closes = [e for e in self.records(daemon) if e["event"] == "session_close"]
            targets = [e for e in closes if e.get("scope") == "udp_target"]
            self.assertEqual(len(closes), len(families) + 1)
            self.assertEqual(len(targets), len(families))
            for event in targets:
                self.check_session(event, "udp_target", "udpassociate", "udp", "closed")
                self.check_counts(event, expected[event["destination"]["peer"]["port"]], packets=3)
            control = [e for e in closes if e.get("scope") == "control"]
            self.assertEqual(len(control), 1)
            self.check_session(control[0], "control", "accept", "tcp", "closed")
            for key in BYTE_KEYS[2:] + PACKET_KEYS:
                self.assertNotIn(key, control[0])

    def test_udp_ipv4_wire_bytes_packets_snapshots_and_close(self):
        self.udp_targets((socket.AF_INET,))

    def test_udp_two_address_family_buckets(self):
        self.require_ipv6()
        self.udp_targets((socket.AF_INET, socket.AF_INET6))

    def test_udp_snapshot_without_target(self):
        with self.daemon() as (daemon, port):
            with self.udp(port):
                for iteration in range(2):
                    event = self.snapshots(daemon, "session", await_handoff=iteration == 0)[0]
                    self.check_session(event, "session", "udpassociate", "udp")
                    self.assertNotIn("destination", event)
                    # No destination endpoint exists yet, but the native
                    # session counters are initialized and known to be zero.
                    self.check_counts(event, (0, 0, 0, 0), packets=0)
            closes = self.wait_events(daemon, "session_close", 2)
            session = [e for e in closes if e.get("scope") == "session"]
            self.assertEqual(len(session), 1)
            self.check_session(session[0], "session", "udpassociate", "udp", "closed")

    def test_idle_timeout_reason_and_known_zero_counters(self):
        with self.daemon(io_timeout=1) as (daemon, port):
            with self.tcp(port) as (proxy, peer, _):
                event, _ = self.closed_tcp(daemon)
                self.check_session(event, "session", "connect", "tcp", "timeout")
                self.check_counts(event, (0, 0, 0, 0))
                self.assertIsInstance(event["timeout"], str)
                self.assertTrue(event["timeout"])
                self.assertGreater(event["duration_us"], 0)
                self.assertEqual(proxy.recv(1), b"")
                self.assertEqual(peer.recv(1), b"")

    def test_target_reset_io_error_reason_and_side(self):
        with self.daemon() as (daemon, port):
            with self.tcp(port) as (proxy, peer, _):
                counts = self.transfer(proxy, peer)
                peer.setsockopt(socket.SOL_SOCKET, socket.SO_LINGER, struct.pack("ii", 1, 0))
                peer.close()
                event, _ = self.closed_tcp(daemon)
                self.check_session(event, "session", "connect", "tcp", "io_error")
                self.assertEqual(event["side"], "target")
                self.check_counts(event, counts)
                self.assertIs(type(event["error"]["code"]), int)
                self.assertNotEqual(event["error"]["code"], 0)

    def test_refused_connect_is_close_with_zero_counters(self):
        with socket.socket() as reserve, self.daemon() as (daemon, port):
            reserve.bind(("127.0.0.1", 0))
            target_port = reserve.getsockname()[1]
            reserve.close()
            with self.client(port) as proxy:
                proxy.sendall(b"\x05\x01\x00" + address("127.0.0.1", target_port))
                self.assertNotEqual(reply(proxy)[0], 0)
            event, closes = self.closed_tcp(daemon)
            self.assertEqual(len(closes), 2)
            self.check_session(event, "session", "connect", "tcp", "io_error")
            self.assertEqual(event["side"], "target")
            self.check_counts(event, (0, 0, 0, 0))


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("binary", type=Path)
    args, remaining = parser.parse_known_args()
    BINARY = str(args.binary.resolve())
    unittest.main(argv=[__file__] + remaining)
