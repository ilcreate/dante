#!/usr/bin/env python3
"""Live format reloads and simultaneous real workers; Linux/macOS loopback only."""

import argparse
from concurrent.futures import ThreadPoolExecutor
from contextlib import contextmanager, ExitStack
import json
import os
from pathlib import Path
import pwd
import re
import signal
import socket
import tempfile
import threading
import time
import unittest

from logformat_iolog_integration_test import Daemon, TIMEOUT, address, exact, reply


class RuntimeIntegrationTest(unittest.TestCase):
    def config(self, port, output, errorlog, fmt):
        username = pwd.getpwuid(os.getuid()).pw_name
        directive = "" if fmt is None else f"logformat: {fmt}\n"
        return directive + f"""logoutput: stderr {output}
errorlog: {errorlog}
internal: 127.0.0.1 port = {port}
external: 127.0.0.1
clientmethod: none
# Duplicate method deliberately exercises warning routing to errorlog.
socksmethod: none none
user.privileged: {username}
user.unprivileged: {username}
client pass {{
 from: 127.0.0.1/32 to: 0.0.0.0/0
 log: connect disconnect error
}}
socks pass {{
 from: 127.0.0.1/32 to: 127.0.0.0/8
 protocol: tcp udp
 log: connect disconnect error data
}}
"""

    @contextmanager
    def daemon(self, fmt="json", fifo=False):
        with tempfile.TemporaryDirectory(prefix="dante-runtime-", dir="/tmp") as tmp:
            root = Path(tmp)
            root.chmod(0o755)
            output, errorlog = root / "output.log", root / "error.log"
            stop = threading.Event()
            reader = None
            captured = bytearray()
            if fifo:
                os.mkfifo(output)
                # Separate read/write opens work on both Linux and Darwin;
                # keepalive prevents EOF between daemon logfile reopens.
                readfd = os.open(output, os.O_RDONLY | os.O_NONBLOCK)
                keepalive = os.open(output, os.O_WRONLY | os.O_NONBLOCK)

                def drain():
                    while True:
                        try:
                            chunk = os.read(readfd, 65536)
                        except BlockingIOError:
                            chunk = b""
                        if chunk:
                            captured.extend(chunk)
                        elif stop.is_set():
                            return
                        else:
                            stop.wait(0.005)

                reader = threading.Thread(target=drain, daemon=True)
                reader.start()
            with socket.socket() as reserve:
                reserve.bind(("127.0.0.1", 0))
                port = reserve.getsockname()[1]
            daemon = Daemon(BINARY, root, self.config(port, output, errorlog, fmt))
            daemon.runtime_output = lambda: bytes(captured) if fifo else output.read_bytes()
            try:
                daemon.ready()
                yield daemon, port, root, output, errorlog
            except BaseException:
                print(daemon.text()[-12000:])
                raise
            finally:
                daemon.close()
                if reader:
                    stop.set()
                    reader.join(timeout=5)
                    os.close(keepalive)
                    os.close(readfd)
                    self.assertFalse(reader.is_alive(), "FIFO reader did not stop")

    @contextmanager
    def tcp(self, port):
        with socket.socket() as target, socket.create_connection(("127.0.0.1", port), TIMEOUT) as proxy:
            target.settimeout(TIMEOUT)
            target.bind(("127.0.0.1", 0))
            target.listen(1)
            proxy.sendall(b"\x05\x01\x00")
            self.assertEqual(exact(proxy, 2), b"\x05\x00")
            proxy.sendall(b"\x05\x01\x00" + address(*target.getsockname()))
            self.assertEqual(reply(proxy)[0], 0)
            with target.accept()[0] as peer:
                peer.settimeout(TIMEOUT)
                yield proxy, peer

    @contextmanager
    def udp(self, port):
        with socket.socket(type=socket.SOCK_DGRAM) as client, \
                socket.socket(type=socket.SOCK_DGRAM) as target, \
                socket.create_connection(("127.0.0.1", port), TIMEOUT) as control:
            client.settimeout(TIMEOUT)
            target.settimeout(TIMEOUT)
            client.bind(("127.0.0.1", 0))
            target.bind(("127.0.0.1", 0))
            control.sendall(b"\x05\x01\x00")
            self.assertEqual(exact(control, 2), b"\x05\x00")
            control.sendall(b"\x05\x03\x00" + address(*client.getsockname()))
            status, relay = reply(control)
            self.assertEqual(status, 0)
            yield client, target, ("127.0.0.1", relay[1])

    def wait(self, daemon, predicate):
        deadline = time.monotonic() + TIMEOUT
        while time.monotonic() < deadline:
            result = predicate()
            if result:
                return result
            self.assertIsNone(daemon.proc.poll(), daemon.text()[-6000:])
            time.sleep(0.025)
        self.fail("runtime condition timed out:\n" + daemon.text()[-8000:])

    def objects(self, data, allow_initial_raw=False):
        self.assertTrue(data.endswith(b"\n"), data[-200:])
        lines = data.splitlines()
        if allow_initial_raw:
            start = next((i for i, line in enumerate(lines) if line.startswith(b"{")), None)
            self.assertIsNotNone(start, data[-2000:])
            lines = lines[start:]
        result = [json.loads(line) for line in lines]
        for event in result:
            self.assertEqual(event["schema_version"], 1)
            self.assertIs(type(event["pid"]), int)
            self.assertIs(type(event["truncated"]), bool)
            self.assertRegex(event["timestamp"], r"^\d+\.\d{6}$")
        return result

    def marker(self, daemon, marker, fmt, offset=0, pid=None):
        def find():
            # Ignore only an unfinished last write while the daemon is live.
            for line in daemon.path.read_bytes()[offset:].split(b"\n")[:-1]:
                if marker.encode() not in line:
                    continue
                if fmt == "json" and line.startswith(b"{"):
                    event = json.loads(line)
                    if marker in event.get("payload", "") or marker in event["message"]:
                        if pid is None or event["pid"] == pid:
                            return event["pid"]
                elif fmt == "raw" and not line.startswith(b"{"):
                    match = re.search(rb"\[(\d+)\]:", line)
                    if match and (pid is None or int(match[1]) == pid):
                        return int(match[1])
            return None
        return self.wait(daemon, find)

    def exchange(self, proxy, peer, data):
        proxy.sendall(data)
        self.assertEqual(exact(peer, len(data)), data)
        peer.sendall(data)
        self.assertEqual(exact(proxy, len(data)), data)

    def test_live_reload_existing_worker_and_new_sessions(self):
        with self.daemon(fmt="raw") as (daemon, port, root, output, errorlog):
            with self.tcp(port) as (proxy, peer):
                self.exchange(proxy, peer, b"reload-initial-worker")
                worker = self.marker(daemon, "reload-initial-worker", "raw")
                for index, fmt in enumerate(("json", "json", "raw", "json", None)):
                    expected = fmt or "raw"
                    offset = daemon.path.stat().st_size
                    (root / "sockd.conf").write_text(self.config(port, output, errorlog, fmt))
                    daemon.proc.send_signal(signal.SIGHUP)
                    self.wait(daemon, lambda: b"SIGHUP ]: config reloaded" in daemon.path.read_bytes()[offset:])
                    # Updates are asynchronous. Probe the original live session
                    # until its original PID emits the requested format.
                    deadline = time.monotonic() + TIMEOUT
                    marker = f"reload-phase-{index}"
                    while True:
                        self.exchange(proxy, peer, marker.encode())
                        time.sleep(0.05)
                        lines = daemon.path.read_bytes()[offset:].split(b"\n")[:-1]
                        if any(marker.encode() in line and line.startswith(b"{") == (expected == "json")
                               for line in lines):
                            break
                        self.assertLess(time.monotonic(), deadline, daemon.text()[-6000:])
                    self.assertEqual(self.marker(daemon, marker, expected, offset, worker), worker)
                    with self.tcp(port) as (newproxy, newpeer):
                        newmarker = f"reload-new-session-{index}"
                        self.exchange(newproxy, newpeer, newmarker.encode())
                        self.marker(daemon, newmarker, expected, offset)
                    if index == 1:
                        daemon.settled()
                        self.objects(daemon.path.read_bytes()[offset:])
            # Transitional mixing is allowed, but every JSON record must parse.
            daemon.settled()
            for line in daemon.path.read_bytes().splitlines():
                if line.startswith(b"{"):
                    self.objects(line + b"\n")

    def test_concurrent_workers_file_and_fifo(self):
        for fifo in (False, True):
            with self.subTest(fifo=fifo), self.daemon(fifo=fifo) as (daemon, port, root, _, errorlog):
                with ExitStack() as stack:
                    # Exceed the standard 32 sessions per IO worker; assert
                    # multiple PIDs actually produced traffic records below.
                    sessions = [stack.enter_context(self.udp(port)) for _ in range(40)]
                    barrier = threading.Barrier(len(sessions))

                    def traffic(index):
                        barrier.wait(timeout=TIMEOUT)
                        client, target, relay = sessions[index]
                        header = b"\0\0\0" + address(*target.getsockname())
                        for sequence in range(4):
                            data = f"concurrent-{index}-{sequence} ".encode() + b'x' * 6000
                            client.sendto(header + data, relay)
                            received, sender = target.recvfrom(65535)
                            self.assertEqual(received, data)
                            target.sendto(data, sender)
                            self.assertEqual(client.recvfrom(65535)[0], header + data)

                    with ThreadPoolExecutor(max_workers=len(sessions)) as pool:
                        list(pool.map(traffic, range(len(sessions))))
                    daemon.proc.send_signal(signal.SIGUSR1)
                    self.wait(daemon, lambda: "negotiators (" in daemon.text())
                self.wait(daemon, lambda: daemon.text().count('"event":"session_close"') >= 80)
                self.wait(daemon, lambda: errorlog.stat().st_size > 0)
                daemon.settled()
                events = self.objects(daemon.runtime_output())
                workers = {e["pid"] for e in events if e["event"] == "io"}
                self.assertGreaterEqual(len(workers), 2)
                traffic_events = [e for e in events if e["event"] == "io"]
                self.assertEqual(len(traffic_events), 40 * 4 * 2)
                self.assertGreaterEqual(sum(e["event"] == "session_close" for e in events), 80)
                if fifo:
                    self.assertTrue(any(e["truncated"] for e in events))
                    # All writes, including diagnostics, fit the FIFO limit.
                    limit = os.pathconf(str(daemon.path.parent / "output.log"), "PC_PIPE_BUF")
                    self.assertTrue(all(len(line) + 1 <= limit
                                        for line in daemon.runtime_output().splitlines()))
                errors = self.objects(errorlog.read_bytes())
                self.assertTrue(errors)
                self.assertTrue(all(e["level"] in ("emergency", "alert", "critical", "error", "warning")
                                    for e in errors))
                self.assertTrue(all(e in events for e in errors))
                self.objects(daemon.path.read_bytes(), allow_initial_raw=True)
                print(f"{('FIFO' if fifo else 'file')}: {len(events)} valid records, "
                      f"{len(workers)} IO workers, {len(traffic_events)} traffic records, "
                      f"{len(errors)} errorlog records")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("binary", type=Path)
    args, remaining = parser.parse_known_args()
    BINARY = str(args.binary.resolve())
    unittest.main(argv=[__file__] + remaining)
