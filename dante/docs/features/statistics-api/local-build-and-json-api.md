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
curl --noproxy "*" --silent --show-error \
  --unix-socket /tmp/dante-stats.sock \
  http://localhost/v1/stats |
jq .
```

Show only latency histograms:

```sh
curl --noproxy "*" --silent --show-error \
  --unix-socket /tmp/dante-stats.sock \
  http://localhost/v1/stats |
jq '.details.latency'
```

Show only revision 5 protocol and traffic details:

```sh
curl --noproxy "*" --silent --show-error \
  --unix-socket /tmp/dante-stats.sock \
  http://localhost/v1/stats |
jq '.details.traffic'
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

`GET /v1/stats` returns schema version 1, revision 5:

```json
{
  "schema_version": 1,
  "schema_revision": 5,
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
    },
    "target_connects": {
      "attempts_total": 0,
      "success_total": 0,
      "refused_total": 0,
      "timeout_total": 0,
      "unreachable_total": 0,
      "network_error_total": 0,
      "resource_error_total": 0,
      "other_total": 0
    },
    "udp": {
      "client_to_target": {
        "received_total": 0,
        "forwarded_total": 0,
        "receive_errors_total": 0,
        "drops": {
          "blocked_total": 0,
          "malformed_total": 0,
          "dns_error_total": 0,
          "unexpected_source_total": 0,
          "send_error_total": 0,
          "internal_error_total": 0,
          "other_total": 0
        }
      },
      "target_to_client": {
        "received_total": 0,
        "forwarded_total": 0,
        "receive_errors_total": 0,
        "drops": {
          "blocked_total": 0,
          "malformed_total": 0,
          "dns_error_total": 0,
          "unexpected_source_total": 0,
          "send_error_total": 0,
          "internal_error_total": 0,
          "other_total": 0
        }
      },
      "unknown": {
        "received_total": 0,
        "forwarded_total": 0,
        "receive_errors_total": 0,
        "drops": {
          "blocked_total": 0,
          "malformed_total": 0,
          "dns_error_total": 0,
          "unexpected_source_total": 0,
          "send_error_total": 0,
          "internal_error_total": 0,
          "other_total": 0
        }
      }
    },
    "workers": {
      "negotiate": {
        "processes": 0, "slots_total": 0, "slots_free": 0,
        "slots_busy": 0, "spawn_failures_total": 0
      },
      "request": {
        "processes": 0, "slots_total": 0, "slots_free": 0,
        "slots_busy": 0, "spawn_failures_total": 0
      },
      "io": {
        "processes": 0, "slots_total": 0, "slots_free": 0,
        "slots_busy": 0, "spawn_failures_total": 0
      },
      "unknown": {
        "processes": 0, "slots_total": 0, "slots_free": 0,
        "slots_busy": 0, "spawn_failures_total": 0
      }
    },
    "auth": {
      "none": {"success_total": 0, "failure_total": 0},
      "username": {"success_total": 0, "failure_total": 0},
      "gssapi": {"success_total": 0, "failure_total": 0},
      "pam": {"success_total": 0, "failure_total": 0},
      "bsdauth": {"success_total": 0, "failure_total": 0},
      "ldap": {"success_total": 0, "failure_total": 0},
      "rfc931": {"success_total": 0, "failure_total": 0},
      "unknown": {"success_total": 0, "failure_total": 0}
    },
    "acl": {
      "client": {"pass_total": 0, "block_total": 0},
      "hostid": {"pass_total": 0, "block_total": 0},
      "socks": {"pass_total": 0, "block_total": 0},
      "unknown": {"pass_total": 0, "block_total": 0}
    },
    "dns": {
      "forward": {
        "success_total": 0, "not_found_total": 0,
        "temporary_total": 0, "system_error_total": 0,
        "internal_error_total": 0, "other_total": 0
      },
      "reverse": {
        "success_total": 0, "not_found_total": 0,
        "temporary_total": 0, "system_error_total": 0,
        "internal_error_total": 0, "other_total": 0
      },
      "unknown": {
        "success_total": 0, "not_found_total": 0,
        "temporary_total": 0, "system_error_total": 0,
        "internal_error_total": 0, "other_total": 0
      }
    },
    "latency": {
      "unit": "microseconds",
      "bucket_upper_bounds": [100, 500, 1000, 5000, 10000, 25000, 50000, 100000, 250000, 500000, 1000000, 2500000, 5000000, 10000000, 30000000, 60000000, 300000000, 3600000000],
      "negotiation": {"count": 0, "sum_microseconds": 0, "cumulative_bucket_counts": [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0]},
      "request": {"count": 0, "sum_microseconds": 0, "cumulative_bucket_counts": [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0]},
      "target_connect": {"count": 0, "sum_microseconds": 0, "cumulative_bucket_counts": [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0]},
      "first_io": {"count": 0, "sum_microseconds": 0, "cumulative_bucket_counts": [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0]},
      "session": {"count": 0, "sum_microseconds": 0, "cumulative_bucket_counts": [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0]},
      "dns_resolver": {"count": 0, "sum_microseconds": 0, "cumulative_bucket_counts": [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0]},
      "authentication": {"count": 0, "sum_microseconds": 0, "cumulative_bucket_counts": [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0]}
    },
    "traffic": {
      "bytes": {
        "tcp": {"client_to_target_total": 0, "target_to_client_total": 0},
        "udp": {"client_to_target_total": 0, "target_to_client_total": 0},
        "unknown": {"client_to_target_total": 0, "target_to_client_total": 0}
      },
      "sessions_by_address_family": {
        "ipv4": {"started_total": 0, "active": 0, "closed_total": 0, "errors_total": 0},
        "ipv6": {"started_total": 0, "active": 0, "closed_total": 0, "errors_total": 0},
        "unknown": {"started_total": 0, "active": 0, "closed_total": 0, "errors_total": 0}
      },
      "udp_datagram_size": {
        "unit": "bytes",
        "bucket_upper_bounds": [64, 128, 256, 512, 1024, 1280, 1500, 2048, 4096, 8192, 16384, 32768, 65507, 65535],
        "client_to_target": {"count": 0, "sum_bytes": 0, "cumulative_bucket_counts": [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0]},
        "target_to_client": {"count": 0, "sum_bytes": 0, "cumulative_bucket_counts": [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0]},
        "unknown": {"count": 0, "sum_bytes": 0, "cumulative_bucket_counts": [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0]}
      },
      "io_outcomes": {
        "tcp": {
          "client": {"read_errors_total": 0, "write_errors_total": 0, "zero_writes_total": 0, "partial_writes_total": 0, "unknown_total": 0},
          "target": {"read_errors_total": 0, "write_errors_total": 0, "zero_writes_total": 0, "partial_writes_total": 0, "unknown_total": 0},
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

Timestamps and statistic values change at runtime. Both histogram bucket arrays
are already cumulative and each `count` is the implicit `+Inf` bucket. Latency
bounds/sums are microseconds; UDP size bounds/sums are bytes. Detailed field
semantics are in [`metrics.md`](metrics.md).
