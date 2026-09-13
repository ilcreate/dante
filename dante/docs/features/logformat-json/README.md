# JSON server logs

Dante supports `logformat: raw | json` in the global server configuration.
The default is `raw`; upgrading the binary does not change existing logs.
To enable JSON Lines, put this directive before other active settings:

```conf
logformat: json
logoutput: /var/log/sockd.jsonl
errorlog: /var/log/sockd-error.jsonl
```

Keep the existing interface, authentication and access rules. Select connection
records inside the rules, for example `log: connect disconnect error`.
`logformat` changes their representation; it does not enable additional events.
Only one directive is allowed; invalid values, duplicates and placement inside
a rule are configuration errors. Client configuration does not accept it.

`logoutput` receives all selected records. `errorlog` receives warning and more
severe records. An `event: "error"` connection record normally has level `info`
and therefore goes to `logoutput`. To additionally log selected network failures
at warning severity, use the existing setting, for example:

```conf
external.log.warning.error: ECONNREFUSED
```

These are diagnostic `message` records, in addition to the structured connection
events. The destinations and severity rules are identical in raw and JSON modes.

## Reading records

Files, stdout and stderr contain one JSON object and one newline per record;
embedded newlines are escaped. There is no Dante text prefix before the object.
Syslog receives the same object without the final newline; the system service
may add its own header. Its on-disk output need not be JSON Lines.

Every record includes:

| Field | Type and meaning |
|---|---|
| `schema_version` | Integer, currently `1` |
| `timestamp` | String with Unix seconds and exactly six fractional digits |
| `level` | Severity: `emergency`, `alert`, `critical`, `error`, `warning`, `notice`, `info`, `debug` |
| `pid`, `process`, `program` | Integer PID, process role and program name |
| `event` | Event type; `message` for general diagnostics |
| `message` | Human-readable text without the standard log prefix |
| `truncated` | Boolean; the logger shortened the record or omitted fields |

Connection events are `accept`, `hostid`, `connect`, `block`, `temporary_block`,
`disconnect`, `error`, `temporary_error` and `io`. When known, they include
`rule.number`, `rule.type`, `verdict`, `protocol`, `command`, and `source`,
`destination`, `source_proxy`, `destination_proxy` address objects. Address
objects distinguish `local` and `peer`; each endpoint has `type`, `address`,
and numeric `port`. IPv6 also has `scope_id`. Authentication metadata contains
a method and available user identifier; passwords are never serialized.
A domain name may become an IP address after resolution.

A block means rejection by a rule. An error can retain `verdict: "pass"` because
an allowed connection failed. `error.code` is emitted only when the original
operation supplies a relevant errno. Unknown values are omitted; known zeros
are retained. Consumers should tolerate additional fields.

`io_bytes` is the size of one I/O operation. `payload` requires the existing
`log: data` rule flag. `tcp_info` is escaped platform-specific text and requires
`log: tcpinfo`. Neither field is enabled by selecting JSON alone.

`session_close` adds `reason`, `side`, optional `timeout`, `duration_us`, and
known `client_bytes_read`, `client_bytes_written`, `target_bytes_read`,
`target_bytes_written`. UDP also includes the corresponding `*_packets_*`
counters. `session_snapshot` has cumulative counters and `idle_us`; it does
not close or reset the session. Close reasons are `closed`, `blocked`,
`io_error`, `error`, `timeout`, `administrative`.

Check `scope` before aggregating: `session`, `control`, `bind_listener` and
`udp_target` are distinct existing summaries and can overlap. A UDP target
record accumulates an IPv4 or IPv6 socket bucket, not each remote destination;
its address is the last known peer. Do not add snapshots to final totals.

The [complete schema and omission rules](../../plans/logformat-json.md#4-контракт-json)
include BIND direction, UDP bucket semantics and truncation details.

## Startup, reload and size limits

Startup messages before reading `logformat: json` remain raw. Putting it first
makes subsequent configuration diagnostics JSON. Help and version output remain
ordinary CLI text.

SIGHUP retains the old format until the new directive is read. Removing the
directive restores raw after a successful parse. Existing workers update
asynchronously, so raw and JSON may briefly coexist during a format switch.
Existing log files also retain earlier records. Configuration reload errors
follow Dante's existing behavior; there is no new transaction rollback.

Normal formatting/output buffers are capped at 64 KiB including NUL; allocation
failure uses 2 KiB stack buffers. Signal messages and debug-buffer dumps use
16 KiB buffers. A participating pipe/FIFO imposes its PIPE_BUF limit on all
copies of the record; signal context uses the conservative POSIX minimum.
Under these limits the serializer preserves a complete JSON object and marks
truncation. Optional fields may disappear completely. Debug dumps retain the
initial portion, so recent entries can be lost when the dump is too large.
Ordinary stack frames are JSON; in signal context a JSON message reports that
backtrace is unavailable. Existing critical-only signal syslog behavior remains.

Successful pipe/FIFO writes within PIPE_BUF are atomic. EINTR and short writes
are retried with bounded consecutive EINTR handling. A permanent I/O failure
can still leave a partial record; network filesystems and external syslog
services can impose their own guarantees and size limits. `truncated` reports
logger truncation, not damage introduced by an external service.

## Validation

Use a standard parser, rather than checking that lines start with `{`:

```sh
python3 - /tmp/dante-accept.jsonl /tmp/dante-error.jsonl <<'PY'
import json, sys
for name in sys.argv[1:]:
    count = 0
    with open(name) as source:
        for count, line in enumerate(source, 1):
            record = json.loads(line)
            assert isinstance(record, dict) and record['schema_version'] == 1
            assert isinstance(record['truncated'], bool)
    print(name, count, 'valid JSON records')
PY
```

Use fresh files after activation for this strict whole-file check; a file that
previously contained raw records needs an explicit format boundary.
Build and automated test instructions are in
[the logging test guide](../../../sockd/tests/README.logformat.md).
The [acceptance report](acceptance.md) records the platforms, code audit,
measurements and local curl demonstration.
