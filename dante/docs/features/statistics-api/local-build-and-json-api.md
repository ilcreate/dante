# Local build and JSON API

## Build

```sh
./configure --disable-client
make -j2
```

Binary:

```sh
./sockd/sockd
```

Check build and run tests:

```sh
./sockd/sockd -v
sockd/tests/run-stats-api-tests.sh
```

## Local configuration

Create `/tmp/dante-stats.conf`:

```conf
logoutput: stderr

internal: 127.0.0.1 port = 1081
external: lo0

socksmethod: none
clientmethod: none

user.privileged: iloskin
user.unprivileged: iloskin

client pass {
    from: 127.0.0.1/32 to: 0.0.0.0/0
}

socks pass {
    from: 127.0.0.1/32 to: 0.0.0.0/0
    protocol: tcp udp
}
```

Validate:

```sh
./sockd/sockd \
  -V \
  -f /tmp/dante-stats.conf \
  -S /tmp/dante-stats.sock
```

Run in foreground:

```sh
./sockd/sockd \
  -d 0 \
  -f /tmp/dante-stats.conf \
  -p /tmp/dante-stats.pid \
  -S /tmp/dante-stats.sock
```

Stop with `Ctrl-C`.

## Query JSON API

```sh
curl --silent --show-error \
  --unix-socket /tmp/dante-stats.sock \
  http://localhost/v1/stats |
jq .
```

## Check counter updates

Open and close a TCP connection without SOCKS negotiation:

```sh
nc -vz 127.0.0.1 1081
```

The following values increase:

```text
counters.client_connections_accepted_total
counters.negotiation_failures_total
details.negotiations.eof_total
```

Test a complete SOCKS CONNECT through a local HTTP server:

```sh
python3 -m http.server 8080 --bind 127.0.0.1
```

In another terminal:

```sh
curl --noproxy "" \
  --socks5-hostname 127.0.0.1:1081 \
  http://127.0.0.1:8080/
```

## JSON response

`GET /v1/stats` returns schema version 1, revision 2:

```json
{
  "schema_version": 1,
  "schema_revision": 2,
  "server_version": "1.4.4",
  "snapshot_time": 1788982182,
  "started_at": 1788982152,
  "uptime_seconds": 30,
  "counters": {
    "client_connections_accepted_total": 0,
    "client_connections_dropped_total": 0,
    "negotiation_failures_total": 0,
    "request_failures_total": 0,
    "sessions_established_total": 0,
    "sessions_closed_total": 0,
    "session_errors_total": 0,
    "client_read_bytes_total": 0,
    "client_written_bytes_total": 0,
    "target_read_bytes_total": 0,
    "target_written_bytes_total": 0
  },
  "gauges": {
    "sessions_active": 0
  },
  "details": {
    "negotiations": {
      "success_total": 0,
      "eof_total": 0,
      "error_total": 0,
      "timeout_total": 0,
      "unknown_total": 0
    },
    "requests": {
      "connect": {
        "success_total": 0,
        "blocked_total": 0,
        "timeout_total": 0,
        "network_error_total": 0,
        "internal_error_total": 0,
        "closed_total": 0,
        "admin_total": 0,
        "other_total": 0
      },
      "bind": {
        "success_total": 0,
        "blocked_total": 0,
        "timeout_total": 0,
        "network_error_total": 0,
        "internal_error_total": 0,
        "closed_total": 0,
        "admin_total": 0,
        "other_total": 0
      },
      "udp_associate": {
        "success_total": 0,
        "blocked_total": 0,
        "timeout_total": 0,
        "network_error_total": 0,
        "internal_error_total": 0,
        "closed_total": 0,
        "admin_total": 0,
        "other_total": 0
      },
      "unknown": {
        "success_total": 0,
        "blocked_total": 0,
        "timeout_total": 0,
        "network_error_total": 0,
        "internal_error_total": 0,
        "closed_total": 0,
        "admin_total": 0,
        "other_total": 0
      }
    },
    "sessions": {
      "tcp": {
        "started_total": 0,
        "active": 0,
        "closed_total": 0,
        "errors_total": 0
      },
      "udp": {
        "started_total": 0,
        "active": 0,
        "closed_total": 0,
        "errors_total": 0
      },
      "unknown": {
        "started_total": 0,
        "active": 0,
        "closed_total": 0,
        "errors_total": 0
      }
    },
    "session_closures": {
      "normal_total": 0,
      "blocked_total": 0,
      "timeout_total": 0,
      "network_error_total": 0,
      "internal_error_total": 0,
      "peer_closed_total": 0,
      "admin_total": 0,
      "other_total": 0
    }
  }
}
```

Timestamps and statistic values change at runtime.
