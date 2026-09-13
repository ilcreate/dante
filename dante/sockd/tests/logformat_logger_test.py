#!/usr/bin/env python3
"""Parse real logger output; check transports, compatibility and failures."""
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import threading
import unittest

BINARY = str(Path(sys.argv.pop(1)).resolve())


class LoggerTest(unittest.TestCase):
    def run_logger(self, case, format="json", pipe=False):
        env = dict(os.environ, TZ="UTC")
        if pipe:
            return subprocess.run([BINARY, format, case], capture_output=True,
                                  env=env, timeout=15)
        with tempfile.TemporaryFile() as output:
            p = subprocess.run([BINARY, format, case], stdout=output,
                               stderr=subprocess.PIPE, env=env, timeout=15)
            output.seek(0)
            p.stdout = output.read()
            return p

    def objects(self, data):
        self.assertTrue(data.endswith(b"\n"), data)
        return [json.loads(line) for line in data.splitlines()]

    def test_common_fields_and_levels(self):
        p = self.run_logger("routing")
        self.assertEqual(p.returncode, 0, p.stderr)
        events = self.objects(p.stdout)
        self.assertEqual(len(events), 8)
        self.assertEqual(self.objects(p.stderr), events[:5])
        for i, event in enumerate(events):
            self.assertEqual(event, dict(schema_version=1, timestamp="1789257600.123456",
                level=["emergency", "alert", "critical", "error", "warning", "notice", "info", "debug"][i],
                pid=1234, process="mother", program="danted", event="message",
                message=f"level {i}", truncated=False))

    def test_typed_event_and_process_roles(self):
        p = self.run_logger("typed")
        self.assertEqual(p.returncode, 0, p.stderr)
        events = self.objects(p.stdout)
        self.assertEqual([e["process"] for e in events],
                         ["mother", "monitor", "negotiate", "request", "io"])
        for event in events:
            self.assertEqual(event["event"], "session_snapshot")
            self.assertEqual(event["message"], "session data")
            self.assertEqual(event["duration_us"], 0)
            self.assertEqual(event["client_bytes_read"], 2**64 - 1)
            self.assertNotIn("target_bytes_read", event)
        raw = self.run_logger("typed", format="raw")
        self.assertEqual(len(raw.stdout.splitlines()), 5)
        self.assertTrue(all(line.endswith(b": info: session data") for line in raw.stdout.splitlines()))

    def test_escaping_and_embedded_nul(self):
        p = self.run_logger("bytes")
        event, = self.objects(p.stdout)
        self.assertEqual(event["message"], 'quote" slash\\ newline\n tab\t é😀\0ÿÀ\x80')
        self.assertFalse(event["truncated"])

    def test_long_and_allocation_failure(self):
        for case in ("long", "alloc1", "alloc2"):
            with self.subTest(case=case):
                p = self.run_logger(case)
                self.assertEqual(p.returncode, 0, p.stderr)
                event, = self.objects(p.stdout)
                self.assertTrue(event["truncated"])
                self.assertGreater(len(event["message"]), 0)
                self.assertLessEqual(len(p.stdout), 65535)
                self.assertEqual(set(event["message"]), {"x"})

    def test_pipe_limit(self):
        p = self.run_logger("long", pipe=True)
        event, = self.objects(p.stdout)
        self.assertTrue(event["truncated"])
        readfd, writefd = os.pipe()
        try:
            self.assertLessEqual(len(p.stdout), os.fpathconf(writefd, "PC_PIPE_BUF"))
        finally:
            os.close(readfd)
            os.close(writefd)

    def test_debug_filter_and_stderr_fallback(self):
        p = self.run_logger("debug")
        self.assertEqual(len(self.objects(p.stdout)), 7)
        p = self.run_logger("fallback")
        self.assertEqual(p.stdout, b"")
        self.assertEqual(len(self.objects(p.stderr)), 5)

    def test_errorlog_fifo_limits_all_copies(self):
        p = self.run_logger("routing-long")
        self.assertEqual(p.returncode, 0, p.stderr)
        self.assertEqual(p.stdout, p.stderr)
        event, = self.objects(p.stdout)
        self.assertTrue(event["truncated"])
        readfd, writefd = os.pipe()
        try:
            self.assertLessEqual(len(p.stderr), os.fpathconf(writefd, "PC_PIPE_BUF"))
        finally:
            os.close(readfd)
            os.close(writefd)

    def test_stderr_limit_with_unopened_logfile(self):
        p = self.run_logger("fallback-empty")
        self.assertEqual(p.returncode, 0, p.stderr)
        self.assertEqual(p.stdout, b"")
        event, = self.objects(p.stderr)
        self.assertTrue(event["truncated"])
        readfd, writefd = os.pipe()
        try:
            self.assertLessEqual(len(p.stderr), os.fpathconf(writefd, "PC_PIPE_BUF"))
        finally:
            os.close(readfd)
            os.close(writefd)

    def test_empty_message(self):
        event, = self.objects(self.run_logger("empty").stdout)
        self.assertEqual(event["message"], "")

    def test_eintr_and_short_write(self):
        for case in ("eintr", "short"):
            with self.subTest(case=case):
                p = self.run_logger(case)
                self.assertEqual(p.returncode, 0, p.stderr)
                self.assertEqual(len(self.objects(p.stdout)), 8)

    def test_syslog_facilities_and_payload(self):
        p = self.run_logger("syslog")
        entries = []
        for line in p.stdout.splitlines():
            marker, priority, payload = line.split(b" ", 2)
            self.assertEqual(marker, b"SYSLOG")
            entries.append((int(priority), json.loads(payload)))
        self.assertEqual(len(entries), 13)
        import syslog
        for priority, event in entries:
            self.assertIn(priority & ~7, (syslog.LOG_LOCAL0, syslog.LOG_LOCAL1))
            self.assertEqual(event["message"], f"level {priority & 7}")
            if priority & ~7 == syslog.LOG_LOCAL1:
                self.assertLessEqual(priority & 7, 4)

    def test_fifo_limit(self):
        with tempfile.TemporaryDirectory() as tmp:
            fifo = str(Path(tmp) / "log.fifo")
            os.mkfifo(fifo)
            fd = os.open(fifo, os.O_RDWR | os.O_NONBLOCK)
            try:
                p = subprocess.run([BINARY, "json", "long"], stdout=fd,
                                   stderr=subprocess.PIPE, timeout=15)
                self.assertEqual(p.returncode, 0, p.stderr)
                data = os.read(fd, 65536)
                event, = self.objects(data)
                self.assertTrue(event["truncated"])
                self.assertLessEqual(len(data), os.fpathconf(fd, "PC_PIPE_BUF"))
            finally:
                os.close(fd)

    def test_concurrent_pipe_writers(self):
        readfd, writefd = os.pipe()
        processes = []
        try:
            for _ in range(4):
                processes.append(subprocess.Popen([BINARY, "json", "repeat"],
                    stdout=writefd, stderr=subprocess.PIPE))
            os.close(writefd)
            writefd = -1
            with os.fdopen(readfd, "rb") as stream:
                data = stream.read()
            readfd = -1
            for p in processes:
                _, errors = p.communicate(timeout=15)
                self.assertEqual(p.returncode, 0, errors)
            events = self.objects(data)
            self.assertEqual(len(events), 48)
            for i in range(12):
                self.assertEqual(sum(e["message"].startswith(f"{i} ") for e in events), 4)
            self.assertTrue(all(e["truncated"] for e in events))
        finally:
            for fd in (readfd, writefd):
                if fd != -1:
                    os.close(fd)
            for p in processes:
                if p.poll() is None:
                    p.kill()
                p.communicate()

    def test_raw_syslog(self):
        import syslog
        p = self.run_logger("syslog", format="raw")
        levels = ["emergency", "alert", "critical", "error", "warning", "notice", "info", "debug"]
        lines = []
        for i, level in enumerate(levels):
            for facility in ([syslog.LOG_LOCAL1, syslog.LOG_LOCAL0] if i <= 4 else [syslog.LOG_LOCAL0]):
                lines.append(f"SYSLOG {facility | i} {level}: level {i}\n\n".encode())
        self.assertEqual(p.stdout, b"".join(lines))

    def test_concurrent_long_file_and_errorlog(self):
        for error_pipe in (False, True):
            with self.subTest(error_pipe=error_pipe), tempfile.TemporaryFile(mode="a+b") as output, \
                    tempfile.TemporaryFile(mode="a+b") as errorfile:
                processes = []
                captured = []
                writefd = None
                reader = None
                errfd = errorfile.fileno()
                if error_pipe:
                    readfd, writefd = os.pipe()
                    errfd = writefd

                    def drain():
                        with os.fdopen(readfd, "rb") as stream:
                            captured.append(stream.read())

                    reader = threading.Thread(target=drain, daemon=True)
                    reader.start()
                try:
                    for _ in range(4):
                        processes.append(subprocess.Popen([BINARY, "json", "repeat-routing"],
                                                          stdout=output, stderr=errfd))
                    for process in processes:
                        self.assertEqual(process.wait(timeout=15), 0)
                finally:
                    for process in processes:
                        if process.poll() is None:
                            process.kill()
                        process.wait(timeout=5)
                    if writefd is not None:
                        os.close(writefd)
                    if reader:
                        reader.join(timeout=5)
                        self.assertFalse(reader.is_alive(), "errorlog reader did not finish")
                output.seek(0)
                errorfile.seek(0)
                streams = (output.read(), captured[0] if error_pipe else errorfile.read())
                parsed = [self.objects(data) for data in streams]
                for events in parsed:
                    self.assertEqual(len(events), 48)
                    self.assertTrue(all(e["truncated"] and e["level"] == "warning" for e in events))
                    for sequence in range(12):
                        self.assertEqual(sum(e["message"].startswith(f"{sequence} ") for e in events), 4)
                # Concurrent workers may reach recipients in different orders.
                self.assertEqual(sorted(e["message"] for e in parsed[0]),
                                 sorted(e["message"] for e in parsed[1]))

    def test_raw_fixture(self):
        p = self.run_logger("routing", format="raw")
        levels = ["emergency", "alert", "critical", "error", "warning", "notice", "info", "debug"]
        lines = [f"Sep 13 00:00:00 (1789257600.123456) danted[1234]: {level}: level {i}\n".encode()
                 for i, level in enumerate(levels)]
        self.assertEqual(p.stdout, b"".join(lines))
        self.assertEqual(p.stderr, b"".join(lines[:5]))


if __name__ == "__main__":
    unittest.main()
