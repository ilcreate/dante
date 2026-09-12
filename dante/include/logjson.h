/* Allocation-free JSON event serialization, shared with the server logger. */
#ifndef DANTE_LOGJSON_H
#define DANTE_LOGJSON_H

#include <stddef.h>
#include <stdint.h>

#define SOCKS_LOG_JSON_MAX (65536)
#define SOCKS_LOG_JSON_MIN (512)

typedef enum {
   SOCKLOG_MESSAGE,
   SOCKLOG_ACCEPT,
   SOCKLOG_HOSTID,
   SOCKLOG_CONNECT,
   SOCKLOG_BLOCK,
   SOCKLOG_TEMPORARY_BLOCK,
   SOCKLOG_DISCONNECT,
   SOCKLOG_ERROR,
   SOCKLOG_TEMPORARY_ERROR,
   SOCKLOG_IO,
   SOCKLOG_SESSION_SNAPSHOT
} socklog_event_type_t;

#define SOCKLOG_COUNTER_DURATION_US             (1U << 0)
#define SOCKLOG_COUNTER_CLIENT_BYTES_READ        (1U << 1)
#define SOCKLOG_COUNTER_CLIENT_BYTES_WRITTEN     (1U << 2)
#define SOCKLOG_COUNTER_TARGET_BYTES_READ        (1U << 3)
#define SOCKLOG_COUNTER_TARGET_BYTES_WRITTEN     (1U << 4)
#define SOCKLOG_COUNTER_CLIENT_PACKETS_READ      (1U << 5)
#define SOCKLOG_COUNTER_CLIENT_PACKETS_WRITTEN   (1U << 6)
#define SOCKLOG_COUNTER_TARGET_PACKETS_READ      (1U << 7)
#define SOCKLOG_COUNTER_TARGET_PACKETS_WRITTEN   (1U << 8)
#define SOCKLOG_COUNTER_ALL                     ((1U << 9) - 1)

typedef struct socklog_counters_t {
   unsigned int present;
   uint64_t duration_us;
   uint64_t client_bytes_read, client_bytes_written;
   uint64_t target_bytes_read, target_bytes_written;
   uint64_t client_packets_read, client_packets_written;
   uint64_t target_packets_read, target_packets_written;
} socklog_counters_t;

typedef struct socklog_endpoint_t {
   /* NULL strings are unknown; port is known whenever the endpoint exists. */
   const char *type, *address;
   unsigned int port, scope_id;
   int scope_id_isset;
} socklog_endpoint_t;

typedef struct socklog_address_t {
   const socklog_endpoint_t *local, *peer;
   const char *auth_method, *auth_user;
   const char *const *hostids;
   size_t hostid_count;
} socklog_address_t;

typedef struct socklog_connection_t {
   /* Non-NULL connection implies a known rule number, including zero. */
   uint64_t rule_number;
   const char *rule_type, *verdict, *protocol, *command;
   const socklog_address_t *source, *destination;
   const socklog_address_t *source_proxy, *destination_proxy;
   int error_isset, error_code;
   const char *detail;
   size_t detail_len;
   int io_bytes_isset;
   uint64_t io_bytes;
   int payload_isset;
   const char *payload;
   size_t payload_len;
   const char *tcp_info;
} socklog_connection_t;

typedef struct socklog_event_t {
   socklog_event_type_t type;
   const char *message;
   size_t message_len;
   int truncated;
   const socklog_counters_t *counters;
   const socklog_connection_t *connection;
} socklog_event_t;

typedef struct socklog_context_t {
   int64_t seconds;
   unsigned int microseconds; /* Fractional part, 0 through 999999. */
   int64_t pid;
   const char *level, *process, *program;
} socklog_context_t;

/*
 * capacity includes the trailing NUL and is capped at SOCKS_LOG_JSON_MAX.
 * Returns bytes excluding NUL and including exactly one final newline, or
 * zero without modifying buf if capacity < SOCKS_LOG_JSON_MIN.  context and
 * event must be non-NULL. NULL common strings become empty strings; unknown
 * optional strings are omitted. Metadata strings are NUL-terminated;
 * message, detail and payload are length-delimited. payload_isset preserves
 * a known empty payload, including a NULL pointer with zero length.
 *
 * Common keys always survive truncation and precede optional metadata.
 * Under pressure, optional properties/objects are omitted whole and common
 * strings are shortened at encoded character boundaries; either sets
 * truncated. The serializer allocates no memory and preserves errno.
 */
size_t socks_logjson(char *buf, size_t capacity,
                    const socklog_context_t *context,
                    const socklog_event_t *event);

void slogevent(int priority, const socklog_event_t *event);

#endif /* DANTE_LOGJSON_H */
