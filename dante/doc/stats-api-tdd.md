Last verified against commit: 5433322b4c52367df1a0cdc3f7d4c3e6f6236767
Latest v1 revision 2 cycle verified against commit:
11448e34dc558a425748f56c832c366baa5051c2

# Statistics API TDD evidence

## Scope and user journeys

The implementation was derived from the requirement to expose Dante server
statistics through a local API while preserving the server's multi-process
model.

1. An operator can enable a read-only API with `sockd -S <unix-socket>`.
2. A local collector can request `GET /v1/stats` and receive a versioned JSON
   snapshot with counters, gauges, timestamps, and server version.
3. Counters updated by mother, negotiation, request, and I/O processes remain
   visible through one process-shared snapshot.
4. Invalid methods, paths, requests, paths that are too long, and output-buffer
   exhaustion fail predictably.
5. The endpoint is owner-only (`0600`), does not overwrite regular files, and
   removes its socket when the monitor exits.

## RED evidence

The first test-only checkpoint is commit `026a29e`. Running
`sockd/tests/run-stats-api-tests.sh` at that checkpoint failed at compile time
because `sockd_stats_t`, the event API, JSON formatter, HTTP contract, and Unix
socket endpoint did not yet exist.

Two later regression tests were also observed failing before their fixes:

- monitor shutdown cleanup called an absent `sockd_stats_api_cleanup()`;
- a delayed request after `connect()` caused `sockd_stats_api_serve()` to return
  `EAGAIN`, reproducing an empty first response from `curl`.

## GREEN evidence

| Check | Command | Result |
|---|---|---|
| Full project build | `make -j2` | Passed; existing legacy C and macOS GSSAPI deprecation warnings remain |
| Unit and Unix-socket integration tests | `sockd/tests/run-stats-api-tests.sh` | Passed |
| Statement coverage of the new statistics module | `llvm-cov report` | 84.81% lines, 84.72% regions, 92.31% functions |
| Configuration validation | `sockd -V -f <test-config> -S <socket>` | Passed |
| Live API | `curl --unix-socket <socket> http://localhost/v1/stats` | Three consecutive valid JSON responses |
| Cross-process update | TCP connect to the live SOCKS listener, then query API | `accepted_total=1`, `negotiation_failures_total=1` |
| Multiple mother processes | Start with `-N 2`, query and update statistics | One API listener; shared counters increased by one |
| Shutdown cleanup | Stop live `sockd`, then inspect socket path | Socket removed |

The committed tests cover counter and gauge transitions, saturation, JSON
schema and buffer limits, HTTP 200/400/404/405 behavior, endpoint validation,
permissions, delayed request delivery, serving a real Unix-socket request, and
cleanup.

## Known follow-ups

- Statistics updates use the existing process-shared file lock. Benchmark this
  under high connection and I/O churn before enabling the endpoint on a very
  latency-sensitive deployment.
- `sessions_active` can remain high if an I/O worker is killed before its normal
  deletion path; startup resets it, but automatic reconciliation is not part of
  this change.
- The compact monitor endpoint handles one connection at a time and waits at
  most one second for the first request bytes. It is intended for a trusted
  local collector, not as a general-purpose HTTP server.
- Counters intentionally have no high-cardinality labels. Per-rule or
  per-destination dimensions should be designed separately if needed.

## Version 1 revision 2 extension

### Scope

This cycle extended the existing endpoint without introducing `/v2` or
removing/changing any original JSON path. It added negotiation outcomes, a
bounded command/result request matrix, per-protocol session lifecycle values,
and normalized close reasons. The resulting 57 detailed series form a stable
input contract for a future Prometheus exporter.

The cycle deliberately did not claim target TCP-connect completion or detailed
UDP datagram/drop accounting. The existing lifecycle hook observes admission
to I/O, while accurate connect and UDP telemetry require later hooks.

### RED evidence

The test-only checkpoint is commit
`9817b401d148266490400438783522e90fa2e0c6`. After the normal generated build
configuration was present, `sockd/tests/run-stats-api-tests.sh` failed at
compile time because `schema_revision`, the bounded taxonomies, detailed
storage, and typed recorder APIs did not exist.

This was the intended contract failure. An earlier attempt made before running
`./configure --disable-client` failed only because `include/autoconf.h` was
absent and was not counted as RED evidence.

### GREEN evidence

The implementation checkpoint is commit
`5c6c8a849419704379f110e0cae76cb365a8b5ac`. It added shared state, typed pure
mutations, one-lock process-shared wrappers, producer hooks, and revision 2 JSON
serialization. The feature test runner and `make -j2` both passed.

### Refactor evidence

The refactor checkpoint is commit
`11448e34dc558a425748f56c832c366baa5051c2`. It replaced repetitive serializer
macros with a format-checked bounded append helper and expanded tests for every
request/close status plus exact output-buffer boundaries.

| Check | Result |
|---|---|
| Feature unit and Unix-socket integration runner | Passed |
| Full server-only build (`make -j2`) | Passed |
| Strict C17 compile and test run | Passed; pre-existing legacy prototype/unused-parameter, header encoding, and standalone `-Wundef` diagnostics remain |
| `sockd/statistics.c` line coverage | 90.55% |
| Function coverage | 96.00% |
| Branch coverage | 80.15% |
| Region coverage | 74.57%; includes production-only shared wrappers and system-error branches |

The tests now cover aggregate compatibility, every bounded taxonomy,
saturation, gauge underflow protection, unknown-value normalization, JSON
revision/details, exact buffer boundaries, HTTP status behavior, real Unix
socket serving, permissions, delayed delivery, and cleanup.
