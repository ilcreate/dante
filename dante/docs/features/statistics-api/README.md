Last verified against implementation commit: 35d9ebe

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

## JSON schema version 1, revision 5

Revision 5 additively extends revision 4. It preserves every existing path and
adds bounded protocol/direction byte counters, session lifecycle by client
address family, UDP datagram-size histograms, and I/O outcomes by protocol and
socket side. Existing consumers can ignore unknown members. Consumers should
check `schema_version == 1` and require the minimum revision for the families
they consume.

A successful response has this shape:

```json
{
  "schema_version": 1,
  "schema_revision": 5,
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
    },
    "target_connects": {
      "attempts_total": 10,
      "success_total": 8,
      "refused_total": 1,
      "timeout_total": 0,
      "unreachable_total": 1,
      "network_error_total": 0,
      "resource_error_total": 0,
      "other_total": 0
    },
    "udp": {
      "client_to_target": {
        "received_total": 7, "forwarded_total": 6,
        "receive_errors_total": 0,
        "drops": {
          "blocked_total": 1, "malformed_total": 0,
          "dns_error_total": 0, "unexpected_source_total": 0,
          "send_error_total": 0, "internal_error_total": 0,
          "other_total": 0
        }
      },
      "target_to_client": {
        "received_total": 6, "forwarded_total": 6,
        "receive_errors_total": 0,
        "drops": {
          "blocked_total": 0, "malformed_total": 0,
          "dns_error_total": 0, "unexpected_source_total": 0,
          "send_error_total": 0, "internal_error_total": 0,
          "other_total": 0
        }
      },
      "unknown": {
        "received_total": 0, "forwarded_total": 0,
        "receive_errors_total": 0,
        "drops": {
          "blocked_total": 0, "malformed_total": 0,
          "dns_error_total": 0, "unexpected_source_total": 0,
          "send_error_total": 0, "internal_error_total": 0,
          "other_total": 0
        }
      }
    },
    "workers": {
      "negotiate": {"processes": 1, "slots_total": 64, "slots_free": 63, "slots_busy": 1, "spawn_failures_total": 0},
      "request": {"processes": 1, "slots_total": 64, "slots_free": 64, "slots_busy": 0, "spawn_failures_total": 0},
      "io": {"processes": 1, "slots_total": 64, "slots_free": 62, "slots_busy": 2, "spawn_failures_total": 0},
      "unknown": {"processes": 0, "slots_total": 0, "slots_free": 0, "slots_busy": 0, "spawn_failures_total": 0}
    },
    "auth": {
      "none": {"success_total": 12, "failure_total": 0},
      "username": {"success_total": 0, "failure_total": 0},
      "gssapi": {"success_total": 0, "failure_total": 0},
      "pam": {"success_total": 0, "failure_total": 0},
      "bsdauth": {"success_total": 0, "failure_total": 0},
      "ldap": {"success_total": 0, "failure_total": 0},
      "rfc931": {"success_total": 0, "failure_total": 0},
      "unknown": {"success_total": 0, "failure_total": 0}
    },
    "acl": {
      "client": {"pass_total": 16, "block_total": 0},
      "hostid": {"pass_total": 0, "block_total": 0},
      "socks": {"pass_total": 18, "block_total": 2},
      "unknown": {"pass_total": 0, "block_total": 0}
    },
    "dns": {
      "forward": {"success_total": 3, "not_found_total": 0, "temporary_total": 0, "system_error_total": 0, "internal_error_total": 0, "other_total": 0},
      "reverse": {"success_total": 2, "not_found_total": 0, "temporary_total": 0, "system_error_total": 0, "internal_error_total": 0, "other_total": 0},
      "unknown": {"success_total": 0, "not_found_total": 0, "temporary_total": 0, "system_error_total": 0, "internal_error_total": 0, "other_total": 0}
    },
    "latency": {
      "unit": "microseconds",
      "bucket_upper_bounds": [100, 500, 1000, 5000, 10000, 25000, 50000, 100000, 250000, 500000, 1000000, 2500000, 5000000, 10000000, 30000000, 60000000, 300000000, 3600000000],
      "negotiation": {"count": 12, "sum_microseconds": 8400, "cumulative_bucket_counts": [0, 2, 8, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12]},
      "request": {"count": 10, "sum_microseconds": 12500, "cumulative_bucket_counts": [0, 1, 5, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10]},
      "target_connect": {"count": 10, "sum_microseconds": 31000, "cumulative_bucket_counts": [0, 0, 2, 8, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10]},
      "first_io": {"count": 8, "sum_microseconds": 6400, "cumulative_bucket_counts": [0, 2, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8]},
      "session": {"count": 8, "sum_microseconds": 2600000, "cumulative_bucket_counts": [0, 0, 0, 0, 0, 1, 2, 3, 5, 6, 7, 8, 8, 8, 8, 8, 8, 8]},
      "dns_resolver": {"count": 5, "sum_microseconds": 7200, "cumulative_bucket_counts": [0, 0, 2, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5]},
      "authentication": {"count": 12, "sum_microseconds": 900, "cumulative_bucket_counts": [8, 11, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12]}
    },
    "traffic": {
      "bytes": {
        "tcp": {"client_to_target_total": 900, "target_to_client_total": 700},
        "udp": {"client_to_target_total": 300, "target_to_client_total": 200},
        "unknown": {"client_to_target_total": 0, "target_to_client_total": 0}
      },
      "sessions_by_address_family": {
        "ipv4": {"started_total": 8, "active": 1, "closed_total": 7, "errors_total": 1},
        "ipv6": {"started_total": 2, "active": 1, "closed_total": 1, "errors_total": 0},
        "unknown": {"started_total": 0, "active": 0, "closed_total": 0, "errors_total": 0}
      },
      "udp_datagram_size": {
        "unit": "bytes",
        "bucket_upper_bounds": [64, 128, 256, 512, 1024, 1280, 1500, 2048, 4096, 8192, 16384, 32768, 65507, 65535],
        "client_to_target": {"count": 3, "sum_bytes": 284, "cumulative_bucket_counts": [1, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3]},
        "target_to_client": {"count": 2, "sum_bytes": 192, "cumulative_bucket_counts": [0, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2]},
        "unknown": {"count": 0, "sum_bytes": 0, "cumulative_bucket_counts": [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0]}
      },
      "io_outcomes": {
        "tcp": {
          "client": {"read_errors_total": 0, "write_errors_total": 1, "zero_writes_total": 0, "partial_writes_total": 1, "unknown_total": 0},
          "target": {"read_errors_total": 1, "write_errors_total": 0, "zero_writes_total": 0, "partial_writes_total": 0, "unknown_total": 0},
          "unknown": {"read_errors_total": 0, "write_errors_total": 0, "zero_writes_total": 0, "partial_writes_total": 0, "unknown_total": 0}
        },
        "udp": {
          "client": {"read_errors_total": 0, "write_errors_total": 0, "zero_writes_total": 0, "partial_writes_total": 0, "unknown_total": 0},
          "target": {"read_errors_total": 0, "write_errors_total": 0, "zero_writes_total": 0, "partial_writes_total": 0, "unknown_total": 0},
          "unknown": {"read_errors_total": 0, "write_errors_total": 0, "zero_writes_total": 0, "partial_writes_total": 0, "unknown_total": 0}
        },
        "unknown": {
          "client": {"read_errors_total": 0, "write_errors_total": 0, "zero_writes_total": 0, "partial_writes_total": 0, "unknown_total": 0},
          "target": {"read_errors_total": 0, "write_errors_total": 0, "zero_writes_total": 0, "partial_writes_total": 0, "unknown_total": 0},
          "unknown": {"read_errors_total": 0, "write_errors_total": 0, "zero_writes_total": 0, "partial_writes_total": 0, "unknown_total": 0}
        }
      }
    }
  }
}
```

`snapshot_time`, `started_at`, and `uptime_seconds` are integer seconds based
on the system wall clock. Latency values are integer microseconds measured
from Dante's monotonic clock. If the wall clock is earlier than `started_at`,
`uptime_seconds` is reported as zero.

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

Revision 3 adds 100 fixed series: target-connect attempts/results, UDP
datagrams/errors/drops by direction, worker capacity and spawn failures,
authentication outcomes, ACL decisions, and DNS backend-query outcomes. The
revision 4 latency families add 140 series (seven histograms, each with 18
finite buckets, `count`, and `sum`). Revision 5 adds 111 series: six traffic
byte counters, twelve address-family lifecycle values, 48 UDP histogram
values, and 45 I/O outcomes. The API now exposes 408 fixed detailed numeric
series in total. The complete semantics and exporter mappings are in
[`metrics.md`](metrics.md).

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
3. require HTTP 200, `schema_version == 1`, and the minimum schema revision
   required by the selected families (`2` for lifecycle details, `3` for the
   operational families, `4` for latency histograms, `5` for protocol and
   traffic breakdown);
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
  including per-datagram revision 3/5 and per-lifecycle revision 4/5 updates,
  and when `-S` is not configured. UDP receive and histogram updates share one
  lock, as do successful UDP forwarding and byte updates.
- Worker gauges are periodic snapshots; target attempts can exceed outcomes
  while nonblocking connects are in flight.
- `stats_wait_readable()` currently uses a fixed stack `fd_set`; high-descriptor
  deployments need the dynamic-fd-set issue corrected before production use.
- Existing socket nodes are removed as stale without checking for an active
  listener or matching owner.

See `docs/context/statistics-api.md` for invariants and explicit maintenance
rules, and `docs/features/statistics-api/testing.md` for verification.
