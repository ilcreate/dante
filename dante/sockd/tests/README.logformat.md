# Log format tests (stages 1–4)

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
separately by the C driver. It does not observe the format inside an existing
worker after a live reload.

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

`logformat_logger_test.py` checks the actual `log.c` implementation (15 tests),
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

`signalslog()`, ring-buffer dumps and stack traces retain their existing paths
until stage 5 and can still emit raw after JSON activation. The full JSON
logging feature is not yet complete.

Verified on 2026-09-13, macOS arm64: full server and static/dynamic client builds,
8 configuration tests, 18 serializer tests (also ASan/UBSan), 15 logger tests,
9 iolog driver tests, 10 iolog network tests, 9 session producer tests,
10 session network tests (including IPv6, no skips), statistics API
and `ci/smoke.py --relay`. `make distdir` includes all new logging test sources.
Thirty-four session raw scenarios were compared byte-for-byte against a stage 3
driver at fixed time/PID, covering close reasons, TCP/BIND/UDP, snapshots and
rule filtering and TCP_INFO. All matched. Ten iolog raw scenarios were compared byte-for-byte
against a driver built against `be177da`: all operations, filter matrix, UDP, BIND, payload enabled,
iooperation only, empty payload, errno fallback, long payload and allocation
failure, at fixed time/PID. Earlier stage 2 checks compared ten common logger
raw scenarios against `f0e827c`. Native hostid and external authentication
services (GSSAPI/PAM/LDAP) were not exercised; proxy and username metadata were
checked with the C driver. Linux and a live syslog service were not exercised.

The shipped parser was regenerated with GNU Bison 3.8.2 and the scanner with
Apple flex 2.6.4, using the rules in `lib/Makefile.am` and `LC_ALL=C` for the
legacy source encoding. Trailing whitespace in the generated scanner was
removed.
