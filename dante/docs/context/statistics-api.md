Last verified against implementation commit: 47fe4ae

# Statistics API implementation context

## Purpose

The statistics API exposes a process-wide snapshot of Dante server activity to
a trusted local collector. It is the integration boundary intended for a future
Prometheus exporter. The exporter itself is not part of the current feature.

## Implementation status

The server-side collection and read-only HTTP endpoint are implemented. The
endpoint is optional and is enabled with the `sockd` command-line option `-S`.
Statistics collection currently runs even when the endpoint is disabled.

Implemented behavior includes:

- process-shared aggregate counters and active-session gauge;
- JSON schema version 1, revision 4, preserving every original aggregate
  field;
- fixed-cardinality negotiation, request, session, target-connect, UDP,
  worker-capacity, authentication, ACL, DNS, and latency details suitable for
  direct exporter translation;
- `GET /v1/stats` over a Unix domain stream socket;
- HTTP/1.0 and HTTP/1.1 request-line parsing;
- owner-only socket mode `0600`;
- one API listener when Dante runs with multiple mother processes;
- socket cleanup during normal monitor shutdown and signal-driven exit.

There is no Prometheus text endpoint, exporter binary, authentication protocol,
TCP listener, TLS, rate limiting, or per-rule/per-destination metric dimension.

## Entry points

- `sockd/sockd.c`: `serverinit()` parses `-S`; the mother process records
  accepted and dropped clients.
- `sockd/shmem.c`: `shmem_setup()` initializes the shared snapshot.
- `sockd/monitor.c`: `run_monitor()` owns and serves the Unix socket.
- `sockd/statistics.c`: counter mutation, snapshots, serialization, HTTP
  parsing, socket creation, request serving, and cleanup.
- `sockd/sockd_negotiate.c`: `run_negotiate()` records terminal negotiation
  outcomes and durations.
- `sockd/sockd_request.c`: records request results, durations, and
  direct/upstream-proxy target-connect attempts/durations.
- `sockd/sockd_io.c`: records sessions, byte counts, closures, errors,
  deferred target-connect outcomes, first-I/O latency, and full session
  duration.
- `sockd/dante_udp.c`: records datagrams, receive errors, and classified drops.
- `sockd/sockd_child.c`: publishes worker capacity and creation failures.
- `sockd/accesscheck.c`, `sockd/rule.c`, and `lib/hostcache.c`: record executed
  authentication checks, ACL decisions, resolver backend results, and the
  corresponding uncached authentication/resolver durations.

## Relevant files

- `include/sockd.h` — `sockd_stats_t`, `sockd_stat_event_t`, schema constant,
  configuration field, shared-memory placement, and function declarations.
- `sockd/statistics.c` — statistics and API implementation.
- `sockd/monitor.c` — API event-loop integration.
- `sockd/shmem.c` — shared snapshot initialization.
- `sockd/sockd.c`, `sockd/sockd_negotiate.c`, `sockd/sockd_request.c`,
  `sockd/sockd_io.c` — metric producers.
- `sockd/sockd_util.c` — signal-exit socket cleanup.
- `sockd/tests/stats_api_test.c` — unit and Unix-socket integration tests.
- `sockd/tests/run-stats-api-tests.sh` — standalone test build and runner.
- `doc/sockd.8` — operator-facing command-line and endpoint documentation.
- `doc/stats-api-tdd.md` — original TDD evidence.
- `docs/features/statistics-api/README.md` — API and exporter-facing contract.
- `docs/features/statistics-api/metrics.md` — canonical metric semantics,
  label sets, and suggested Prometheus names.
- `docs/features/statistics-api/architecture.md` — component design.
- `docs/features/statistics-api/testing.md` — verification procedure.

## Architecture

All process types inherit the same mapped `sockscf.shmeminfo` region. Producers
mutate its embedded `sockd_stats_t`. The monitor belonging to the main mother
process copies a locked snapshot, converts it to JSON, and responds through the
Unix socket. Other monitor processes do not create an API listener.

The feature does not introduce a new daemon or external dependency. See
`docs/features/statistics-api/architecture.md` for the process flow.

## Data flow

1. `shmem_setup()` initializes `sockd_stats_t` with schema version 1 and the
   current wall-clock start time.
2. Mother, negotiation, request, and I/O processes call the typed
   `sockd_stats_update*()` wrappers at lifecycle events.
3. Updates acquire an exclusive byte-range lock on `sockscf.shmemfd`, mutate
   the shared structure, and release the lock.
4. `run_monitor()` waits for the API listener alongside its mother control fd.
5. On a connection, the monitor copies a consistent snapshot under the same
   lock and passes it to `sockd_stats_api_serve()`.
6. The request is parsed, the snapshot is serialized by `sockd_stats_json()`,
   and a single HTTP response is sent before closing the connection.
7. A future exporter can scrape the endpoint and translate the JSON fields to
   Prometheus metric families without linking to Dante internals.

## Important data structures

### `sockd_stats_t`

Stored inside the anonymous `sockscf.shmeminfo` mapping. It contains:

- `schema_version`, `schema_revision`, and `started_at` metadata;
- monotonic, saturating `uint64_t` counters;
- the original aggregate counters/gauge;
- five negotiation outcome counters;
- a four-command by eight-result request matrix;
- per-protocol session lifecycle values for TCP, UDP, and unknown protocols;
- eight normalized session close-reason counters.
- target-connect attempts and seven terminal result counters;
- three UDP directions with receive/forward/error and drop-reason counters;
- four worker types with capacity gauges and spawn-failure counters;
- bounded authentication, ACL-phase, and DNS operation/result matrices.
- seven fixed latency histograms with 18 cumulative finite buckets, count, and
  microsecond sum per histogram.

### Mutation APIs

`sockd_stat_event_t` and `sockd_stats_add()` retain the original aggregate
mutation interface. Typed `sockd_stats_add_negotiation()`,
`sockd_stats_add_request()`, `sockd_stats_add_session_started()`,
`sockd_stats_add_session_closed()`, and the revision 3 typed functions update
bounded details. Revision 4 adds `sockd_stats_add_latency()` and
`sockd_stats_update_latency()` for validated monotonic intervals. Their
process-shared `sockd_stats_update*()` wrappers take one lock per logical event
and preserve the caller's `errno`.

### `option_t.stats_socket`

Points to the command-line argument supplied with `-S`. A null pointer means
the API listener is disabled. This is command-line state, not a `sockd.conf`
directive.

## Ownership and lifetime

- The main mother creates and maps the statistics storage before worker forks.
- All inherited processes use the same shared mapping.
- Counters survive worker recycling and configuration reload.
- All values reset when the server creates a new shared mapping on restart.
- Only the monitor whose parent is the main mother owns the API listener.
- The listener exists for that monitor's lifetime and is removed on its exit.
- Accepted API connections are owned by the monitor and closed after one
  response.

## Concurrency model

Every update and snapshot uses an exclusive `fcntl` byte-range lock at offset
zero of `sockscf.shmemfd`. Counter saturation prevents unsigned wraparound, and
the session gauge never decrements below zero.

The API side is single-threaded and serves one accepted connection at a time.
It waits for initial readability for at most one second. The listener and
accepted client descriptors are nonblocking, but request processing is still
synchronous inside `run_monitor()`.

The current `stats_wait_readable()` uses a stack `fd_set`, unlike Dante's normal
dynamically sized fd-set pattern. Deployments where the accepted descriptor can
exceed the platform `FD_SETSIZE` require this to be corrected before production
use.

## Configuration

Enable the endpoint at process startup:

```sh
./sockd/sockd -f /etc/sockd.conf -S /run/dante/stats.sock
```

Without `-S`, no API socket is created. The path must be non-empty and fit in
`sockaddr_un.sun_path`. An existing regular file is preserved and causes the
open operation to fail. An existing socket node is removed before binding.

The socket node is changed to mode `0600`; therefore a future exporter must run
under the same effective UID as the monitor unless the access model is changed.

## API and metrics contract

The only successful request is `GET /v1/stats`. It returns JSON schema version
1, revision 4, with server metadata, timestamps, compatible aggregates, and a
fixed `details` taxonomy. Counter names end in `_total`. The complete
field-level semantics and suggested future Prometheus mapping are in
`docs/features/statistics-api/metrics.md`.

## Security constraints

- Treat the endpoint as a local, trusted-collector interface.
- Filesystem mode `0600` is the only implemented authorization mechanism;
  peer credentials are not inspected.
- Place the socket in a private directory owned by the service account. The
  implementation performs pathname-based `lstat()`, `unlink()`, `bind()`, and
  `chmod()` operations and does not protect against all races in an
  attacker-writable parent directory.
- The endpoint has no rate limiting. A permitted same-UID client can repeatedly
  consume the monitor's one-second request wait.
- Aggregate output contains no client addresses, credentials, rule names, or
  secrets, but it does expose operational load and failure information.
- Do not bridge this Unix socket to an untrusted network without adding an
  authenticated, rate-limited boundary.

## Error handling

- Null or empty socket paths fail with `EINVAL`.
- Paths too long for `sun_path` fail with `ENAMETOOLONG`.
- Existing non-socket nodes fail with `EEXIST` and are not removed in the
  normal non-racy path.
- Socket creation, binding, permission, listening, and nonblocking failures
  return `-1`; partial setup is cleaned up.
- A client that sends no readable data within one second fails with
  `ETIMEDOUT` and is closed.
- Malformed request lines and unsupported HTTP versions receive 400.
- Non-GET methods receive 405 with `Allow: GET`.
- Unknown GET paths receive 404.
- Serialization truncation fails with `ENOSPC`; truncated JSON is not served.
- Serving failures are logged by the monitor with `swarn()`.
- Failure to create the configured listener is fatal to that monitor via
  `serr()` and is then subject to Dante's existing child supervision.

## Invariants

- `schema_version` is `SOCKD_STATS_SCHEMA_VERSION` and currently equals 1.
- `schema_revision` is `SOCKD_STATS_SCHEMA_REVISION` and currently equals 4.
- Counters never wrap; they saturate at `UINT64_MAX`.
- Global and per-protocol active gauges never underflow.
- Unknown enum inputs enter bounded `unknown` or `other` buckets.
- Latency bucket counts are cumulative, never exceed their histogram `count`,
  and use inclusive finite upper bounds. `count` is the implicit `+Inf`
  bucket.
- Missing, invalid, reversed, or incomplete latency timestamp pairs are
  omitted rather than recorded as zero-duration observations.
- Detailed events update their compatible aggregate in the same locked
  mutation; producers must not also emit the old failure/close event.
- A snapshot is copied while holding the same lock used by writers.
- The API is read-only and never mutates statistics based on HTTP input.
- The response advertises its exact byte length and closes the connection.
- Only one monitor process binds the API path, including with `sockd -N > 1`.
- API state resets on server restart, not on worker restart or SIGHUP reload.

## Build and test commands

```sh
./configure --disable-client
make -j2
sockd/tests/run-stats-api-tests.sh
```

The project has no general automated test suite. The feature runner compiles
`stats_api_test.c` and `statistics.c` as a standalone test binary. Detailed
coverage and live-test procedures are in
`docs/features/statistics-api/testing.md`.

## Known pitfalls

- Statistics locking is active even when `-S` is absent.
- Byte updates occur on the I/O hot path and serialize across processes on one
  lock; benchmark high-throughput deployments.
- `sessions_active` can remain elevated after abnormal I/O-worker termination.
- Request parsing uses one `recv()` and does not assemble fragmented HTTP
  request lines.
- The monitor handles one API connection at a time; a slow local peer delays
  normal monitor work.
- The stack `fd_set` in `stats_wait_readable()` is unsafe if its descriptor is
  outside the platform's fixed set size.
- Existing socket nodes are treated as stale and removed without checking
  whether another service is actively listening.
- Socket cleanup checks the path type but does not verify device/inode identity
  against the node originally created.
- Revision 4 has 297 fixed detailed numeric series. Adding client,
  destination, user,
  rule, PID, or free-form error labels in a future exporter would create a
  cardinality or privacy problem.
- `sessions_established_total` is a compatibility name for I/O admission, not
  proof that the target TCP connect completed.
- Target attempts can temporarily exceed outcomes while nonblocking connects
  are in flight; abnormal worker death can leave that difference.
- UDP receive errors count failed receive calls. Datagram counters are
  independent of the aggregate payload-byte counters.
- Worker capacity is a `childcheck()` snapshot and can be stale until its next
  cycle; retiring and waiting workers are excluded.
- Each UDP event currently takes the global statistics lock; benchmark
  packet-heavy workloads.
- Each recorded latency interval takes the same global lock. Outcome and
  latency updates are individually coherent but are not one atomic combined
  update, so a concurrent scrape can temporarily observe a one-event skew.

## Do NOT

- Do NOT claim that a Prometheus exporter or `/metrics` endpoint exists.
- Do NOT expose the Unix socket through a public TCP proxy without separate
  authentication and rate limiting.
- Do NOT place the socket in a group/world-writable non-private directory.
- Do NOT change field meaning or remove JSON fields without incrementing the
  schema version. Additive fields within version 1 require a schema revision.
- Do NOT accumulate counters again in an exporter; expose Dante's raw values so
  Prometheus can detect resets through `started_at` and normal counter logic.
- Do NOT treat `sessions_active` as exact after abnormal worker death.
- Do NOT read or write `sockd_stats_t` across processes without the shared lock
  or a deliberately designed replacement synchronization mechanism.
- Do NOT add per-client, per-destination, or per-rule labels without a bounded
  cardinality design.
- Do NOT infer target-connect success from session admission or infer UDP
  datagram dimensions from aggregate byte counters; use revision 3 directly.
- Do NOT treat a missing latency observation as zero. For Prometheus, divide
  bounds and sums by `1000000`, use the finite buckets as already cumulative,
  and synthesize `le="+Inf"` from `count`.
- Do NOT move API ownership to every monitor; only the main mother's monitor
  may bind the configured path.
- Do NOT use a fixed-size `fd_set` for descriptors that may exceed
  `FD_SETSIZE`.
