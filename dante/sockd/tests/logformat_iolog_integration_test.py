#!/usr/bin/env python3
"""Exercise structured iolog records with real SOCKS5 TCP, BIND and UDP traffic.

Run: python3 logformat_iolog_integration_test.py /path/to/sockd -v
Requires loopback sockets and permission to start sockd workers. Connection
fields are checked directly; only the raw-format parity test reads raw labels.
Stage 4 close counters and stage 5 signal diagnostics are outside this suite.
"""

import argparse
from collections import Counter
from contextlib import contextmanager
import ipaddress
import json
import os
from pathlib import Path
import pwd
import re
import signal
import socket
import struct
import subprocess
import sys
import tempfile
import time
import unittest


TIMEOUT = 8
EVENTS = {"accept", "hostid", "connect", "block", "temporary_block",
          "disconnect", "error", "temporary_error", "io"}
PAYLOAD = 'logformat "quoted" \\ line\n\t café '.encode() + b"\x00\xff\xfe"
RESPONSE = b"target-reply\x00\x01\x7f"


def exact(sock, length):
    result = bytearray()
    while len(result) < length:
        chunk = sock.recv(length - len(result))
        if not chunk:
            raise AssertionError("unexpected EOF in SOCKS exchange")
        result.extend(chunk)
    return bytes(result)


def address(host, port, domain=False):
    if domain:
        encoded = host.encode("ascii")
        result = b"\x03" + bytes([len(encoded)]) + encoded
    else:
        parsed = ipaddress.ip_address(host)
        result = bytes([1 if parsed.version == 4 else 4]) + parsed.packed
    return result + struct.pack("!H", port)


def read_address(sock, atyp):
    if atyp == 1:
        host = socket.inet_ntop(socket.AF_INET, exact(sock, 4))
    elif atyp == 4:
        host = socket.inet_ntop(socket.AF_INET6, exact(sock, 16))
    elif atyp == 3:
        host = exact(sock, exact(sock, 1)[0]).decode("ascii")
    else:
        raise AssertionError(f"invalid SOCKS address type: {atyp}")
    return host, struct.unpack("!H", exact(sock, 2))[0]


def reply(sock):
    version, status, reserved, atyp = exact(sock, 4)
    if (version, reserved) != (5, 0):
        raise AssertionError("invalid SOCKS5 reply")
    return status, read_address(sock, atyp)


def signal_group(pid, sig):
    """Darwin may transiently report EPERM while a process group exits."""
    for attempt in range(20):
        try:
            os.killpg(pid, sig)
            return
        except ProcessLookupError:
            return
        except PermissionError:
            if attempt == 19:
                raise
            time.sleep(0.05)


class Daemon:
    def __init__(self, binary, root, config):
        self.path = root / "daemon.log"
        configpath = root / "sockd.conf"
        configpath.write_text(config)
        self.log = self.path.open("wb")
        self.proc = subprocess.Popen(
            [binary, "-f", str(configpath), "-p", str(root / "pid")],
            stdout=self.log, stderr=self.log, start_new_session=True,
        )

    def text(self):
        return self.path.read_text(encoding="utf-8", errors="strict")

    def ready(self):
        deadline = time.monotonic() + 15
        while time.monotonic() < deadline:
            output = self.text()
            if self.proc.poll() is not None:
                raise AssertionError("sockd exited at startup:\n" + output[-6000:])
            if " running" in output:
                return
            time.sleep(0.05)
        raise AssertionError("sockd startup timed out:\n" + self.text()[-6000:])

    def settled(self):
        # Successful synchronous relay proves the I/O happened. Give the worker
        # time to write its record after the send and to observe the client EOF.
        deadline = time.monotonic() + 3
        unchanged_since = time.monotonic()
        size = -1
        while time.monotonic() < deadline:
            current = self.path.stat().st_size
            if current != size:
                size = current
                unchanged_since = time.monotonic()
            if time.monotonic() - unchanged_since >= 0.3:
                return
            time.sleep(0.05)

    def close(self):
        signal_group(self.proc.pid, signal.SIGTERM)
        try:
            self.proc.wait(timeout=5)
        except subprocess.TimeoutExpired:
            signal_group(self.proc.pid, signal.SIGKILL)
            self.proc.wait(timeout=5)
        finally:
            # The mother may exit before a worker; terminate any remaining group.
            try:
                signal_group(self.proc.pid, signal.SIGKILL)
            finally:
                self.log.close()


class LogformatIologIntegrationTest(unittest.TestCase):
    @contextmanager
    def daemon(self, fmt="json", logs="connect disconnect error",
               verdict="pass", ipv6=False, target=None):
        with tempfile.TemporaryDirectory(prefix="dante-iolog-", dir="/tmp") as tmp:
            root = Path(tmp)
            root.chmod(0o755)
            with socket.socket() as reserve:
                reserve.bind(("127.0.0.1", 0))
                port = reserve.getsockname()[1]
            username = pwd.getpwuid(os.getuid()).pw_name
            unprivileged = "nobody" if os.getuid() == 0 else username
            rulelog = f" log: {logs}\n" if logs else ""
            extra_external = "external: ::1\n" if ipv6 else ""
            if target is None:
                target = "::1/128" if ipv6 else "0.0.0.0/0"
            config = f"""logformat: {fmt}
logoutput: stderr
internal: 127.0.0.1 port = {port}
external: 127.0.0.1
{extra_external}clientmethod: none
socksmethod: none
user.privileged: {username}
user.unprivileged: {unprivileged}
client pass {{
 from: 127.0.0.1/32 to: 0.0.0.0/0
{rulelog}}}
socks {verdict} {{
 from: 127.0.0.1/32 to: {target}
 protocol: tcp udp
{rulelog}}}
"""
            daemon = Daemon(BINARY, root, config)
            try:
                daemon.ready()
                yield daemon, port
                daemon.settled()
            finally:
                daemon.close()

    @contextmanager
    def client(self, port):
        with socket.create_connection(("127.0.0.1", port), timeout=TIMEOUT) as sock:
            sock.sendall(b"\x05\x01\x00")
            self.assertEqual(exact(sock, 2), b"\x05\x00")
            yield sock

    def records(self, daemon):
        daemon.settled()
        output = daemon.text()
        self.assertIsNone(daemon.proc.poll(), output[-6000:])
        records = []
        for line in output.splitlines():
            if line.startswith("{"):
                event = json.loads(line)
                self.assertEqual(event["schema_version"], 1)
                self.assertRegex(event["timestamp"], r"^\d+\.\d{6}$")
                self.assertIs(type(event["pid"]), int)
                self.assertIs(event["truncated"], False)
                records.append(event)
            # Early parser diagnostics and signalslog remain raw until stage 5.
            elif re.search(r"(?:pass|block)\(\d+\): (?:tcp|udp)/", line):
                self.fail("connection event escaped JSON serialization: " + line)
        self.assertTrue(records, output[-6000:])
        return records

    def events(self, records, name=None, command=None):
        return [event for event in records if event["event"] in EVENTS
                and (name is None or event["event"] == name)
                and (command is None or event.get("command") == command)]

    def require_event(self, records, name, command, protocol, verdict="pass"):
        matches = self.events(records, name, command)
        self.assertTrue(matches, f"missing structured {name}/{command}: {records[-8:]}")
        for event in matches:
            self.assertEqual(event["protocol"], protocol)
            self.assertEqual(event["verdict"], verdict)
            self.assertIs(type(event["rule"]["number"]), int)
            self.assertGreater(event["rule"]["number"], 0)
            self.assertIn(event["rule"]["type"], ("client-rule", "socks-rule"))
            self.assertIsInstance(event["message"], str)
            self.assertNotIn("password", event)
            self.check_addresses(event)
        return matches

    def check_addresses(self, event):
        for name in ("source", "destination", "source_proxy", "destination_proxy"):
            if name not in event:
                continue
            group = event[name]
            self.assertIsInstance(group, dict)
            for side in ("local", "peer"):
                if side not in group:
                    continue
                endpoint = group[side]
                self.assertIn(endpoint["type"], ("ipv4", "ipv6", "domain"))
                self.assertIsInstance(endpoint["address"], str)
                self.assertIs(type(endpoint["port"]), int)
                self.assertGreaterEqual(endpoint["port"], 0)
                self.assertLessEqual(endpoint["port"], 65535)
                if endpoint["type"] != "domain":
                    parsed = ipaddress.ip_address(endpoint["address"])
                    self.assertEqual(parsed.version, 4 if endpoint["type"] == "ipv4" else 6)
                if "authentication" in group:
                    self.assertEqual(group["authentication"]["method"], "none")
                    self.assertNotIn("user", group["authentication"])
                    self.assertNotIn("password", group["authentication"])

    def has_endpoint(self, events, host, port, kind):
        endpoints = [group[side] for event in events
                     for name in ("source", "destination", "source_proxy", "destination_proxy")
                     for group in [event.get(name, {})]
                     for side in ("local", "peer") if side in group]
        self.assertTrue(any(all(endpoint.get(key) == value for key, value in
                               {"type": kind, "address": host, "port": port}.items())
                            for endpoint in endpoints), endpoints)

    def tcp(self, port, family=socket.AF_INET, domain=False, payload=PAYLOAD,
            fragmented=False):
        host = "::1" if family == socket.AF_INET6 else "127.0.0.1"
        with socket.socket(family) as target:
            target.settimeout(TIMEOUT)
            target.bind((host, 0))
            target.listen(1)
            target_port = target.getsockname()[1]
            with self.client(port) as proxy:
                client_port = proxy.getsockname()[1]
                proxy.sendall(b"\x05\x01\x00" + address(
                    "localhost" if domain else host, target_port, domain))
                self.assertEqual(reply(proxy)[0], 0)
                with target.accept()[0] as peer:
                    peer.settimeout(TIMEOUT)
                    if fragmented:
                        # Deliberately split within UTF-8 where possible. Reading
                        # the first piece before sending the second forces the
                        # relay to process separate chunks, without sleeps.
                        split = (payload.index(b"\xc3") + 1 if b"\xc3" in payload
                                 else len(payload) // 2)
                        pieces = (payload[:split], payload[split:])
                    else:
                        pieces = (payload,)
                    for piece in pieces:
                        proxy.sendall(piece)
                        self.assertEqual(exact(peer, len(piece)), piece)
                    peer.sendall(RESPONSE)
                    self.assertEqual(exact(proxy, len(RESPONSE)), RESPONSE)
                    proxy.shutdown(socket.SHUT_WR)
                    self.assertEqual(peer.recv(1), b"")
        return host, target_port, client_port

    def test_tcp_connect_ipv4(self):
        with self.daemon(logs="connect disconnect error tcpinfo") as (daemon, port):
            host, target_port, client_port = self.tcp(port)
            records = self.records(daemon)
            self.require_event(records, "accept", "accept", "tcp")
            events = self.require_event(records, "connect", "connect", "tcp")
            self.has_endpoint(events, host, target_port, "ipv4")
            self.has_endpoint(events, "127.0.0.1", client_port, "ipv4")
            for event in events:
                self.assertEqual(event["source"]["local"]["port"], port)
                self.assertEqual(event["source"]["peer"]["port"], client_port)
                self.assertEqual(event["destination"]["peer"]["port"], target_port)
                self.assertGreater(event["destination"]["local"]["port"], 0)
                self.assertNotIn("source_proxy", event)
                self.assertNotIn("destination_proxy", event)
            if sys.platform.startswith("linux"):
                self.assertTrue(any(event.get("tcp_info") for event in events))
            for event in events:
                if "tcp_info" in event:
                    self.assertIsInstance(event["tcp_info"], str)
            self.assertTrue(all("payload" not in event for event in records))
            self.assertFalse(self.events(records, "io"))

    def test_tcp_connect_domain(self):
        with self.daemon() as (daemon, port):
            _, target_port, _ = self.tcp(port, domain=True)
            records = self.records(daemon)
            events = self.require_event(records, "connect", "connect", "tcp")
            # The established connection records the resolved socket address.
            self.has_endpoint(events, "127.0.0.1", target_port, "ipv4")
        # Rejection retains the original request host before opening a target.
        with self.daemon(verdict="block") as (daemon, port):
            with self.client(port) as proxy:
                proxy.sendall(b"\x05\x01\x00" + address("localhost", 9, domain=True))
                self.assertEqual(reply(proxy)[0], 2)
            events = self.require_event(self.records(daemon), "block", "connect", "tcp", "block")
            self.has_endpoint(events, "localhost", 9, "domain")

    def test_tcp_connect_ipv6(self):
        try:
            with socket.socket(socket.AF_INET6) as probe:
                probe.bind(("::1", 0))
        except OSError as error:
            self.skipTest(f"IPv6 loopback unavailable: {error}")
        with self.daemon(ipv6=True) as (daemon, port):
            host, target_port, _ = self.tcp(port, socket.AF_INET6)
            events = self.require_event(self.records(daemon), "connect", "connect", "tcp")
            self.has_endpoint(events, host, target_port, "ipv6")

    def test_bind(self):
        with self.daemon(logs="connect disconnect error data") as (daemon, port):
            with socket.socket() as incoming, self.client(port) as proxy:
                client_port = proxy.getsockname()[1]
                incoming.settimeout(TIMEOUT)
                incoming.bind(("127.0.0.1", 0))
                incoming_port = incoming.getsockname()[1]
                proxy.sendall(b"\x05\x02\x00" + address("127.0.0.1", incoming_port))
                status, bound = reply(proxy)
                self.assertEqual(status, 0)
                incoming.connect(("127.0.0.1" if bound[0] == "0.0.0.0" else bound[0], bound[1]))
                self.assertEqual(reply(proxy)[0], 0)
                incoming.sendall(PAYLOAD)
                self.assertEqual(exact(proxy, len(PAYLOAD)), PAYLOAD)
                proxy.sendall(RESPONSE)
                self.assertEqual(exact(incoming, len(RESPONSE)), RESPONSE)
            records = self.records(daemon)
            self.require_event(records, "connect", "bind", "tcp")
            self.require_event(records, "connect", "bindreply", "tcp")
            self.check_payload(records, True, {incoming_port: PAYLOAD, client_port: RESPONSE})

    def test_rule_block(self):
        for command, number, protocol in (("connect", 1, "tcp"), ("udpassociate", 3, "udp")):
            with self.subTest(command=command), self.daemon(verdict="block") as (daemon, port):
                with self.client(port) as proxy:
                    proxy.sendall(b"\x05" + bytes([number]) + b"\x00" + address("127.0.0.1", 9))
                    self.assertEqual(reply(proxy)[0], 2, "expected SOCKS ruleset-denied response")
                records = self.records(daemon)
                self.require_event(records, "block", command, protocol, "block")
                self.assertFalse(self.events(records, "connect", command))
                self.assertFalse(self.events(records, "error", command))

    def test_connect_failure(self):
        # Release an ephemeral port immediately before requesting it: macOS can
        # silently queue SYNs for a still-bound, non-listening socket.
        with socket.socket() as refused, self.daemon() as (daemon, port):
            refused.bind(("127.0.0.1", 0))
            target_port = refused.getsockname()[1]
            refused.close()
            with self.client(port) as proxy:
                proxy.sendall(b"\x05\x01\x00" + address("127.0.0.1", target_port))
                self.assertNotEqual(reply(proxy)[0], 0)
            records = self.records(daemon)
            self.require_event(records, "accept", "accept", "tcp")
            # Async refusal is logged by io_delete(), whose typed close event
            # belongs to stage 4. It must not become a rule-block event here.
            self.assertFalse(self.events(records, "block", "connect"))
            self.assertFalse(self.events(records, "connect", "connect"))

    def test_request_connection_error(self):
        # Permit IPv6 while configuring only an IPv4 external address. The
        # request has no usable outgoing address and hits iolog(OPERATION_ERROR)
        # before the async-connect/close path, without relying on external DNS.
        with self.daemon(target="::1/128") as (daemon, port):
            with self.client(port) as proxy:
                proxy.sendall(b"\x05\x01\x00" + address("::1", 9))
                self.assertNotEqual(reply(proxy)[0], 0)
            records = self.records(daemon)
            events = self.require_event(records, "error", "connect", "tcp")
            for event in events:
                self.assertEqual(event["rule"]["type"], "client-rule")
                self.assertIn("source", event)
                # Request validation runs before the destination is initialized.
                self.assertNotIn("destination", event)
            self.assertFalse(self.events(records, "block", "connect"))

    def check_payload(self, records, enabled, expected_by_source):
        events = self.events(records, "io")
        self.assertTrue(events, "no structured I/O events")
        transferred = Counter()
        for event in events:
            self.assertIs(type(event["io_bytes"]), int)
            self.assertGreaterEqual(event["io_bytes"], 0)
            self.check_addresses(event)
            self.assertEqual("payload" in event, enabled)
            source_port = event["source"]["peer"]["port"]
            self.assertIn(source_port, expected_by_source)
            offset = transferred[source_port]
            transferred[source_port] += event["io_bytes"]
            self.assertLessEqual(transferred[source_port], len(expected_by_source[source_port]))
            if enabled:
                chunk = expected_by_source[source_port][offset:transferred[source_port]]
                # Compare each actual byte range independently: TCP may split
                # UTF-8 characters, which then become invalid bytes in each
                # individual record. JSON cannot reconstruct those boundaries.
                decoded = chunk.decode("utf-8", "surrogateescape")
                decoded = "".join(chr(ord(c) - 0xdc00) if 0xdc80 <= ord(c) <= 0xdcff else c
                                  for c in decoded)
                self.assertEqual(event["payload"], decoded)
        self.assertEqual(dict(transferred), {port: len(data) for port, data in expected_by_source.items()})
        if not enabled:
            self.assertTrue(all("payload" not in event for event in records))

    def test_tcp_payload_and_iooperation(self):
        for flag, enabled in (("data", True), ("iooperation", False)):
            with self.subTest(flag=flag), self.daemon(logs="connect disconnect error " + flag) as (daemon, port):
                _, target_port, client_port = self.tcp(port, fragmented=True)
                self.check_payload(self.records(daemon), enabled,
                                   {client_port: PAYLOAD, target_port: RESPONSE})

    def udp(self, port):
        with socket.socket(type=socket.SOCK_DGRAM) as target, \
                socket.socket(type=socket.SOCK_DGRAM) as udp, self.client(port) as proxy:
            target.settimeout(TIMEOUT)
            udp.settimeout(TIMEOUT)
            target.bind(("127.0.0.1", 0))
            udp.bind(("127.0.0.1", 0))
            proxy.sendall(b"\x05\x03\x00" + address(*udp.getsockname()))
            status, relay = reply(proxy)
            self.assertEqual(status, 0)
            relay = ("127.0.0.1" if relay[0] == "0.0.0.0" else relay[0], relay[1])
            udp.sendto(b"\x00\x00\x00" + address(*target.getsockname()) + PAYLOAD, relay)
            data, sender = target.recvfrom(65535)
            self.assertEqual(data, PAYLOAD)
            target.sendto(RESPONSE, sender)
            response, _ = udp.recvfrom(65535)
            self.assertEqual(response, b"\x00\x00\x00" + address(*target.getsockname()) + RESPONSE)
            return target.getsockname()[1], udp.getsockname()[1]

    def test_udp_payload_and_iooperation(self):
        for flag, enabled in (("data", True), ("iooperation", False), ("", None)):
            with self.subTest(flag=flag), self.daemon(logs="connect disconnect error " + flag) as (daemon, port):
                target_port, client_port = self.udp(port)
                records = self.records(daemon)
                self.require_event(records, "connect", "udpassociate", "udp")
                if enabled is None:
                    self.assertFalse(self.events(records, "io"))
                    self.assertTrue(all("payload" not in event for event in records))
                else:
                    self.check_payload(records, enabled,
                                       {client_port: PAYLOAD, target_port: RESPONSE})
                    self.assertEqual(len(self.events(records, "io")), 2)
                    self.has_endpoint(self.events(records, "io"), "127.0.0.1", target_port, "ipv4")

    def test_raw_json_event_count_and_filter_parity(self):
        # TCP chunk boundaries may differ between runs. Compare lifecycle counts
        # and byte totals by direction; exact I/O counts are asserted for UDP.
        for logs in ("connect", "connect iooperation", ""):
            counts = {}
            io_bytes = {}
            for fmt in ("raw", "json"):
                with self.subTest(logs=logs, fmt=fmt), self.daemon(fmt=fmt, logs=logs) as (daemon, port):
                    _, target_port, client_port = self.tcp(
                        port, payload=b"parity-probe", fragmented=fmt == "json")
                    directions = {client_port: "client_to_target", target_port: "target_to_client"}
                    io_bytes[fmt] = Counter()
                    daemon.settled()
                    if fmt == "raw":
                        matches = re.findall(r"(?:pass|block)\(\d+\): tcp/(accept|connect) ([\[-]): ([^\n]*)", daemon.text())
                        counts[fmt] = Counter(command for command, marker, _ in matches if marker == "[")
                        for _, marker, body in matches:
                            if marker != "-":
                                continue
                            fields = re.fullmatch(r"127\.0\.0\.1\.(\d+) .* \((\d+)\)", body)
                            self.assertIsNotNone(fields, body)
                            source_port, size = map(int, fields.groups())
                            self.assertIn(source_port, directions)
                            io_bytes[fmt][directions[source_port]] += size
                    else:
                        records = self.records(daemon)
                        counts[fmt] = Counter(event["command"]
                                              for event in self.events(records)
                                              if event["event"] in ("accept", "connect"))
                        for event in self.events(records, "io"):
                            source_port = event["source"]["peer"]["port"]
                            self.assertIn(source_port, directions)
                            self.assertNotIn("payload", event)
                            io_bytes[fmt][directions[source_port]] += event["io_bytes"]
            self.assertEqual(counts["raw"], counts["json"], f"log: {logs!r}")
            self.assertEqual(io_bytes["raw"], io_bytes["json"], f"log: {logs!r}")
            expected_bytes = ({"client_to_target": len(b"parity-probe"),
                               "target_to_client": len(RESPONSE)}
                              if "iooperation" in logs else {})
            self.assertEqual(dict(io_bytes["json"]), expected_bytes)
            if not logs:
                self.assertFalse(counts["json"])


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("binary", type=Path)
    args, remaining = parser.parse_known_args()
    BINARY = str(args.binary.resolve())
    unittest.main(argv=[__file__] + remaining)
