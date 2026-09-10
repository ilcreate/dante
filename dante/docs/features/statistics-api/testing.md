Last verified against implementation commit: 47fe4ae

# Statistics API testing

## Test strategy

The feature has a standalone C test runner because Dante does not provide a
repository-wide automated test suite. The tests exercise pure transition and
serialization code together with a real Unix socket endpoint. A separate live
check verifies integration with the multi-process `sockd` runtime.

## Prerequisites

Generated configuration headers must exist before compiling the standalone
test. From a clean tree:

```sh
./configure --disable-client
```

The tests require a C compiler, POSIX process APIs, and permission to create a
Unix socket under `${TMPDIR:-/tmp}`. Some restricted sandboxes deny Unix-socket
creation even when ordinary temporary files are allowed.

## Build verification

Run the normal server build first:

```sh
make -j2
```

The verified build completed successfully on macOS. The source tree emits
existing K&R C prototype and deprecated macOS GSSAPI warnings; those warnings
are not introduced by the API test harness.

## Automated feature test

```sh
sockd/tests/run-stats-api-tests.sh
```

The runner:

1. creates a unique temporary executable with `mktemp`;
2. compiles `sockd/tests/stats_api_test.c` and `sockd/statistics.c` with
   `SOCKD_STATS_TEST=1` and server-only definitions;
3. runs the executable;
4. removes the executable through a shell trap.

Success prints:

```text
all stats API tests passed
```

## Automated coverage

The revision 4 implementation was last measured at 94.09% line coverage,
73.91% region coverage, 97.87% function coverage, and 82.77% branch coverage
for `sockd/statistics.c`. The lower region percentage includes production-only
shared-memory wrappers and system-error branches excluded from the standalone
test build.

One macOS/Clang coverage invocation is:

```sh
cc \
  -DHAVE_CONFIG_H=1 \
  -DSTANDALONE_UNIT_TEST=1 \
  -DSOCKD_STATS_TEST=1 \
  -DSOCKS_CLIENT=0 \
  -DSOCKS_SERVER=1 \
  -Iinclude -Ilib -Ilibscompat \
  -fprofile-instr-generate -fcoverage-mapping \
  sockd/tests/stats_api_test.c sockd/statistics.c \
  -o /tmp/dante-stats-api-r4-coverage.bin

LLVM_PROFILE_FILE=/tmp/dante-stats-api-r4.profraw \
  /tmp/dante-stats-api-r4-coverage.bin

xcrun llvm-profdata merge -sparse \
  /tmp/dante-stats-api-r4.profraw \
  -o /tmp/dante-stats-api-r4.profdata

xcrun llvm-cov report \
  /tmp/dante-stats-api-r4-coverage.bin \
  -instr-profile=/tmp/dante-stats-api-r4.profdata \
  sockd/statistics.c
```

Equivalent compiler coverage tooling can be used on other platforms.

## Strict compiler check

The standalone feature also compiles and runs under C17 with additional
diagnostics:

```sh
cc -std=c17 \
  -Wall -Wextra -Wpedantic -Wconversion -Wshadow \
  -Wformat=2 -Wundef \
  -DHAVE_CONFIG_H=1 \
  -DSTANDALONE_UNIT_TEST=1 \
  -DSOCKD_STATS_TEST=1 \
  -DSOCKS_CLIENT=0 \
  -DSOCKS_SERVER=1 \
  -Iinclude -Ilib -Ilibscompat \
  sockd/tests/stats_api_test.c sockd/statistics.c \
  -o /tmp/dante-stats-api-r4-strict.bin

/tmp/dante-stats-api-r4-strict.bin
```

The repository still reports pre-existing diagnostics for legacy K&R
prototype/unused parameters in `sockd_check_ipclatency`, invalid UTF-8 in old
header comments, and configuration macros omitted from this standalone command
under `-Wundef`. Revision 4 adds no new diagnostic in this check.

## Test inventory

`sockd/tests/stats_api_test.c` covers:

| Area | Assertions |
|---|---|
| Initialization | Schema version/revision, start time, zero active gauges |
| Event transitions | Every event updates its documented field |
| Negotiation taxonomy | Success, EOF, error, timeout, and unknown normalization; compatible failure aggregate |
| Request taxonomy | All command/result buckets, unknown normalization, and compatible failure aggregate |
| Session taxonomy | TCP/UDP/unknown start and close values, all close reasons, and legacy error definition |
| Target-connect taxonomy | Attempts, every normalized `errno` class, pending-compatible outcomes, saturation |
| UDP taxonomy | Both directions, receive/forward/error counters, every drop reason, unknown normalization |
| Worker capacity | Process/slot gauges, free-to-total clamping, busy derivation, signed process types, spawn failures |
| Auth/ACL/DNS | Every bounded method/phase/operation/result family and fallback bucket |
| Latency histograms | All seven phases, exact finite bounds, cumulative buckets, zero and above-maximum durations |
| Latency validation | Null/missing timestamps, invalid microseconds, reversed intervals, signed negative times, and overflow-safe saturation |
| Gauge safety | Closing more sessions than active clamps the gauge at zero |
| Counter safety | Addition saturates at `UINT64_MAX` |
| JSON | Metadata, revision 4 details, representative matrices and histograms, string length, clock rollback behavior |
| Buffer bounds | Exact-fit-minus-NUL, zero-size, and small JSON/HTTP buffers return `ENOSPC` |
| HTTP success | `GET /v1/stats`, response headers, and JSON body |
| HTTP errors | 400 malformed/incomplete/version, 404 path, 405 method |
| Path validation | Null, empty, overlong, and existing regular-file paths |
| Socket permissions | Created path is a socket with mode `0600` |
| Real serving | Connect, send, serve, receive, and inspect a real response |
| Delivery race regression | Request bytes delayed after connect still receive a response |
| Cleanup | Close-time and cleanup-only removal of the socket path |

## Manual single-mother integration check

Use a minimal local configuration with a non-production port and run:

```sh
./sockd/sockd \
  -d 0 \
  -f /tmp/dante-stats-e2e.conf \
  -p /tmp/dante-stats-e2e.pid \
  -S /tmp/dante-stats-e2e.sock
```

From another terminal:

```sh
curl --noproxy "*" --silent --show-error \
  --unix-socket /tmp/dante-stats-e2e.sock \
  http://localhost/v1/stats
```

Expected observations:

- HTTP client receives schema version 1, revision 4 JSON;
- repeated requests receive independent responses;
- `started_at` stays constant while `snapshot_time` and uptime advance;
- a TCP connection to the configured SOCKS listener increments
  `client_connections_accepted_total`;
- closing without a valid SOCKS negotiation increments
  `negotiation_failures_total` and one detailed negotiation outcome;
- completed SOCKS flows increment the matching negotiation, request,
  target-connect, first-I/O, session, and authentication histograms;
- a hostname lookup increments `dns_resolver.count`, while a cache hit does
  not create another resolver observation;
- every finite bucket array is nondecreasing and its last entry is at most
  `count`;
- stopping `sockd` removes the Unix socket path.

Do not use a production listener or configuration for this check.

## Multiple-mother integration check

Repeat the live check with:

```sh
./sockd/sockd \
  -d 0 \
  -N 2 \
  -f /tmp/dante-stats-e2e.conf \
  -p /tmp/dante-stats-e2e.pid \
  -S /tmp/dante-stats-e2e.sock
```

Verify that only one monitor logs the statistics listener and that traffic
handled by either mother contributes to the same returned counters.

## Prometheus exporter contract tests to add later

These are requirements for a future exporter and are not current tests:

- schema version 1 is accepted, lifecycle details require revision 2,
  operational details require revision 3, latency histograms require revision
  4, and unknown versions fail explicitly;
- every aggregate and detailed JSON counter maps to a Prometheus
  `Counter`-compatible sample without exporter-side accumulation;
- global and per-protocol active sessions map to gauges;
- all 297 fixed detailed numeric series are exported at zero when absent,
  without discovering labels dynamically;
- latency bounds and sums are converted from microseconds to seconds, finite
  buckets are not accumulated again, and `count` supplies the `+Inf` bucket;
- server restart is observed as a counter reset and changed `started_at`;
- timeouts, connection refusal, malformed JSON, non-200 responses, short reads,
  and content-length mismatches make the scrape fail without stale success;
- metric names and help text remain stable;
- labels remain bounded;
- concurrent Prometheus scrapes do not create an unbounded retry storm against
  the single-client Dante endpoint.

## Security and robustness gaps not covered by current tests

- accepted descriptors above `FD_SETSIZE` and the fixed stack `fd_set` path;
- API connection flooding and repeated one-second slow-client waits;
- shared-lock contention under high TCP/UDP packet rates;
- shared-lock contention from lifecycle latency observations;
- abnormal target-connect worker death leaving attempts without outcomes;
- timing lag between worker slot changes and the next capacity scan;
- peer UID validation;
- socket path replacement races between `lstat()`, `bind()`, `chmod()`, and
  `unlink()`;
- abnormal I/O-worker death leaving `sessions_active` stale;
- fuzzing arbitrary binary and fragmented request-line inputs;
- behavior on platforms that treat Unix socket permissions differently.

These gaps should be addressed or explicitly risk-accepted before treating the
endpoint as hardened for hostile local users.

## Final verification checklist

- `git diff --check` passes.
- Full server build passes.
- Standalone feature tests pass.
- Live HTTP query returns schema version 1.
- A SOCKS-side event changes the expected counter.
- `-N 2` produces one listener and a shared snapshot.
- Socket mode is `0600` and the path disappears on shutdown.
- Documentation still matches the field names and event locations in
  `include/sockd.h` and `sockd/statistics.c`.
