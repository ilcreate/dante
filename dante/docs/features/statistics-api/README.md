Last verified against implementation commit: 11448e34dc558a425748f56c832c366baa5051c2

# Dante statistics API

## Overview

The feature provides a read-only, versioned JSON snapshot of process-shared
`sockd` statistics over a Unix domain socket. It is intended to be scraped by a
trusted local process and is the current source contract for a future
Prometheus exporter.

The future exporter is not implemented in this repository. There is no
`/metrics` endpoint and Dante does not emit Prometheus text format.

## Enabling the endpoint

The API is disabled unless `sockd` receives `-S <unix-socket>`:

```sh
./sockd/sockd -f /etc/sockd.conf -S /run/dante/stats.sock
```

Query it with an HTTP client that supports Unix sockets:

```sh
curl --unix-socket /run/dante/stats.sock http://localhost/v1/stats
```

The HTTP hostname is ignored because transport is the Unix socket. The socket
is created with mode `0600`, so the collector normally needs the same effective
UID as the monitor process.

## HTTP contract

| Request | Result |
|---|---|
| `GET /v1/stats HTTP/1.0` | 200 JSON snapshot |
| `GET /v1/stats HTTP/1.1` | 200 JSON snapshot |
| Any other method | 405 JSON error and `Allow: GET` |
| `GET` for another path | 404 JSON error |
| Malformed line, missing newline, unsupported HTTP version | 400 JSON error |

Every response uses HTTP/1.1 and includes:

- `Content-Type: application/json`
- an exact `Content-Length`
- `Cache-Control: no-store`
- `Connection: close`

One request is served per connection. The implementation reads at most 4096
bytes once and requires the request line to fit within 511 bytes. It waits up
to one second for the first readable data.

## JSON schema version 1, revision 2

Revision 2 is an additive extension of version 1. It preserves every original
counter and gauge, adds `schema_revision`, and adds a fixed-cardinality
`details` object. Existing consumers can keep reading the original paths and
ignore unknown members. Consumers that require the new families should check
both `schema_version == 1` and `schema_revision >= 2`.

A successful response has this shape:

```json
{
  "schema_version": 1,
  "schema_revision": 2,
  "server_version": "1.4.4",
  "snapshot_time": 1788897413,
  "started_at": 1788897390,
  "uptime_seconds": 23,
  "counters": {
    "client_connections_accepted_total": 16,
    "client_connections_dropped_total": 0,
    "negotiation_failures_total": 4,
    "request_failures_total": 2,
    "sessions_established_total": 10,
    "sessions_closed_total": 8,
    "session_errors_total": 1,
    "client_read_bytes_total": 1200,
    "client_written_bytes_total": 900,
    "target_read_bytes_total": 900,
    "target_written_bytes_total": 1200
  },
  "gauges": {
    "sessions_active": 2
  },
  "details": {
    "negotiations": {
      "success_total": 12,
      "eof_total": 1,
      "error_total": 2,
      "timeout_total": 1,
      "unknown_total": 0
    },
    "requests": {
      "connect": {
        "success_total": 9, "blocked_total": 1, "timeout_total": 0,
        "network_error_total": 1, "internal_error_total": 0,
        "closed_total": 0, "admin_total": 0, "other_total": 0
      },
      "bind": {
        "success_total": 0, "blocked_total": 0, "timeout_total": 0,
        "network_error_total": 0, "internal_error_total": 0,
        "closed_total": 0, "admin_total": 0, "other_total": 0
      },
      "udp_associate": {
        "success_total": 1, "blocked_total": 0, "timeout_total": 0,
        "network_error_total": 0, "internal_error_total": 0,
        "closed_total": 0, "admin_total": 0, "other_total": 0
      },
      "unknown": {
        "success_total": 0, "blocked_total": 0, "timeout_total": 0,
        "network_error_total": 0, "internal_error_total": 0,
        "closed_total": 0, "admin_total": 0, "other_total": 0
      }
    },
    "sessions": {
      "tcp": {
        "started_total": 9, "active": 2,
        "closed_total": 7, "errors_total": 1
      },
      "udp": {
        "started_total": 1, "active": 0,
        "closed_total": 1, "errors_total": 0
      },
      "unknown": {
        "started_total": 0, "active": 0,
        "closed_total": 0, "errors_total": 0
      }
    },
    "session_closures": {
      "normal_total": 4,
      "blocked_total": 0,
      "timeout_total": 0,
      "network_error_total": 1,
      "internal_error_total": 0,
      "peer_closed_total": 3,
      "admin_total": 0,
      "other_total": 0
    }
  }
}
```

All timestamps and durations are integer seconds. `snapshot_time` and
`started_at` use the system wall clock. If the clock is earlier than
`started_at`, `uptime_seconds` is reported as zero.

### Counter semantics

All counters are unsigned 64-bit values and saturate at `UINT64_MAX` rather
than wrapping.

| JSON field | Actual increment point |
|---|---|
| `client_connections_accepted_total` | Mother accepts a TCP client |
| `client_connections_dropped_total` | Mother has no negotiation child available; this is not a count of every possible drop |
| `negotiation_failures_total` | Negotiation ends with EOF, error, timeout, or an unknown terminal outcome |
| `request_failures_total` | Request processing returns a status other than `IO_NOERROR` |
| `sessions_established_total` | Legacy name: an I/O worker receives and allocates a new I/O object; target connection completion is not guaranteed |
| `sessions_closed_total` | An allocated I/O object enters `io_delete()` |
| `session_errors_total` | `io_delete()` receives `IO_IOERROR`, `IO_ERROR`, or `IO_TIMEOUT` |
| `client_read_bytes_total` | Bytes read from the internal/client side during an I/O update |
| `client_written_bytes_total` | Bytes written to the internal/client side during an I/O update |
| `target_read_bytes_total` | Bytes read from the external/target side during an I/O update |
| `target_written_bytes_total` | Bytes written to the external/target side during an I/O update |

### Gauge semantics

`sessions_active` is incremented with `sessions_established_total` and
decremented when an allocated I/O object enters `io_delete()`. It is clamped at
zero. It may remain elevated if an I/O worker terminates without running the
normal deletion path.

### Detailed families

Revision 2 exposes 57 fixed detailed numeric series:

- five negotiation outcomes: `success`, `eof`, `error`, `timeout`, `unknown`;
- 32 request cells: four commands (`connect`, `bind`, `udp_associate`,
  `unknown`) by eight results (`success`, `blocked`, `timeout`,
  `network_error`, `internal_error`, `closed`, `admin`, `other`);
- twelve session lifecycle values: four fields for each of `tcp`, `udp`, and
  `unknown`;
- eight normalized session close reasons.

Unknown internal values are normalized into bounded `unknown` or `other`
buckets. The full field semantics and exporter mapping are defined in
[`metrics.md`](metrics.md).

## Reset and consistency behavior

- Writers and readers use the same process-shared lock, so one response is a
  coherent structure copy.
- Counters survive worker recycling and configuration reload.
- Counters reset when the server creates a new shared mapping on restart.
- `started_at` identifies the reset epoch from the API consumer's perspective.
- With `sockd -N > 1`, only the main mother's monitor serves the API, while all
  mother and worker processes update the same snapshot.

## Guidance for a future Prometheus exporter

This section is an integration recommendation, not implemented behavior.

The exporter should:

1. connect to the configured Unix socket for every scrape;
2. issue `GET /v1/stats HTTP/1.1` and read until the advertised content length
   or connection close;
3. require HTTP 200, `schema_version == 1`, and `schema_revision >= 2` when
   detailed families are needed;
4. expose Dante counter values directly instead of accumulating them again;
5. let Prometheus handle counter resets, using `started_at` as supporting reset
   context;
6. use bounded connection/read deadlines and avoid aggressive retry loops;
7. reject unknown schema versions rather than silently guessing semantics;
8. keep label cardinality fixed and small.

A reasonable mapping for the original aggregate paths is:

| JSON source | Suggested Prometheus metric |
|---|---|
| `counters.client_connections_accepted_total` | `dante_client_connections_accepted_total` |
| `counters.client_connections_dropped_total` | `dante_client_connections_dropped_total` |
| `counters.negotiation_failures_total` | `dante_negotiation_failures_total` |
| `counters.request_failures_total` | `dante_request_failures_total` |
| `counters.sessions_established_total` | `dante_sessions_admitted_total` |
| `counters.sessions_closed_total` | `dante_sessions_closed_total` |
| `counters.session_errors_total` | `dante_session_errors_total` |
| `counters.client_read_bytes_total` | `dante_client_read_bytes_total` |
| `counters.client_written_bytes_total` | `dante_client_written_bytes_total` |
| `counters.target_read_bytes_total` | `dante_target_read_bytes_total` |
| `counters.target_written_bytes_total` | `dante_target_written_bytes_total` |
| `gauges.sessions_active` | `dante_sessions_active` |
| `started_at` | `dante_start_time_seconds` |
| `server_version` | `dante_build_info{version="..."} 1` |

The exporter should not use client, destination, or rule names as labels unless
a separately reviewed bounded-cardinality contract is introduced.

The exporter should flatten detailed objects into fixed labels, for example
`details.requests.connect.blocked_total` becomes
`dante_requests_total{command="connect",result="blocked"}`. It must export
the values returned by Dante directly, not maintain a second accumulator.

## Current operational and security limits

- Access control is Unix socket mode `0600`; there is no peer-credential check.
- Use a private parent directory, not an attacker-writable location.
- The monitor serves one connection synchronously, so a slow same-UID client
  can delay monitor work.
- Statistics updates take a global process-shared lock on the I/O hot path,
  including when `-S` is not configured.
- `stats_wait_readable()` currently uses a fixed stack `fd_set`; high-descriptor
  deployments need the dynamic-fd-set issue corrected before production use.
- Existing socket nodes are removed as stale without checking for an active
  listener or matching owner.

See `docs/context/statistics-api.md` for invariants and explicit maintenance
rules, and `docs/features/statistics-api/testing.md` for verification.
