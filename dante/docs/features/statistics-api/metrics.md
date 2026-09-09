Last verified against implementation commit: 11448e34dc558a425748f56c832c366baa5051c2

# Statistics API metric dictionary

This document is the canonical semantic dictionary for `GET /v1/stats` at
schema version 1, revision 2. The API exposes raw cumulative values. A future
Prometheus exporter should translate them without accumulating them again.

## Compatibility

- `schema_version` remains `1` and all revision 1 fields are preserved.
- `schema_revision` is `2` when the `details` object is available.
- Consumers of only the original aggregate fields may ignore
  `schema_revision` and unknown JSON members.
- Consumers requiring detailed statistics should require
  `schema_version == 1` and `schema_revision >= 2`.
- All counters are saturating `uint64_t` values and reset only when the server
  creates a new shared mapping on restart.

## Original aggregate fields

| JSON path | Type | Meaning | Suggested Prometheus metric |
|---|---|---|---|
| `counters.client_connections_accepted_total` | counter | TCP clients accepted by the mother after local-address discovery succeeds | `dante_client_connections_accepted_total` |
| `counters.client_connections_dropped_total` | counter | Accepted clients dropped because no negotiation child was available; not every possible pre-negotiation drop | `dante_client_connections_dropped_total` |
| `counters.negotiation_failures_total` | counter | Terminal negotiation EOF, protocol/error outcome, unknown outcome, or timeout | `dante_negotiation_failures_total` |
| `counters.request_failures_total` | counter | `dorequest()` result other than `IO_NOERROR` | `dante_request_failures_total` |
| `counters.sessions_established_total` | counter | Legacy name: I/O objects accepted and allocated by an I/O worker; this does not guarantee a completed target TCP connect | `dante_sessions_admitted_total` |
| `counters.sessions_closed_total` | counter | Allocated I/O objects entering `io_delete()` | `dante_sessions_closed_total` |
| `counters.session_errors_total` | counter | Session close status `IO_IOERROR`, `IO_ERROR`, or `IO_TIMEOUT` | `dante_session_errors_total` |
| `counters.client_read_bytes_total` | counter, bytes | Socket payload bytes read from the internal/client side | `dante_client_read_bytes_total` |
| `counters.client_written_bytes_total` | counter, bytes | Socket payload bytes written to the internal/client side | `dante_client_written_bytes_total` |
| `counters.target_read_bytes_total` | counter, bytes | Socket payload bytes read from the external/target side | `dante_target_read_bytes_total` |
| `counters.target_written_bytes_total` | counter, bytes | Socket payload bytes written to the external/target side | `dante_target_written_bytes_total` |
| `gauges.sessions_active` | gauge | Admitted I/O objects not yet processed by `io_delete()` | `dante_sessions_active` |

The byte fields are payload accounting at Dante's socket boundary. They are
not IP/TCP/UDP wire bytes and do not prove that a remote application consumed
the data.

## Negotiation outcomes

JSON path template:

```text
details.negotiations.<result>_total
```

Suggested mapping:

```text
dante_negotiations_total{result="<result>"}
```

| Result | Meaning |
|---|---|
| `success` | Negotiation completed and the object was successfully forwarded to the mother; temporary handoff retries are not double-counted |
| `eof` | Client closed during negotiation |
| `error` | Negotiation ended with a protocol, socket, or other terminal error |
| `timeout` | Negotiation exceeded the configured negotiation timeout |
| `unknown` | An internal value outside the known enumeration was normalized into a bounded bucket |

Every non-success outcome also increments the original
`negotiation_failures_total` counter.

## Request outcomes

JSON path template:

```text
details.requests.<command>.<result>_total
```

Suggested mapping:

```text
dante_requests_total{command="<command>",result="<result>"}
```

Commands are a fixed set:

| Command | Meaning |
|---|---|
| `connect` | SOCKS CONNECT request |
| `bind` | SOCKS BIND request |
| `udp_associate` | SOCKS UDP ASSOCIATE request |
| `unknown` | Unsupported, pseudo, or otherwise unrecognized internal command |

Results are a fixed set:

| Result | Dante statuses |
|---|---|
| `success` | `IO_NOERROR` |
| `blocked` | `IO_BLOCK`, `IO_TMPBLOCK` |
| `timeout` | `IO_TIMEOUT` |
| `network_error` | `IO_IOERROR` |
| `internal_error` | `IO_ERROR` |
| `closed` | `IO_CLOSE` |
| `admin` | `IO_ADMINTERMINATION` |
| `other` | `IO_TMPERROR`, `IO_EAGAIN`, and unknown values |

The request outcome is recorded once after `dorequest()` returns. Every
non-success result also increments `request_failures_total`.

## Session lifecycle by protocol

Protocols are `tcp`, `udp`, and `unknown`. Each protocol object contains:

| JSON path suffix | Type | Meaning | Suggested Prometheus metric |
|---|---|---|---|
| `started_total` | counter | I/O objects accepted and allocated by an I/O worker | `dante_sessions_started_total{protocol}` |
| `active` | gauge | Started objects not yet closed through `io_delete()` | `dante_sessions_active{protocol}` |
| `closed_total` | counter | Objects entering `io_delete()` | `dante_sessions_closed_total{protocol}` |
| `errors_total` | counter | Closures classified as network error, internal error, or timeout | `dante_session_errors_total{protocol}` |

JSON path template:

```text
details.sessions.<protocol>.<field>
```

`started_total` deliberately avoids the word `established`: for a direct,
nonblocking CONNECT, the I/O object can be admitted before the target connect
completes. Genuine target-connect attempts/results require a separate future
family.

## Session close reasons

JSON path template:

```text
details.session_closures.<reason>_total
```

Suggested mapping:

```text
dante_session_closures_total{reason="<reason>"}
```

| Reason | Dante statuses |
|---|---|
| `normal` | `IO_NOERROR` |
| `blocked` | `IO_BLOCK`, `IO_TMPBLOCK` |
| `timeout` | `IO_TIMEOUT` |
| `network_error` | `IO_IOERROR` |
| `internal_error` | `IO_ERROR` |
| `peer_closed` | `IO_CLOSE` |
| `admin` | `IO_ADMINTERMINATION` |
| `other` | `IO_TMPERROR`, `IO_EAGAIN`, and unknown values |

Close reasons are independent of `session_errors_total`: blocked, peer, admin,
normal, and other closures are classified but do not count as legacy session
errors.

## Cardinality and privacy contract

Revision 2 contains 57 fixed detailed series: five negotiation results, 32
command/result request cells, twelve protocol/session cells, and eight close
reasons. An exporter can initialize all of them to zero without discovering
labels dynamically.

Future revisions must not use these values as labels:

- username, group, SID, principal, password, or authentication token;
- client or target address, hostname, domain, or port;
- PID, request ID, session ID, raw `errno`, or free-form error text;
- rule number or source line unless an explicitly opt-in bounded policy is
  designed and reviewed.

Unknown inputs must continue to map to the existing `unknown` or `other`
buckets rather than becoming new keys.

## Known accuracy limits

- Active gauges can remain elevated if an I/O worker terminates without the
  normal `io_delete()` path.
- `sessions_established_total` is retained for compatibility but represents
  admission to I/O, not target establishment.
- Detailed TCP/UDP byte and datagram families are not part of revision 2.
  Dante UDP accounting is held in per-target objects and must not be inferred
  from the current aggregate byte updater.
- Kernel backlog drops, firewall drops, retransmissions, and on-wire byte
  counts require operating-system telemetry outside this API.
