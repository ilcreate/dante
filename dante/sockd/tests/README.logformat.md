# Log format tests (stages 1–6)

After configuring and building Dante, run from the source root:

```sh
make -C sockd -f Makefile -f tests/logformat-tests.mk check-logformat
```

This runs `sockd -V` checks, a daemon with SIGHUP reloads, and a C driver
using the real server initialization, parser, reset and configuration copy
routines. The tests need permission to create loopback and Unix sockets.
They use temporary configurations and stop the daemon when finished.

If the static client library is also built (the default), check that client
configuration rejects the server-only directive:

```sh
make -C sockd -f Makefile -f tests/logformat-tests.mk check-logformat-client
```

To run all configuration checks together after building both drivers:

```sh
python3 sockd/tests/logformat_config_test.py sockd/sockd \
  --lifecycle-binary sockd/logformat_lifecycle_test \
  --client-binary sockd/logformat_client_test -v
```

Configuration tests cover stage 1 and JSON configuration diagnostics from
stage 2 of `docs/plans/logformat-json.md`. The SIGHUP test observes the mother's
effective format; child inheritance and configuration copying are checked
separately by the C driver. The stage 6 runtime suite below additionally observes an existing
worker after live reloads.

Run the serializer and real logger tests after building the server:

```sh
make -C sockd -f Makefile -f tests/logformat-tests.mk check-logjson check-logformat-logger
```

`logjson_test.py` parses the standalone serializer's output with Python's JSON
parser (18 tests). It checks every byte, valid and invalid UTF-8, embedded NUL,
random binary messages, metadata pressure, capacity boundaries, zero and maximum
64-bit counters, nested connection objects, authentication fields, payload, and
`errno`. A standalone sanitizer run is also available:

```sh
cc -std=c99 -Wall -Wextra -Werror -pedantic -fsanitize=address,undefined \
  -g -Iinclude sockd/tests/logjson_test.c lib/logjson.c -o /tmp/dante-logjson-test
python3 sockd/tests/logjson_test.py /tmp/dante-logjson-test -v
```

`logformat_logger_test.py` checks the actual `log.c` implementation (16 tests),
with fixed time/PID and replacements only for clock, allocation, write and syslog
boundaries. It covers all priorities and process roles, typed events, stdout,
errorlog, stderr fallback (including a failed first logfile open), syslog payload
and facilities, debug filtering, both allocation failures, empty/long/binary
messages, EINTR and short writes, named FIFO and concurrent pipe writers.
Raw fixtures retain the original timestamp/PID formatting and syslog prefix.
The wrappers do not require a running syslog service. The allocation tests
exercise both message formatting and JSON output allocation failures.

The JSON path caps each formatting/output allocation at 65536 bytes including
NUL; on allocation failure it uses 2048-byte stack buffers and sets `truncated`.
For participating pipe/FIFO descriptors, all copies use the smallest PIPE_BUF
limit so the bytes agree across destinations. Optional numeric/string properties and address objects may
be omitted as whole properties under space pressure; mandatory keys remain.
The serializer adds exactly one final newline and never cuts an encoded
character or JSON delimiter. Syslog receives the object without that newline
or Dante's text prefix.

Short writes are completed; EINTR is retried up to ten times consecutively.
A permanent I/O error can still leave a partial record on transports that allow
short writes. Atomicity is guaranteed by the pipe/FIFO size constraint under
normal successful writes, not for arbitrary record sizes on every transport.
External syslog truncation is outside the serializer's guarantee.

Run the stage 3 producer and network tests:

```sh
make -C sockd -f Makefile -f tests/logformat-tests.mk check-logformat-iolog
make -C sockd -f Makefile -f tests/logformat-tests.mk check-logformat-iolog-integration
```

`logformat_iolog_test.py` runs a C driver through the real `iolog()` (9 tests).
It checks every operation, the exact event-selection matrix for all rule log
flags, equality of human messages with raw, IPv4/IPv6/domain endpoints, both
proxy directions, username/none authentication, password exclusion, zero ports,
zero IPv6 scope, unknown fields, explicit error provenance, payload gating,
TCP_INFO, large records and allocation failure. The producer's legacy strvis
message buffer also has a bounded allocation in JSON mode; if its text is
shortened, `truncated` stays true even if structured data fits.

`logformat_iolog_integration_test.py` starts actual server processes (10 tests).
It covers TCP CONNECT by IPv4 and domain, IPv6, full duplex BIND, UDP ASSOCIATE,
blocked TCP/domain/UDP requests, a synchronous request error, a refused TCP
connection, payload/iooperation/off, TCP_INFO and raw/JSON parity. Loopback and
Unix sockets must be permitted. IPv6 cases explicitly skip if ::1 is unavailable.
TCP payload checks aggregate the stream by direction; raw/JSON comparisons do
not require TCP read chunk boundaries to match. The deterministic C driver
checks exact event counts for identical calls.

Connection fields and their omission rules are documented in section 4.2 of
`docs/plans/logformat-json.md`. `io_bytes` is an operation's original length,
not the final session byte count. A domain can become a resolved IP by the time
a successful connect event is generated. Error events retain the rule's verdict;
only block/temporary_block force `verdict: block`. This distinguishes connection
failure from rule rejection, while retaining the original human-readable text.
`error.code` is included only when the legacy call uses errno as the diagnostic
source; an explicit message does not prove that the current errno belongs to it.

Run the stage 4 native producer and network tests:

```sh
make -C sockd -f Makefile -f tests/logformat-tests.mk check-logformat-session
make -C sockd -f Makefile -f tests/logformat-tests.mk check-logformat-session-integration
```

`logformat_session_test.py` calls the real `io_delete()` and deferred `siginfo()`
with deterministic monotonic clocks (9 tests). It checks all six close reasons,
timeout kinds, error provenance, side selection, BIND reversal, UDP socket buckets,
control counters, rule filtering, unchanged raw messages and event counts.
TCP_INFO uses the original text, including metadata longer than the legacy
human message buffer, and obeys each rule’s tcpinfo flag.
Snapshots must leave the session and UDP target structures unchanged.
Unknown timestamps are omitted; known zero elapsed times remain present.
The driver uses local sockets; run it with the same socket permissions as integration tests.

`logformat_session_integration_test.py` checks actual TCP/BIND/UDP traffic,
known byte and UDP packet counts, zero traffic, IPv4 and IPv6 UDP buckets,
idle timeout, refused connections, TCP reset, log filtering and raw/JSON close counts.
Repeated snapshots are checked while TCP/UDP sessions remain usable; a UDP
association with no target also produces a snapshot. Administrative, blocked
and other-error close statuses are covered by the C driver: Dante has no
runtime caller for administrative termination outside the Barefoot build.

Stage 4 introduces `session_close` and `session_snapshot`; section 4.3 of the
plan defines top-level numeric fields and `scope`. Control/BIND listener
summaries remain separate existing records. UDP target counts aggregate per
address-family socket bucket, not per remote destination. The snapshots are
cumulative values, not traffic deltas.

Run the stage 5 special logger, fatal diagnostic and runtime tests:

```sh
make -C sockd -f Makefile -f tests/logformat-tests.mk check-logformat-special check-logformat-diagnostic
make -C sockd -f Makefile -f tests/logformat-tests.mk check-logformat-special-integration
```

The special driver exercises real `log.c` paths (12 tests), including actual
SIGUSR1, allocation/stdio/backtrace guards, debug/ring retention and repeated
flush, stack output, escaping/truncation, pipe limits, syslog routing, errno,
short writes and permanent write failure without recursive logging. Its
translation unit enables ring code even in a normal build. Stack assertions
require nonempty output when backtrace is available; otherwise that case skips.
The diagnostic driver runs public warning/fatal helpers and a real flex input
failure in subprocesses (4 tests), checking output and exit status.

Runtime tests validate every line after the first JSON record, including
startup, SIGUSR1, JSON-preserving SIGHUP, SIGTERM, parser errors and unexpected
SIGSEGV. Temporary server processes disable core dumps and are cleaned up.
To exercise the fourth case (a real fatal ring-buffer flush), build a separate
source copy with `--enable-livedebug` and run:

```sh
python3 sockd/tests/logformat_special_integration_test.py sockd/sockd --livedebug -v
```

Without `--livedebug`, the ring runtime case explicitly skips; the other three
cases run in both builds. The ordinary build has no runtime ring buffer.

Signal messages and ring dumps use `event: message`, fixed 16 KiB input/output
buffers and explicit truncation. In signal context FIFO records use the
conservative POSIX pipe minimum without calling fpathconf. Large ring dumps
retain the initial part; newest entries may be omitted. Ordinary stack frames
are JSON; in signal context a message reports that backtrace is unavailable.
The existing critical-only signal syslog exception is preserved. See section
4.5 of the plan for the full contract and limits.

## Final acceptance and CI

Run the complete server acceptance suite from a configured/built tree:

```sh
bash ci/run-server-tests.sh
```

This runs all logging suites, statistics API tests and the JSON relay smoke.
It needs loopback/Unix sockets and process signaling. The default run explicitly
skips client-library checking and the livedebug-only fatal ring case; cover
these in their appropriate builds:

```sh
make -C sockd -f Makefile -f tests/logformat-tests.mk check-logformat-client
# In a separate --disable-client --enable-livedebug build:
bash ci/run-server-tests.sh --livedebug
```

`logformat_runtime_integration_test.py` adds 2 tests (each contains multiple
phases): raw → JSON → JSON-preserving reload → raw → JSON → directive removal
while a TCP session stays usable with the same I/O PID; new sessions at each
phase; and 40 simultaneous UDP associations with at least two I/O workers,
6 KB payloads, 320 I/O records per transport, file/FIFO/stderr validation and
errorlog warning routing. Run just these with `check-logformat-integration`.
The logger suite also tests four concurrent long-record writers to a logfile
plus errorlog=file or errorlog=pipe, requiring 48 valid objects per recipient.

The targetless UDP snapshot test waits for I/O handoff with bounded status
probes before requesting its subsequent snapshot. It sends no target traffic.
Daemon cleanup retries transient process-group EPERM with a finite deadline.

`ci/smoke.py` enables JSON first and uses a dedicated fresh log file. It parses
every physical log line, validates common fields, relays known bytes in both
directions, exercises a refused target, and requires structured accept,
connect, error and matching session-close counters. CLI version output is
separate. The same smoke runs on packaged Linux and macOS binaries.

`ci/build-package.sh` invokes the server suite after compiling the exported
release archive on the Linux/macOS package matrix. `packages.yml` separately
builds the default client libraries and a livedebug server from that same
archive. `ci/build-test-variant.sh` runs those variant checks. `make distdir`
also includes parser/scanner, serializer, all logging tests, statistics test
runner, aggregate server runner and the JSON user documentation.

Measure logger overhead outside CI pass/fail gates:

```sh
make -C sockd -f Makefile -f tests/logformat-tests.mk logformat_benchmark
python3 sockd/tests/logformat_benchmark.py sockd/logformat_benchmark
python3 sockd/tests/logformat_benchmark.py sockd/logformat_benchmark --file /tmp/dante-benchmark.log
```

The benchmark links real log.o and alternates raw/JSON on identical formatted
messages and structured session-close events. It checks small output samples
before timing; seven paired 100,000-event samples follow warmup. `--file`
appends potentially hundreds of MB; there is no fsync. Results describe logger
CPU/buffered writes, not SOCKS throughput. See the [acceptance report](../../docs/features/logformat-json/acceptance.md)
for recorded timings, tested platforms and the actual curl demonstration.

Stages 1–6 are implemented. The complete contract and completion status are in
[the plan](../../docs/plans/logformat-json.md).

Verified on 2026-09-13, macOS arm64: full server and static/dynamic client builds,
8 configuration tests, 18 serializer tests (also ASan/UBSan), 15 logger tests,
9 iolog driver tests, 10 iolog network tests, 9 session producer tests,
10 session network tests (including IPv6, no skips), statistics API
and `ci/smoke.py --relay`. Stage 5 adds 12 special logger tests (also ASan/UBSan
for the actual log.c test translation unit), 4 diagnostic tests and 4 runtime
scenarios, including a real `--enable-livedebug` server. Twenty-two raw
comparisons against `cfe6d18` matched stdout/stderr and status exactly.
`make distdir` includes all new logging test sources.
Thirty-four session raw scenarios were compared byte-for-byte against a stage 3
driver at fixed time/PID, covering close reasons, TCP/BIND/UDP, snapshots and
rule filtering and TCP_INFO. All matched. Ten iolog raw scenarios were compared byte-for-byte
against a driver built against `be177da`: all operations, filter matrix, UDP, BIND, payload enabled,
iooperation only, empty payload, errno fallback, long payload and allocation
failure, at fixed time/PID. Earlier stage 2 checks compared ten common logger
raw scenarios against `f0e827c`. Native hostid and external authentication
services (GSSAPI/PAM/LDAP) were not exercised; proxy and username metadata were
checked with the C driver. These stage 1–5 runs did not exercise Linux or a live syslog service.
Stage 6 Linux results are recorded in the acceptance report; syslog remains
verified at the API boundary, not through an external service.

The shipped parser was regenerated with GNU Bison 3.8.2 and the scanner with
Apple flex 2.6.4, using the rules in `lib/Makefile.am` and `LC_ALL=C` for the
legacy source encoding. Trailing whitespace in the generated scanner was
removed.
