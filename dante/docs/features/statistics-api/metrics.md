Last verified against implementation commit: 47fe4ae

# Statistics API metric dictionary

This document is the canonical semantic dictionary for `GET /v1/stats` at
schema version 1, revision 4. The API exposes raw cumulative values. A future
Prometheus exporter should translate them without accumulating them again.

## Compatibility

- `schema_version` remains `1` and all revision 1 fields are preserved.
- `schema_revision >= 2` provides the original detailed lifecycle families.
- `schema_revision >= 3` provides target-connect, UDP, worker, authentication,
  ACL, and DNS families.
- `schema_revision >= 4` provides the seven fixed latency histograms.
- Consumers of only the original aggregate fields may ignore
  `schema_revision` and unknown JSON members.
- Consumers requiring detailed statistics should require
  `schema_version == 1` and the minimum revision needed by each family.
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
completes. Use the revision 3 target-connect family for actual completion.

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

## Target-connect outcomes

JSON paths:

```text
details.target_connects.attempts_total
details.target_connects.<result>_total
```

Suggested mappings:

```text
dante_target_connect_attempts_total
dante_target_connect_results_total{result="<result>"}
```

| Result | Normalized outcome |
|---|---|
| `success` | Direct or upstream-proxy target connection completed |
| `refused` | `ECONNREFUSED` |
| `timeout` | `ETIMEDOUT`, including pending connects deleted on timeout |
| `unreachable` | `EHOSTUNREACH` or `ENETUNREACH` |
| `network_error` | `ECONNRESET`, `ECONNABORTED`, or `ENETDOWN` |
| `resource_error` | `EMFILE`, `ENFILE`, `ENOBUFS`, or `ENOMEM` |
| `other` | Any remaining error, including an invalid zero-error failure normalized to `EIO` |

Direct immediate results are recorded in the request worker. `EINPROGRESS`
results are recorded when the I/O worker observes completion or deletes the
pending connect. Non-direct upstream proxy connections are recorded as one
logical target-connect result after `serverchain()`. Attempts can temporarily
exceed results while connects are pending and can remain unmatched after an
abnormal worker death.

## UDP datagrams and drops

JSON path template:

```text
details.udp.<direction>.<field>
details.udp.<direction>.drops.<reason>_total
```

Directions are `client_to_target`, `target_to_client`, and `unknown`.
Suggested mappings are:

```text
dante_udp_datagrams_received_total{direction="<direction>"}
dante_udp_datagrams_forwarded_total{direction="<direction>"}
dante_udp_receive_errors_total{direction="<direction>"}
dante_udp_drops_total{direction="<direction>",reason="<reason>"}
```

`received_total` increments after a successful datagram receive;
`forwarded_total` increments only after a complete successful forwarding send.
`receive_errors_total` counts failed receive calls, including temporary and
delayed socket errors, and therefore is not a datagram count.

Drop reasons are fixed: `blocked`, `malformed`, `dns_error`,
`unexpected_source`, `send_error`, `internal_error`, and `other`. A received
datagram that reaches a known terminal rejection/error path increments one
drop bucket. These values do not include kernel/firewall drops before Dante,
and they are independent of byte counters.

## Worker capacity

JSON path template:

```text
details.workers.<type>.<field>
```

Types are `negotiate`, `request`, `io`, and `unknown`. Suggested mappings:

```text
dante_worker_processes{type="<type>"}
dante_worker_slots{type="<type>",state="total|free|busy"}
dante_worker_spawn_failures_total{type="<type>"}
```

`processes`, `slots_total`, `slots_free`, and `slots_busy` are gauges published
after `childcheck()` scans workers. Waiting-for-exit and retiring workers are
excluded; `slots_free` is clamped to total and busy is derived as total minus
free. `spawn_failures_total` counts both insufficient descriptor reservations
and failed `addchild()` attempts. Gauges can be stale between scans.

## Authentication checks

JSON path template:

```text
details.auth.<method>.success_total
details.auth.<method>.failure_total
```

Suggested mapping:

```text
dante_auth_checks_total{method="<method>",result="success|failure"}
```

Methods are `none`, `username`, `gssapi`, `pam`, `bsdauth`, `ldap`, `rfc931`,
and `unknown`. PAM variants are intentionally collapsed into `pam`.
Counters represent authentication checks actually executed by
`accesscheck()`; cached early-return decisions are not counted again. No user,
principal, credential, address, or rule identifier is exposed.

## ACL decisions

JSON path template:

```text
details.acl.<phase>.pass_total
details.acl.<phase>.block_total
```

Suggested mapping:

```text
dante_acl_decisions_total{phase="<phase>",decision="pass|block"}
```

Phases are `client` (`ACCEPT`/`BOUNCETO`), `hostid`, `socks`
(`CONNECT`/`BIND`/`UDP ASSOCIATE` and reply checks), and `unknown`. One value
is emitted for every terminal `rulespermit()` decision, so UDP per-packet rule
checks are included. This measures decisions, not unique clients or sessions.

## DNS queries

JSON path template:

```text
details.dns.<operation>.<result>_total
```

Suggested mapping:

```text
dante_dns_queries_total{operation="<operation>",result="<result>"}
```

Operations are `forward`, `reverse`, and `unknown`. Results are `success`,
`not_found`, `temporary`, `system_error`, `internal_error`, and `other`.
Counters increment only when Dante calls the resolver backend after its retry
logic; cache hits are excluded. The JSON never exposes queried names, returned
addresses, or raw `EAI_*` values.

## Latency histograms

Revision 4 exposes seven fixed histograms under `details.latency`. All values
are process-lifetime cumulative values measured with Dante's monotonic clock.
Durations and finite bucket bounds are integer microseconds.

```text
details.latency.unit
details.latency.bucket_upper_bounds
details.latency.<phase>.count
details.latency.<phase>.sum_microseconds
details.latency.<phase>.cumulative_bucket_counts
```

The shared finite upper bounds are:

```text
100, 500, 1000, 5000, 10000, 25000, 50000, 100000, 250000,
500000, 1000000, 2500000, 5000000, 10000000, 30000000,
60000000, 300000000, 3600000000
```

Each `cumulative_bucket_counts` array has exactly 18 entries paired by index
with `bucket_upper_bounds`. A duration equal to a bound is included in that
bucket. `count` is the implicit Prometheus `+Inf` bucket, so a duration above
the largest finite bound increments `count` and `sum_microseconds` but no
finite bucket. Counters and sums saturate at `UINT64_MAX`.

| Phase | Measured interval | Suggested Prometheus histogram |
|---|---|---|
| `negotiation` | negotiation start to terminal negotiation completion | `dante_negotiation_duration_seconds` |
| `request` | negotiation end to completion of `dorequest()` | `dante_request_duration_seconds` |
| `target_connect` | immediately before direct connect or upstream `serverchain()` to its synchronous or deferred terminal outcome | `dante_target_connect_duration_seconds` |
| `first_io` | session establishment to first observed client/target I/O | `dante_time_to_first_io_seconds` |
| `session` | client acceptance to entry into `io_delete()` | `dante_session_duration_seconds` |
| `dns_resolver` | resolver backend lookup, including the existing resource-retry path; cache hits excluded | `dante_dns_resolver_duration_seconds` |
| `authentication` | authentication check executed by `accesscheck()`; cached early returns excluded | `dante_authentication_duration_seconds` |

An exporter must divide bounds and sums by `1000000` to produce seconds,
export each finite cumulative value directly as a histogram bucket, emit an
additional `le="+Inf"` bucket equal to `count`, and expose `count` and the
converted sum. It must not re-cumulate the bucket array.

An operation is omitted when either timestamp is missing or invalid, the end
precedes the start, or the process dies before reaching the observation hook.
Such cases are not recorded as zero latency. Equal valid timestamps record a
zero-duration observation.

## Cardinality and privacy contract

Revision 4 contains 297 fixed detailed numeric series: the 157 revision 3
series plus 140 latency values from seven histograms, each containing 18
finite buckets, one count, and one sum. The shared bucket-bound array and unit
string are metadata, not metric series. An exporter can initialize all numeric
series to zero without discovering labels dynamically.

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
- UDP datagram accounting is available in revision 3, but protocol-specific
  byte families are not. Do not derive datagrams from aggregate byte values.
- Target attempts and results can differ while connects are pending or after
  abnormal worker death.
- Worker capacity is eventually updated by the child-management scan, not an
  instantaneous scheduler value.
- A histogram update and its related outcome/lifecycle counter can use
  separate lock acquisitions. A concurrent scrape can therefore temporarily
  observe one without the other even though each individual structure copy is
  coherent.
- Histograms omit incomplete or invalid-clock intervals. Saturated counts or
  sums also make derived averages and quantiles lower-confidence.
- Kernel backlog drops, firewall drops, retransmissions, and on-wire byte
  counts require operating-system telemetry outside this API.
