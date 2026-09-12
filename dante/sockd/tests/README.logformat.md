# Log format tests (stages 1 and 2)

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

To run all checks together after building both drivers:

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
parser (10 tests). It checks every byte, valid and invalid UTF-8, embedded NUL,
random binary messages, metadata pressure, capacity boundaries, zero and maximum
64-bit counters, and `errno`. A standalone sanitizer run is also available:

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
limit so the bytes agree across destinations. Optional numeric properties may
be omitted as whole properties under space pressure; mandatory keys remain.
The serializer adds exactly one final newline and never cuts an encoded
character or JSON delimiter. Syslog receives the object without that newline
or Dante's text prefix.

Short writes are completed; EINTR is retried up to ten times consecutively.
A permanent I/O error can still leave a partial record on transports that allow
short writes. Atomicity is guaranteed by the pipe/FIFO size constraint under
normal successful writes, not for arbitrary record sizes on every transport.
External syslog truncation is outside the serializer's guarantee.

Stage 2 only converts ordinary `slog()`/`vslog()` output and adds `slogevent()`
for future producers. Existing `iolog()` text currently appears as a `message`
event; structured connection fields and production counters await stages 3–4.
`signalslog()`, ring-buffer dumps and stack traces retain their existing paths
until stage 5, so they can still emit raw after JSON activation. This is not yet
the complete JSON logging feature.

Verified on 2026-09-13, macOS arm64: full server and static/dynamic client builds,
8 configuration tests, 10 serializer tests (also ASan/UBSan), 15 logger tests,
statistics API tests and `ci/smoke.py --relay`. `make distdir` also passed and
includes the new module, header and logging tests. Additionally, ten raw scenarios
were compared byte-for-byte with a driver built against commit `f0e827c` at
fixed time/PID: routing, syslog, stderr fallback, debug filtering, EINTR, short
writes, empty/long messages and two allocation-failure cases. Linux and live
syslog-service behavior were not exercised in this session.

The shipped parser was regenerated with GNU Bison 3.8.2 and the scanner with
Apple flex 2.6.4, using the rules in `lib/Makefile.am` and `LC_ALL=C` for the
legacy source encoding. Trailing whitespace in the generated scanner was
removed.
