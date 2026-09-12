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

typedef struct socklog_event_t {
   socklog_event_type_t type;
   const char *message;
   size_t message_len;
   int truncated;
   const socklog_counters_t *counters;
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
 * event must be non-NULL.  NULL string values are represented as empty strings.
 * Metadata strings are NUL-terminated; message is length-delimited.
 *
 * Common keys always survive truncation.  Under pressure, optional counters
 * are omitted as whole properties and strings are shortened at encoded
 * character boundaries; either sets truncated.  errno is preserved.
 */
size_t socks_logjson(char *buf, size_t capacity,
                    const socklog_context_t *context,
                    const socklog_event_t *event);

void slogevent(int priority, const socklog_event_t *event);

#endif /* DANTE_LOGJSON_H */
