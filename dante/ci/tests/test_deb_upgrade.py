import importlib.util
import socket
import struct
import subprocess
import tempfile
import threading
import unittest
import json
from pathlib import Path
from unittest import mock


CI = Path(__file__).parents[1]
spec = importlib.util.spec_from_file_location("deb_probe", CI / "deb_probe.py")
deb_probe = importlib.util.module_from_spec(spec)
spec.loader.exec_module(deb_probe)


def exact(sock, size):
    data = b""
    while len(data) < size:
        chunk = sock.recv(size - len(data))
        if not chunk:
            raise EOFError
        data += chunk
    return data


class FakeSocksServer:
    def __init__(self, accept=True):
        self.accept = accept
        self.client, self.server = socket.socketpair()
        self.port = 1080
        self.error = None
        self.thread = threading.Thread(target=self.serve, daemon=True)

    def __enter__(self):
        self.thread.start()
        return self

    def __exit__(self, *_):
        self.client.close()
        self.server.close()
        self.thread.join(timeout=2)
        if self.error:
            raise self.error

    def serve(self):
        try:
            self.assert_protocol(self.server)
        except BaseException as exc:
            self.error = exc

    def assert_protocol(self, client):
        if exact(client, 3) != b"\x05\x01\x02":
            raise AssertionError("client did not require username/password")
        client.sendall(b"\x05\x02")
        if exact(client, 2) != b"\x01\x04":
            raise AssertionError("invalid RFC 1929 username header")
        if exact(client, 4) != b"user":
            raise AssertionError("invalid username")
        if exact(client, 1) != b"\x04" or exact(client, 4) != b"pass":
            raise AssertionError("invalid password")
        client.sendall(b"\x01" + (b"\x00" if self.accept else b"\x01"))
        if not self.accept:
            return
        request = exact(client, 10)
        if request[:4] != b"\x05\x01\x00\x01":
            raise AssertionError("invalid CONNECT request")
        client.sendall(b"\x05\x00\x00\x01\x7f\x00\x00\x01\x00\x50")
        if exact(client, 4) != b"ping":
            raise AssertionError("relay payload missing")
        client.sendall(b"pong")


class PamProbeTests(unittest.TestCase):
    def test_authenticated_tcp_relay(self):
        with FakeSocksServer() as server:
            with mock.patch.object(deb_probe.socket, "create_connection", return_value=server.client):
                result = deb_probe.socks_tcp(
                    "127.0.0.1", server.port, "user", "pass", "127.0.0.1", 80, b"ping"
                )
        self.assertEqual(result, b"pong")

    def test_rejected_password_is_required_to_fail(self):
        with FakeSocksServer(accept=False) as server:
            with mock.patch.object(deb_probe.socket, "create_connection", return_value=server.client):
                with self.assertRaisesRegex(deb_probe.AuthenticationRejected, "rejected"):
                    deb_probe.socks_tcp(
                        "127.0.0.1", server.port, "user", "pass", "127.0.0.1", 80, b"ping"
                    )

    def test_empty_and_oversized_credentials_are_rejected_locally(self):
        left, right = socket.socketpair()
        self.addCleanup(left.close)
        self.addCleanup(right.close)
        for username, password in [("", "x"), ("x" * 256, "x"), ("x", "y" * 256)]:
            with self.subTest(username=len(username), password=len(password)):
                with self.assertRaisesRegex(ValueError, "1..255"):
                    deb_probe.authenticate(left, username, password)

    def test_server_must_select_password_authentication(self):
        left, right = socket.socketpair()
        self.addCleanup(left.close)
        self.addCleanup(right.close)
        def reject():
            exact(right, 3)
            right.sendall(b"\x05\xff")
        thread = threading.Thread(target=reject, daemon=True)
        thread.start()
        with self.assertRaisesRegex(deb_probe.AuthenticationRejected, "method"):
            deb_probe.authenticate(left, "user", "pass")
        thread.join(timeout=1)


class ProtocolParsingTests(unittest.TestCase):
    def test_ipv4_ipv6_and_domain_addresses_are_parsed(self):
        cases = [
            (1, socket.inet_pton(socket.AF_INET, "127.0.0.1") + struct.pack("!H", 80), ("127.0.0.1", 80)),
            (4, socket.inet_pton(socket.AF_INET6, "::1") + struct.pack("!H", 443), ("::1", 443)),
            (3, b"\x09localhost" + struct.pack("!H", 1080), ("localhost", 1080)),
        ]
        for atyp, payload, expected in cases:
            with self.subTest(atyp=atyp):
                left, right = socket.socketpair()
                with left, right:
                    right.sendall(payload)
                    self.assertEqual(deb_probe.read_address(left, atyp), expected)

    def test_unknown_address_type_is_rejected(self):
        left, right = socket.socketpair()
        with left, right, self.assertRaisesRegex(RuntimeError, "address type"):
            deb_probe.read_address(left, 9)

    def test_eof_and_failed_request_are_rejected(self):
        left, right = socket.socketpair()
        right.close()
        with left, self.assertRaisesRegex(RuntimeError, "unexpected EOF"):
            deb_probe.exact(left, 1)
        left, right = socket.socketpair()
        with left, right:
            right.sendall(b"\x05\x05\x00\x01")
            with self.assertRaisesRegex(RuntimeError, "code 5"):
                deb_probe.request(left, 1, "127.0.0.1", 80)

    def test_udp_associate_relay_round_trip(self):
        control_client, control_server = socket.socketpair()

        def socks_server():
            try:
                self.assertEqual(exact(control_server, 3), b"\x05\x01\x02")
                control_server.sendall(b"\x05\x02")
                header = exact(control_server, 2)
                exact(control_server, header[1])
                password_size = exact(control_server, 1)[0]
                exact(control_server, password_size)
                control_server.sendall(b"\x01\x00")
                request = exact(control_server, 10)
                self.assertEqual(request[1], 3)
                control_server.sendall(b"\x05\x00\x00\x01\x00\x00\x00\x00\x27\x0f")
            finally:
                control_server.close()

        class Datagram:
            def __enter__(self): return self
            def __exit__(self, *_args): return None
            def settimeout(self, _timeout): pass
            def sendto(self, packet, address):
                self.packet = packet
                self.address = address
            def recvfrom(self, _size):
                return b"\x00\x00\x00\x01\x7f\x00\x00\x01\x46\xa1pong", ("127.0.0.1", 9999)

        thread = threading.Thread(target=socks_server, daemon=True)
        thread.start()
        datagram = Datagram()
        with mock.patch.object(deb_probe.socket, "create_connection", return_value=control_client), \
             mock.patch.object(deb_probe.socket, "socket", return_value=datagram):
            result = deb_probe.socks_udp("127.0.0.1", 1080, "user", "pass", "127.0.0.1", 18081, b"ping")
        thread.join(timeout=1)
        self.assertEqual(result, b"pong")
        self.assertEqual(datagram.address, ("127.0.0.1", 9999))

    def test_statistics_response_requires_schema_one(self):
        class FakeUnixSocket:
            def __init__(self, body):
                self.blocks = [body, b""]
            def __enter__(self): return self
            def __exit__(self, *_args): return None
            def settimeout(self, _timeout): pass
            def connect(self, _path): pass
            def sendall(self, _request): pass
            def recv(self, _size): return self.blocks.pop(0)

        response = b"HTTP/1.1 200 OK\r\nContent-Length: 21\r\n\r\n" + json.dumps({"schema_version": 1}).encode()
        with mock.patch.object(deb_probe.socket, "socket", return_value=FakeUnixSocket(response)):
            self.assertEqual(deb_probe.stats(Path("/run/stats.sock"))["schema_version"], 1)

        bad = b"HTTP/1.1 200 OK\r\n\r\n" + json.dumps({"schema_version": 2}).encode()
        with mock.patch.object(deb_probe.socket, "socket", return_value=FakeUnixSocket(bad)):
            with self.assertRaisesRegex(RuntimeError, "schema"):
                deb_probe.stats(Path("/run/stats.sock"))

        error = b"HTTP/1.1 404 Not Found\r\n\r\n{}"
        with mock.patch.object(deb_probe.socket, "socket", return_value=FakeUnixSocket(error)):
            with self.assertRaisesRegex(RuntimeError, "404"):
                deb_probe.stats(Path("/run/stats.sock"))


class ProbeCliTests(unittest.TestCase):
    def test_cli_dispatches_tcp_udp_stats_and_echo_servers(self):
        with mock.patch.object(deb_probe, "socks_tcp", return_value=b"dante-tcp-relay") as tcp:
            self.assertEqual(deb_probe.main(["tcp", "--username", "u", "--password", "p", "--target-port", "80"]), 0)
            tcp.assert_called_once()
        with mock.patch.object(deb_probe, "socks_udp", return_value=b"dante-udp-relay") as udp:
            self.assertEqual(deb_probe.main(["udp", "--username", "u", "--password", "p", "--target-port", "53"]), 0)
            udp.assert_called_once()
        with mock.patch.object(deb_probe, "stats", return_value={"schema_version": 1}), \
             mock.patch("builtins.print") as output:
            self.assertEqual(deb_probe.main(["stats", "/run/stats.sock"]), 0)
            output.assert_called_once()
        with mock.patch.object(deb_probe, "echo_tcp") as echo:
            self.assertEqual(deb_probe.main(["echo-tcp", "18080"]), 0)
            echo.assert_called_once_with(18080)
        with mock.patch.object(deb_probe, "echo_udp") as echo:
            self.assertEqual(deb_probe.main(["echo-udp", "18081"]), 0)
            echo.assert_called_once_with(18081)

    def test_cli_rejects_changed_relay_payloads(self):
        with mock.patch.object(deb_probe, "socks_tcp", return_value=b"wrong"):
            with self.assertRaisesRegex(RuntimeError, "TCP relay"):
                deb_probe.main(["tcp", "--username", "u", "--password", "p", "--target-port", "80"])
        with mock.patch.object(deb_probe, "socks_udp", return_value=b"wrong"):
            with self.assertRaisesRegex(RuntimeError, "UDP relay"):
                deb_probe.main(["udp", "--username", "u", "--password", "p", "--target-port", "53"])


class GuestGuardTests(unittest.TestCase):
    def test_upgrade_script_requires_all_disposable_vm_guards(self):
        script = (CI / "deb_upgrade.sh").read_text()
        self.assertIn('DANTE_CI_VM:-', script)
        self.assertIn('/etc/dante-ci-disposable', script)
        self.assertIn('/proc/1/comm', script)
        self.assertIn('systemd-detect-virt', script)

    def test_upgrade_script_exercises_required_upgrade_states(self):
        script = (CI / "deb_upgrade.sh").read_text()
        for required in [
            "enabled", "disabled", "masked", "/etc/danted.conf", "/etc/sockd.conf",
            "pam-success", "pam-wrong-password", "pam-account-denied", "udp-relay",
            "rollback", "clean-install", "stats-api", "reboot",
        ]:
            with self.subTest(required=required):
                self.assertIn(required, script)

    def test_udp_fixture_allows_association_and_reply_commands(self):
        script = (CI / "deb_upgrade.sh").read_text()
        socks_rule = script.split("socks pass {", 1)[1].split("}", 1)[0]
        command = next(
            line.split(":", 1)[1].split()
            for line in socks_rule.splitlines()
            if line.strip().startswith("command:")
        )
        self.assertIn("udpassociate", command)
        self.assertIn("udpreply", command)

    def test_statistics_fixture_matches_documented_override(self):
        script = (CI / "deb_upgrade.sh").read_text()
        documentation = (CI.parent / "docs" / "debian-packages.md").read_text()
        documented = documentation.split("```ini\n", 1)[1].split("\n```", 1)[0]
        fixture = script.split(
            "cat > /etc/systemd/system/danted.service.d/statistics.conf <<'EOF'\n", 1
        )[1].split("\nEOF", 1)[0]
        self.assertEqual(fixture, documented)

    def test_statistics_directory_keeps_daemon_user_ownership(self):
        script = (CI / "deb_upgrade.sh").read_text()
        fixture = script.split(
            "cat > /etc/systemd/system/danted.service.d/statistics.conf <<'EOF'\n", 1
        )[1].split("\nEOF", 1)[0]
        self.assertNotIn("RuntimeDirectory=", fixture)
        self.assertIn(
            "ExecStartPre=/usr/bin/install -d -o proxy -g proxy -m 0750 /run/danted-stats",
            fixture,
        )

    def test_upgrade_checks_runtime_state_before_manual_recovery(self):
        script = (CI / "deb_upgrade.sh").read_text()
        enabled = script.index("assert_unit_state enabled")
        disabled = script.index("assert_unit_state disabled")
        masked = script.index("assert_unit_state masked")
        recovery = script.index("systemctl unmask danted.service", masked)
        self.assertIn("assert_unit_activity active", script[enabled:disabled])
        self.assertIn("assert_unit_activity inactive", script[disabled:masked])
        self.assertIn("assert_unit_activity inactive", script[masked:recovery])

    def test_upgrade_preserves_isolated_pam_policy_and_rollback_identity(self):
        script = (CI / "deb_upgrade.sh").read_text()
        for required in [
            "pam-policy.fingerprint", "distro-version", "distro-binary.sha256",
            "assert_rollback_identity",
        ]:
            with self.subTest(required=required):
                self.assertIn(required, script)

    def test_guest_failure_trap_captures_service_and_package_diagnostics(self):
        script = (CI / "deb_upgrade.sh").read_text()
        self.assertIn("collect_failure_diagnostics", script)
        self.assertRegex(script, r"trap ['\"]collect_failure_diagnostics")
        self.assertIn("journalctl -u danted.service", script)
        self.assertIn("dpkg-query", script)
        self.assertNotIn("cat /etc/pam.d/", script)

    def test_upgrade_script_refuses_to_run_without_explicit_vm_marker(self):
        with tempfile.TemporaryDirectory() as tmp:
            candidate = Path(tmp) / "candidate.deb"
            candidate.touch()
            result = subprocess.run(
                ["sh", str(CI / "deb_upgrade.sh"), "upgrade", str(candidate),
                 "debian12", str(CI / "deb_probe.py")],
                text=True,
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
            )
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("DANTE_CI_VM marker is missing", result.stdout)


if __name__ == "__main__":
    unittest.main()
