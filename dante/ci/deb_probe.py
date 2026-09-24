#!/usr/bin/env python3
"""Dependency-free SOCKS5/PAM, relay, and statistics probes for the DEB VM."""

from __future__ import annotations

import argparse
import json
import socket
import struct
import sys
from pathlib import Path


class AuthenticationRejected(RuntimeError):
    pass


def exact(stream: socket.socket, size: int) -> bytes:
    result = b""
    while len(result) < size:
        block = stream.recv(size - len(result))
        if not block:
            raise RuntimeError("unexpected EOF from SOCKS server")
        result += block
    return result


def authenticate(stream: socket.socket, username: str, password: str) -> None:
    user = username.encode()
    secret = password.encode()
    if not user or len(user) > 255 or len(secret) > 255:
        raise ValueError("SOCKS credentials must contain 1..255 bytes")
    stream.sendall(b"\x05\x01\x02")
    if exact(stream, 2) != b"\x05\x02":
        raise AuthenticationRejected("SOCKS server rejected username/password method")
    stream.sendall(b"\x01" + bytes([len(user)]) + user + bytes([len(secret)]) + secret)
    reply = exact(stream, 2)
    if reply[0] != 1 or reply[1] != 0:
        raise AuthenticationRejected("PAM authentication rejected")


def read_address(stream: socket.socket, atyp: int) -> tuple[str, int]:
    if atyp == 1:
        host = socket.inet_ntop(socket.AF_INET, exact(stream, 4))
    elif atyp == 4:
        host = socket.inet_ntop(socket.AF_INET6, exact(stream, 16))
    elif atyp == 3:
        host = exact(stream, exact(stream, 1)[0]).decode()
    else:
        raise RuntimeError(f"invalid SOCKS address type {atyp}")
    return host, struct.unpack("!H", exact(stream, 2))[0]


def request(stream: socket.socket, command: int, target_host: str, target_port: int) -> tuple[str, int]:
    address = socket.inet_pton(socket.AF_INET, target_host)
    stream.sendall(b"\x05" + bytes([command]) + b"\x00\x01" + address + struct.pack("!H", target_port))
    reply = exact(stream, 4)
    if reply[0] != 5 or reply[1] != 0:
        raise RuntimeError(f"SOCKS request failed with code {reply[1] if len(reply) > 1 else -1}")
    return read_address(stream, reply[3])


def socks_tcp(
    proxy_host: str, proxy_port: int, username: str, password: str,
    target_host: str, target_port: int, payload: bytes,
) -> bytes:
    with socket.create_connection((proxy_host, proxy_port), timeout=10) as stream:
        stream.settimeout(10)
        authenticate(stream, username, password)
        request(stream, 1, target_host, target_port)
        stream.sendall(payload)
        return exact(stream, len(payload))


def socks_udp(
    proxy_host: str, proxy_port: int, username: str, password: str,
    target_host: str, target_port: int, payload: bytes,
) -> bytes:
    with socket.create_connection((proxy_host, proxy_port), timeout=10) as control:
        control.settimeout(10)
        authenticate(control, username, password)
        relay_host, relay_port = request(control, 3, "0.0.0.0", 0)
        if relay_host == "0.0.0.0":
            relay_host = proxy_host
        with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as client:
            client.settimeout(10)
            packet = b"\x00\x00\x00\x01" + socket.inet_aton(target_host) + struct.pack("!H", target_port) + payload
            client.sendto(packet, (relay_host, relay_port))
            reply, _ = client.recvfrom(65535)
        if len(reply) < 10 or reply[:3] != b"\x00\x00\x00" or reply[3] != 1:
            raise RuntimeError("invalid SOCKS UDP relay response")
        return reply[10:]


def stats(path: Path) -> dict:
    with socket.socket(socket.AF_UNIX) as client:
        client.settimeout(10)
        client.connect(str(path))
        client.sendall(b"GET /v1/stats HTTP/1.0\r\nHost: localhost\r\n\r\n")
        chunks = []
        while block := client.recv(65536):
            chunks.append(block)
    headers, body = b"".join(chunks).split(b"\r\n\r\n", 1)
    if not headers.startswith(b"HTTP/1.1 200 "):
        raise RuntimeError(f"statistics endpoint returned {headers.splitlines()[0]!r}")
    result = json.loads(body)
    if result.get("schema_version") != 1:
        raise RuntimeError("unexpected statistics schema")
    return result


def echo_tcp(port: int) -> None:
    with socket.socket() as listener:
        listener.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        listener.bind(("127.0.0.1", port))
        listener.listen(8)
        while True:
            peer, _ = listener.accept()
            with peer:
                peer.settimeout(10)
                data = peer.recv(65535)
                peer.sendall(data)


def echo_udp(port: int) -> None:
    with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as server:
        server.bind(("127.0.0.1", port))
        while True:
            data, peer = server.recvfrom(65535)
            server.sendto(data, peer)


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    subparsers = parser.add_subparsers(dest="command", required=True)
    for name in ("tcp", "udp"):
        command = subparsers.add_parser(name)
        command.add_argument("--proxy-port", type=int, default=1080)
        command.add_argument("--username", required=True)
        command.add_argument("--password", required=True)
        command.add_argument("--target-port", required=True, type=int)
    statistics = subparsers.add_parser("stats")
    statistics.add_argument("socket", type=Path)
    for name in ("echo-tcp", "echo-udp"):
        command = subparsers.add_parser(name)
        command.add_argument("port", type=int)
    args = parser.parse_args(argv)
    if args.command == "tcp":
        result = socks_tcp("127.0.0.1", args.proxy_port, args.username, args.password,
                           "127.0.0.1", args.target_port, b"dante-tcp-relay")
        if result != b"dante-tcp-relay":
            raise RuntimeError("TCP relay changed payload")
    elif args.command == "udp":
        result = socks_udp("127.0.0.1", args.proxy_port, args.username, args.password,
                           "127.0.0.1", args.target_port, b"dante-udp-relay")
        if result != b"dante-udp-relay":
            raise RuntimeError("UDP relay changed payload")
    elif args.command == "stats":
        print(json.dumps(stats(args.socket), sort_keys=True))
    elif args.command == "echo-tcp":
        echo_tcp(args.port)
    elif args.command == "echo-udp":
        echo_udp(args.port)
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except AuthenticationRejected as exc:
        print(f"deb-probe: {exc}", file=sys.stderr)
        raise SystemExit(2)
    except (OSError, RuntimeError, ValueError) as exc:
        print(f"deb-probe: {exc}", file=sys.stderr)
        raise SystemExit(1)
