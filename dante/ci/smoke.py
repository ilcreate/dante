#!/usr/bin/env python3
"""Exercise an installed daemon over loopback and read complete HTTP responses."""

import argparse
import json
import os
import pwd
import signal
import socket
import struct
import subprocess
import tempfile
import time
from pathlib import Path


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
        config.write_text(f"""logoutput: stderr
internal: 127.0.0.1 port = {port}
external: 127.0.0.1
clientmethod: none
socksmethod: none
user.privileged: {username}
user.unprivileged: {unprivileged}
client pass {{
 from: 127.0.0.1/32 to: 0.0.0.0/0
}}
socks pass {{
 from: 127.0.0.1/32 to: 127.0.0.0/8
 protocol: tcp udp
}}
""")
        subprocess.run([binary, "-V", "-f", str(config)], check=True)
        with (root / "daemon.log").open("w+") as log:
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
                deadline = time.monotonic() + 5
                while True:
                    status, after = http(endpoint)
                    if after["counters"]["client_connections_accepted_total"] > before["counters"]["client_connections_accepted_total"]:
                        break
                    assert time.monotonic() < deadline, "accepted counter did not advance"
                    time.sleep(0.1)
                assert status == 200 and daemon.poll() is None
                print("PASS: full JSON, HTTP errors, SOCKS5, counters" + (", TCP relay" if args.relay else ""))
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


if __name__ == "__main__":
    main()
