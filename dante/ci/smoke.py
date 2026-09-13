#!/usr/bin/env python3
"""Exercise an installed daemon and validate its stats and JSON connection logs."""

import argparse
import json
import os
import pwd
import re
import signal
import socket
import struct
import subprocess
import tempfile
import time
from pathlib import Path


COMMON_LOG_FIELDS = {
    "schema_version", "timestamp", "level", "pid", "process", "program",
    "event", "message", "truncated",
}
RELAY_BYTES = {
    "client_bytes_read": 19,
    "client_bytes_written": 8,
    "target_bytes_read": 8,
    "target_bytes_written": 19,
}


def read_json_log(path):
    raw = path.read_bytes()
    assert raw, "JSON log is empty"
    assert raw.endswith(b"\n"), "JSON log does not end with a newline"
    try:
        text = raw.decode("utf-8", "strict")
    except UnicodeDecodeError as error:
        raise AssertionError(f"JSON log is not UTF-8: {error}") from error
    records = []
    for lineno, line in enumerate(text.removesuffix("\n").split("\n"), 1):
        assert line, f"JSON log line {lineno} is empty"
        try:
            record = json.loads(line)
        except json.JSONDecodeError as error:
            raise AssertionError(f"JSON log line {lineno} is invalid: {error}") from error
        assert isinstance(record, dict), f"JSON log line {lineno} is not an object"
        missing = COMMON_LOG_FIELDS - record.keys()
        assert not missing, f"JSON log line {lineno} lacks {sorted(missing)}"
        assert record["schema_version"] == 1, f"JSON log line {lineno} has unknown schema"
        assert re.fullmatch(r"[0-9]+\.[0-9]{6}", record["timestamp"]), \
            f"JSON log line {lineno} has invalid timestamp"
        assert type(record["pid"]) is int, f"JSON log line {lineno} has invalid pid"
        assert type(record["truncated"]) is bool, \
            f"JSON log line {lineno} has invalid truncated flag"
        for field in ("level", "process", "program", "event", "message"):
            assert isinstance(record[field], str), \
                f"JSON log line {lineno} has invalid {field}"
        records.append(record)
    return records


def _require_event(records, description, predicate):
    matches = [record for record in records if predicate(record)]
    assert matches, f"missing structured {description} event"
    return matches


def validate_json_log(path, relay=False, client_port=None, target_port=None):
    records = read_json_log(path)
    _require_event(records, "accept/accept", lambda event:
                   event["event"] == "accept" and event.get("command") == "accept"
                   and event.get("protocol") == "tcp"
                   and event.get("rule", {}).get("type") == "client-rule")
    _require_event(records, "error/connect", lambda event:
                   event["event"] == "error" and event.get("command") == "connect"
                   and event.get("protocol") == "tcp"
                   and event.get("rule", {}).get("type") == "client-rule")
    if relay:
        def is_relay(event, name):
            return (event["event"] == name and event.get("command") == "connect"
                    and event.get("protocol") == "tcp"
                    and event.get("source", {}).get("peer", {}).get("port") == client_port
                    and event.get("destination", {}).get("peer", {}).get("port") == target_port)

        connects = _require_event(records, "connect/connect", lambda event:
                                  is_relay(event, "connect"))
        assert all(event.get("rule", {}).get("type") == "socks-rule"
                   and event.get("verdict") == "pass" for event in connects), \
            "structured connect event has wrong rule or verdict"
        closes = _require_event(records, "session_close/connect", lambda event:
                                is_relay(event, "session_close")
                                and event.get("scope") == "session"
                                and event.get("reason") == "closed")
        for field, expected in RELAY_BYTES.items():
            assert any(event.get(field) == expected for event in closes), \
                f"structured session_close has wrong {field}"
    return records


def exact(sock, length):
    result = b""
    while len(result) < length:
        chunk = sock.recv(length - len(result))
        if not chunk:
            raise RuntimeError("unexpected EOF")
        result += chunk
    return result


def http(path, target="/v1/stats", method="GET"):
    with socket.socket(socket.AF_UNIX) as client:
        client.settimeout(4)
        client.connect(str(path))
        client.sendall(f"{method} {target} HTTP/1.0\r\nHost: localhost\r\n\r\n".encode())
        chunks = []
        while True:
            chunk = client.recv(16384)
            if not chunk:
                break
            chunks.append(chunk)
    headers, body = b"".join(chunks).split(b"\r\n\r\n", 1)
    fields = dict(line.lower().split(b":", 1) for line in headers.split(b"\r\n")[1:])
    assert int(fields[b"content-length"]) == len(body), "truncated HTTP response"
    return int(headers.split()[1]), json.loads(body)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("binary", type=Path)
    parser.add_argument("--relay", action="store_true")
    args = parser.parse_args()
    binary = str(args.binary.resolve())
    subprocess.run([binary, "-v"], check=True)
    with tempfile.TemporaryDirectory(prefix="dante-smoke-", dir="/tmp") as tmp:
        root = Path(tmp)
        root.chmod(0o755)
        # Use a short path: Darwin UNIX sockets have a 104-byte path limit.
        socket_dir = root / "api"
        socket_dir.mkdir(mode=0o700)
        if os.getuid() == 0:
            nobody = pwd.getpwnam("nobody")
            os.chown(socket_dir, nobody.pw_uid, nobody.pw_gid)
        endpoint = socket_dir / "stats.sock"
        with socket.socket() as reserve:
            reserve.bind(("127.0.0.1", 0))
            port = reserve.getsockname()[1]
        username = pwd.getpwuid(os.getuid()).pw_name
        unprivileged = "nobody" if os.getuid() == 0 else username
        config = root / "sockd.conf"
        event_log = root / "events.log"
        event_log.touch(mode=0o666)
        event_log.chmod(0o666)
        config.write_text(f"""logformat: json
logoutput: {event_log}
errorlog: {event_log}
internal: 127.0.0.1 port = {port}
external: 127.0.0.1
clientmethod: none
socksmethod: none
user.privileged: {username}
user.unprivileged: {unprivileged}
client pass {{
 from: 127.0.0.1/32 to: 0.0.0.0/0
 log: connect disconnect error
}}
socks pass {{
 from: 127.0.0.1/32 to: 127.0.0.0/8
 protocol: tcp
 log: connect disconnect error
}}
socks pass {{
 from: 127.0.0.1/32 to: ::1/128
 protocol: tcp
 command: connect
 log: connect disconnect error
}}
""")
        subprocess.run([binary, "-V", "-f", str(config)], check=True)
        daemon_log = root / "daemon.log"
        client_port = target_port = None
        with daemon_log.open("w+") as log:
            daemon = subprocess.Popen([binary, "-f", str(config), "-p", str(root / "pid"), "-S", str(endpoint)],
                                      stdout=log, stderr=log, start_new_session=True)
            try:
                deadline = time.monotonic() + 15
                while True:
                    if daemon.poll() is not None:
                        raise RuntimeError("daemon exited at startup")
                    try:
                        status, before = http(endpoint)
                        assert status == 200 and before["schema_version"] == 1
                        break
                    except (OSError, AssertionError):
                        if time.monotonic() >= deadline:
                            raise
                        time.sleep(0.1)
                assert http(endpoint, "/v2/stats")[0] == 404
                assert http(endpoint, method="POST")[0] == 405
                with socket.create_connection(("127.0.0.1", port), timeout=5) as proxy:
                    client_port = proxy.getsockname()[1]
                    proxy.sendall(b"\x05\x01\x00")
                    assert exact(proxy, 2) == b"\x05\x00", "SOCKS5 negotiation failed"
                    if args.relay:
                        with socket.socket() as target:
                            target.bind(("127.0.0.1", 0))
                            target.listen(1)
                            target.settimeout(5)
                            target_port = target.getsockname()[1]
                            proxy.sendall(b"\x05\x01\x00\x01\x7f\x00\x00\x01" + struct.pack("!H", target_port))
                            with target.accept()[0] as peer:
                                peer.settimeout(5)
                                reply = exact(proxy, 4)
                                assert reply[:3] == b"\x05\x00\x00", reply
                                assert reply[3] in (1, 4), reply
                                exact(proxy, 6 if reply[3] == 1 else 18)
                                proxy.sendall(b"dante-package-probe")
                                assert exact(peer, 19) == b"dante-package-probe"
                                peer.sendall(b"relay-ok")
                                assert exact(proxy, 8) == b"relay-ok"
                with socket.create_connection(("127.0.0.1", port), timeout=5) as proxy:
                    proxy.sendall(b"\x05\x01\x00")
                    assert exact(proxy, 2) == b"\x05\x00", "SOCKS5 negotiation failed"
                    proxy.sendall(b"\x05\x01\x00\x04" + socket.inet_pton(socket.AF_INET6, "::1")
                                  + struct.pack("!H", 9))
                    reply = exact(proxy, 4)
                    assert reply[:1] == b"\x05" and reply[1] != 0, reply
                    assert reply[3] in (1, 4), reply
                    exact(proxy, 6 if reply[3] == 1 else 18)
                deadline = time.monotonic() + 5
                while True:
                    status, after = http(endpoint)
                    if after["counters"]["client_connections_accepted_total"] > before["counters"]["client_connections_accepted_total"]:
                        break
                    assert time.monotonic() < deadline, "accepted counter did not advance"
                    time.sleep(0.1)
                assert status == 200 and daemon.poll() is None
                deadline = time.monotonic() + 5
                while time.monotonic() < deadline:
                    try:
                        records = read_json_log(event_log)
                    except AssertionError:
                        records = []
                    have_error = any(event.get("event") == "error"
                                     and event.get("command") == "connect" for event in records)
                    have_close = not args.relay or any(
                        event.get("event") == "session_close"
                        and event.get("scope") == "session"
                        and event.get("destination", {}).get("peer", {}).get("port") == target_port
                        for event in records)
                    if have_error and have_close:
                        break
                    time.sleep(0.05)
                else:
                    raise AssertionError("structured logging events did not settle")
            except BaseException:
                log.flush()
                log.seek(0)
                print(log.read())
                raise
            finally:
                try:
                    os.killpg(daemon.pid, signal.SIGTERM)
                except ProcessLookupError:
                    pass
                try:
                    daemon.wait(timeout=5)
                except subprocess.TimeoutExpired:
                    os.killpg(daemon.pid, signal.SIGKILL)
                    daemon.wait(timeout=5)
        try:
            records = validate_json_log(event_log, args.relay, client_port, target_port)
        except BaseException:
            print(daemon_log.read_text())
            print(event_log.read_text())
            raise
        print(f"PASS: {len(records)} JSON log lines, HTTP errors, SOCKS5, counters, "
              "structured accept/error" + (", TCP relay/close" if args.relay else ""))


if __name__ == "__main__":
    main()
