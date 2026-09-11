Last verified against commit: 35d9ebee3f37fead6099eb4625f85bb63e2b455a
Latest v1 revision 2 cycle verified against commit:
11448e34dc558a425748f56c832c366baa5051c2
Latest v1 revision 3 cycle verified against commit:
35a8cd405789be5bedcd870ca52f2a9a6528500b
Latest v1 revision 4 cycle verified against commit:
47fe4aeba12c59dc770da74d9516cd741302392a
Latest v1 revision 5 cycle verified against commit:
35d9ebee3f37fead6099eb4625f85bb63e2b455a

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

## Version 1 revision 3 operational extension

### Scope

This cycle added true target-connect outcomes, directional UDP datagrams and
drops, worker capacity, authentication outcomes, ACL decisions, and DNS
backend-query outcomes. All keys and labels are compile-time bounded; no
identity, address, rule, PID, raw `errno`, or free-form error becomes a metric
dimension. Revision 1 and 2 JSON paths remain unchanged.

### RED evidence

The test-only checkpoint is commit
`8e397f4b7f4d68e8dbb4941e75efe8a2fc427fb2`. The feature runner failed to
compile because the revision 3 structures, enums, mutation functions, and JSON
fields did not exist. This was the intended missing-contract failure.

### GREEN evidence

The implementation checkpoint is commit
`0bbec19d371054d87ce3aef2481b41e1c65eb10f`. It added the shared representation,
bounded normalizers, producer hooks, and revision 3 serialization. The feature
runner and full `make -j2` build passed.

### Refactor evidence

The refactor checkpoint is commit
`35a8cd405789be5bedcd870ca52f2a9a6528500b`. It expanded taxonomy branch tests,
made all shared statistics wrappers preserve caller `errno`, classified
post-`sendto()` UDP forwarding failures, and prevented a failed proxy-chain
connect with a zero `errno` from being counted as success.

| Check | Result |
|---|---|
| Feature unit and Unix-socket integration runner | Passed |
| Full server-only build (`make -j2`) | Passed; only existing legacy C/GSSAPI warnings |
| Strict C17 compile and test run | Passed; only pre-existing repository diagnostics |
| `sockd/statistics.c` line coverage | 94.18% |
| Function coverage | 97.73% |
| Branch coverage | 83.90% |
| Region coverage | 73.61%; production-only shared wrappers remain excluded from the standalone build |

The revision 3 contract contains 100 new fixed numeric series and 157 detailed
series in total. Target attempts may temporarily exceed outcomes while a
nonblocking connect is pending. UDP receive errors count failed receive calls,
worker capacity is a scan-time snapshot, and resolver cache hits are excluded.

## Version 1 revision 4 latency extension

### Scope

This cycle added seven fixed cumulative latency histograms for negotiation,
request processing, target connect, first I/O, full accepted-to-close session,
resolver backend, and executed authentication intervals. Each histogram has
18 finite microsecond buckets, a count, and a saturating microsecond sum. The
revision adds 140 fixed numeric series, bringing the detailed total to 297.

### RED evidence

The test-only checkpoint is commit
`4957bf2ee26760385777e0f1a8bf509cb18b9dc7`. The feature runner failed at
compile time because the revision 4 schema constant, latency taxonomy,
histogram storage, bounds, and observation functions did not exist. Tests were
therefore committed before production code.

### GREEN evidence

The implementation checkpoint is commit
`dd4539b2b20d4880db1aee1787d7acd90cd14c4a`. It added overflow-safe duration
conversion, saturating cumulative histograms, JSON serialization, and the
negotiation, request, target-connect, first-I/O, session, DNS, and
authentication producer hooks. The feature runner and full server-only build
passed.

### Refactor and final verification

The refactor checkpoint is commit
`47fe4aeba12c59dc770da74d9516cd741302392a`. It aligned session duration with
the existing full accepted-to-close lifecycle and made the implicit `+Inf`
meaning of `count` explicit.

| Check | Result |
|---|---|
| Feature unit and Unix-socket integration runner | Passed |
| Full server-only build (`make -j2`) | Passed; only existing legacy C/GSSAPI warnings |
| Strict C17 compile and test run | Passed; only pre-existing repository diagnostics |
| AddressSanitizer and UndefinedBehaviorSanitizer | Passed with no findings |
| `sockd/statistics.c` line coverage | 94.09% |
| Function coverage | 97.87% |
| Branch coverage | 82.77% |
| Region coverage | 73.91%; production-only shared wrappers remain excluded from the standalone build |

The live revision 4 check returned valid JSON on repeated requests and
observed all seven histogram families. A hostname request recorded one
successful DNS backend query and one resolver duration. SOCKS negotiation and
target connection reached a local HTTP server, which logged the request and a
200 response. On this macOS host, Dante's existing descriptor-passing path
reset `SO_RCVBUF` to zero, so the client saw an empty response after successful
upstream handling; this legacy platform limitation was logged by Dante and was
not introduced by the latency change. The API socket was removed on shutdown.

Tests cover inclusive exact boundaries, cumulative bucket ordering, zero and
above-largest durations, invalid/missing/reversed timestamps, signed-time and
arithmetic overflow cases, saturating count/sum/buckets, JSON shape, and the
existing HTTP/Unix-socket contract.

## Version 1 revision 5 protocol and traffic extension

### Scope

This cycle added six byte counters by TCP/UDP/unknown and traffic direction,
twelve lifecycle values by client IPv4/IPv6/unknown family, three UDP
datagram-size histograms, and 45 I/O outcome counters by protocol and socket
side. The 111 new fixed numeric series bring the detailed total to 408. No DNS
cache, worker lifecycle/backpressure, SOCKS protocol-detail, or control-plane
task was started.

### RED evidence

The initial test-only checkpoint is commit
`2e0850256d61ea403c5d751bdc4b0b7d1062f3d1`. Its valid standalone build failed
because revision 5 types, fields, bounds, mutation functions, and changed
session signatures did not exist. The tests specified normalization,
saturation, exact UDP bucket boundaries, cumulative JSON buckets, compatibility
aggregates, and maximum-size snapshot serialization.

A live UDP round-trip then found that the original generic I/O aggregation did
not publish per-target UDP bytes. Regression checkpoint
`c5dd5d463fe46bf368ca55cc94c7152b56487369` failed at compile time against a
new pure forwarded-UDP byte transition before its production implementation
was added.

### GREEN and refactor evidence

Implementation checkpoint `6e288ab1ff4ec591f08526c364862d20138e2f5b`
added the revision 5 shared storage, JSON contract, and TCP/UDP producers.
Refactor checkpoint `e4b36ac906614e793cc0b0ac1bd4ba00f90ab066`
changed the UDP histogram hot path from up to 14 bucket writes to one raw
finite-bucket write; serialization still returns cumulative counts.

Regression fix `35d9ebee3f37fead6099eb4625f85bb63e2b455a` records
forwarded UDP count and the actual socket read/write byte deltas under one lock
and excludes UDP from the unreliable legacy aggregate path. The same path
continues to update all four compatible byte counters.

### Final verification

| Check | Result |
|---|---|
| Feature unit and Unix-socket integration runner | Passed |
| Full build (`make -j2`) | Passed |
| Strict C17 compile and test run | Passed; only pre-existing repository diagnostics |
| AddressSanitizer and UndefinedBehaviorSanitizer | Passed with no findings |
| `sockd/statistics.c` line coverage | 94.42% |
| Function coverage | 98.18% |
| Branch coverage | 82.36% |
| Region coverage | 72.42%; production-only shared wrappers remain excluded from the standalone build |

The live schema revision 5 test completed one SOCKS5 UDP ASSOCIATE round-trip
through a local echo server. The API reported one receive and one forward in
each direction, client-to-target size 27 bytes, target-to-client size 17 bytes,
directional UDP byte totals of 27 and 17, compatible aggregate read/write
totals of 27/17/17/27, and one completed IPv4 session. Dante remained able to
serve repeated API requests and the temporary listener was shut down after
verification.
