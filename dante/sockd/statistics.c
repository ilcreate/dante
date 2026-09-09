/*
 * Copyright (c) 2011, 2012, 2013
 *      Inferno Nettverk A/S, Norway.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. The above copyright notice, this list of conditions and the following
 *    disclaimer must appear in all copies of the software, derivative works
 *    or modified versions, and any portions thereof, aswell as in all
 *    supporting documentation.
 * 2. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by
 *      Inferno Nettverk A/S, Norway.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * Inferno Nettverk A/S requests users of this software to return to
 *
 *  Software Distribution Coordinator  or  sdc@inet.no
 *  Inferno Nettverk A/S
 *  Oslo Research Park
 *  Gaustadalléen 21
 *  NO-0349 Oslo
 *  Norway
 *
 * any improvements or extensions that they make and grant Inferno Nettverk A/S
 * the rights to redistribute these changes.
 *
 */

#include "common.h"

#if SOCKD_STATS_TEST
#undef close
#undef snprintf
#undef socket
#endif /* SOCKD_STATS_TEST */

static const char rcsid[] =
"$Id: statistics.c,v 1.33 2013/10/27 15:24:43 karls Exp $";

#define SOCKD_STATS_REQUEST_MAX (4096)
#define SOCKD_STATS_BODY_MAX (8192)
#define SOCKD_STATS_RESPONSE_MAX (16384)
#define SOCKD_STATS_IO_TIMEOUT_SECONDS (1)

static void
add_counter(uint64_t *counter, const uint64_t value)
{
   if (UINT64_MAX - *counter < value)
      *counter = UINT64_MAX;
   else
      *counter += value;
}

void
sockd_stats_init(sockd_stats_t *stats, const time_t started_at)
{
   bzero(stats, sizeof(*stats));
   stats->schema_version = SOCKD_STATS_SCHEMA_VERSION;
   stats->schema_revision = SOCKD_STATS_SCHEMA_REVISION;
   stats->started_at     = started_at;
}

void
sockd_stats_add(sockd_stats_t *stats, const sockd_stat_event_t event,
                const uint64_t value)
{
   switch (event) {
      case SOCKD_STAT_CLIENT_ACCEPTED:
         add_counter(&stats->client_connections_accepted, value);
         break;

      case SOCKD_STAT_CLIENT_DROPPED:
         add_counter(&stats->client_connections_dropped, value);
         break;

      case SOCKD_STAT_NEGOTIATION_FAILED:
         add_counter(&stats->negotiation_failures, value);
         break;

      case SOCKD_STAT_REQUEST_FAILED:
         add_counter(&stats->request_failures, value);
         break;

      case SOCKD_STAT_SESSION_ESTABLISHED:
         add_counter(&stats->sessions_established, value);
         add_counter(&stats->sessions_active, value);
         break;

      case SOCKD_STAT_SESSION_CLOSED:
         add_counter(&stats->sessions_closed, value);
         if (value >= stats->sessions_active)
            stats->sessions_active = 0;
         else
            stats->sessions_active -= value;
         break;

      case SOCKD_STAT_SESSION_ERROR:
         add_counter(&stats->session_errors, value);
         break;

      case SOCKD_STAT_CLIENT_READ_BYTES:
         add_counter(&stats->client_read_bytes, value);
         break;

      case SOCKD_STAT_CLIENT_WRITTEN_BYTES:
         add_counter(&stats->client_written_bytes, value);
         break;

      case SOCKD_STAT_TARGET_READ_BYTES:
         add_counter(&stats->target_read_bytes, value);
         break;

      case SOCKD_STAT_TARGET_WRITTEN_BYTES:
         add_counter(&stats->target_written_bytes, value);
         break;
   }
}

static sockd_stats_negotiation_t
normalize_negotiation(const sockd_stats_negotiation_t outcome)
{
   switch (outcome) {
      case SOCKD_STATS_NEGOTIATION_SUCCESS:
      case SOCKD_STATS_NEGOTIATION_EOF:
      case SOCKD_STATS_NEGOTIATION_ERROR:
      case SOCKD_STATS_NEGOTIATION_TIMEOUT:
      case SOCKD_STATS_NEGOTIATION_UNKNOWN:
         return outcome;

      case SOCKD_STATS_NEGOTIATION_COUNT:
         break;
   }

   return SOCKD_STATS_NEGOTIATION_UNKNOWN;
}

static sockd_stats_command_t
normalize_command(const int command)
{
   switch (command) {
      case SOCKS_CONNECT:
         return SOCKD_STATS_COMMAND_CONNECT;

      case SOCKS_BIND:
         return SOCKD_STATS_COMMAND_BIND;

      case SOCKS_UDPASSOCIATE:
         return SOCKD_STATS_COMMAND_UDP_ASSOCIATE;
   }

   return SOCKD_STATS_COMMAND_UNKNOWN;
}

static sockd_stats_result_t
normalize_result(const iostatus_t result)
{
   switch (result) {
      case IO_NOERROR:
         return SOCKD_STATS_RESULT_SUCCESS;

      case IO_BLOCK:
      case IO_TMPBLOCK:
         return SOCKD_STATS_RESULT_BLOCKED;

      case IO_TIMEOUT:
         return SOCKD_STATS_RESULT_TIMEOUT;

      case IO_IOERROR:
         return SOCKD_STATS_RESULT_NETWORK_ERROR;

      case IO_ERROR:
         return SOCKD_STATS_RESULT_INTERNAL_ERROR;

      case IO_CLOSE:
         return SOCKD_STATS_RESULT_CLOSED;

      case IO_ADMINTERMINATION:
         return SOCKD_STATS_RESULT_ADMIN;

      case IO_TMPERROR:
      case IO_EAGAIN:
         return SOCKD_STATS_RESULT_OTHER;
   }

   return SOCKD_STATS_RESULT_OTHER;
}

static sockd_stats_protocol_t
normalize_protocol(const int protocol)
{
   switch (protocol) {
      case SOCKS_TCP:
         return SOCKD_STATS_PROTOCOL_TCP;

      case SOCKS_UDP:
         return SOCKD_STATS_PROTOCOL_UDP;
   }

   return SOCKD_STATS_PROTOCOL_UNKNOWN;
}

static sockd_stats_close_t
normalize_close(const iostatus_t status)
{
   switch (status) {
      case IO_NOERROR:
         return SOCKD_STATS_CLOSE_NORMAL;

      case IO_BLOCK:
      case IO_TMPBLOCK:
         return SOCKD_STATS_CLOSE_BLOCKED;

      case IO_TIMEOUT:
         return SOCKD_STATS_CLOSE_TIMEOUT;

      case IO_IOERROR:
         return SOCKD_STATS_CLOSE_NETWORK_ERROR;

      case IO_ERROR:
         return SOCKD_STATS_CLOSE_INTERNAL_ERROR;

      case IO_CLOSE:
         return SOCKD_STATS_CLOSE_PEER;

      case IO_ADMINTERMINATION:
         return SOCKD_STATS_CLOSE_ADMIN;

      case IO_TMPERROR:
      case IO_EAGAIN:
         return SOCKD_STATS_CLOSE_OTHER;
   }

   return SOCKD_STATS_CLOSE_OTHER;
}

static int
status_is_error(const iostatus_t status)
{
   return status == IO_IOERROR || status == IO_ERROR || status == IO_TIMEOUT;
}

static void
subtract_gauge(uint64_t *gauge, const uint64_t value)
{
   if (value >= *gauge)
      *gauge = 0;
   else
      *gauge -= value;
}

void
sockd_stats_add_negotiation(sockd_stats_t *stats,
                            const sockd_stats_negotiation_t outcome,
                            const uint64_t value)
{
   const sockd_stats_negotiation_t normalized
   = normalize_negotiation(outcome);

   add_counter(&stats->negotiation_outcome[normalized], value);
   if (normalized != SOCKD_STATS_NEGOTIATION_SUCCESS)
      add_counter(&stats->negotiation_failures, value);
}

void
sockd_stats_add_request(sockd_stats_t *stats, const int command,
                        const iostatus_t result, const uint64_t value)
{
   const sockd_stats_command_t normalized_command = normalize_command(command);
   const sockd_stats_result_t normalized_result = normalize_result(result);

   add_counter(&stats->request_outcome[normalized_command][normalized_result],
               value);
   if (normalized_result != SOCKD_STATS_RESULT_SUCCESS)
      add_counter(&stats->request_failures, value);
}

void
sockd_stats_add_session_started(sockd_stats_t *stats, const int protocol,
                                const uint64_t value)
{
   sockd_stats_session_t *session
   = &stats->sessions[normalize_protocol(protocol)];

   sockd_stats_add(stats, SOCKD_STAT_SESSION_ESTABLISHED, value);
   add_counter(&session->started, value);
   add_counter(&session->active, value);
}

void
sockd_stats_add_session_closed(sockd_stats_t *stats, const int protocol,
                               const iostatus_t status, const uint64_t value)
{
   sockd_stats_session_t *session
   = &stats->sessions[normalize_protocol(protocol)];

   sockd_stats_add(stats, SOCKD_STAT_SESSION_CLOSED, value);
   add_counter(&session->closed, value);
   subtract_gauge(&session->active, value);
   add_counter(&stats->session_close_reason[normalize_close(status)], value);

   if (status_is_error(status)) {
      sockd_stats_add(stats, SOCKD_STAT_SESSION_ERROR, value);
      add_counter(&session->errors, value);
   }
}

#if !SOCKD_STATS_TEST
void
sockd_stats_update(const sockd_stat_event_t event, const uint64_t value)
{
   if (sockscf.shmeminfo == NULL)
      return;

   socks_lock(sockscf.shmemfd, (off_t)0, 1, 1, 1);
   sockd_stats_add(&sockscf.shmeminfo->stats, event, value);
   socks_unlock(sockscf.shmemfd, (off_t)0, 1);
}

void
sockd_stats_update_negotiation(const sockd_stats_negotiation_t outcome,
                               const uint64_t value)
{
   if (sockscf.shmeminfo == NULL)
      return;

   socks_lock(sockscf.shmemfd, (off_t)0, 1, 1, 1);
   sockd_stats_add_negotiation(&sockscf.shmeminfo->stats, outcome, value);
   socks_unlock(sockscf.shmemfd, (off_t)0, 1);
}

void
sockd_stats_update_request(const int command, const iostatus_t result,
                           const uint64_t value)
{
   if (sockscf.shmeminfo == NULL)
      return;

   socks_lock(sockscf.shmemfd, (off_t)0, 1, 1, 1);
   sockd_stats_add_request(&sockscf.shmeminfo->stats, command, result, value);
   socks_unlock(sockscf.shmemfd, (off_t)0, 1);
}

void
sockd_stats_update_session_started(const int protocol, const uint64_t value)
{
   if (sockscf.shmeminfo == NULL)
      return;

   socks_lock(sockscf.shmemfd, (off_t)0, 1, 1, 1);
   sockd_stats_add_session_started(&sockscf.shmeminfo->stats, protocol, value);
   socks_unlock(sockscf.shmemfd, (off_t)0, 1);
}

void
sockd_stats_update_session_closed(const int protocol,
                                  const iostatus_t status,
                                  const uint64_t value)
{
   if (sockscf.shmeminfo == NULL)
      return;

   socks_lock(sockscf.shmemfd, (off_t)0, 1, 1, 1);
   sockd_stats_add_session_closed(&sockscf.shmeminfo->stats,
                                  protocol,
                                  status,
                                  value);
   socks_unlock(sockscf.shmemfd, (off_t)0, 1);
}

void
sockd_stats_update_io(const uint64_t client_read,
                      const uint64_t client_written,
                      const uint64_t target_read,
                      const uint64_t target_written)
{
   sockd_stats_t *stats;

   if (sockscf.shmeminfo == NULL
   ||  (client_read == 0 && client_written == 0
     && target_read == 0 && target_written == 0))
      return;

   socks_lock(sockscf.shmemfd, (off_t)0, 1, 1, 1);
   stats = &sockscf.shmeminfo->stats;
   sockd_stats_add(stats, SOCKD_STAT_CLIENT_READ_BYTES, client_read);
   sockd_stats_add(stats, SOCKD_STAT_CLIENT_WRITTEN_BYTES, client_written);
   sockd_stats_add(stats, SOCKD_STAT_TARGET_READ_BYTES, target_read);
   sockd_stats_add(stats, SOCKD_STAT_TARGET_WRITTEN_BYTES, target_written);
   socks_unlock(sockscf.shmemfd, (off_t)0, 1);
}

void
sockd_stats_snapshot(sockd_stats_t *stats)
{
   if (sockscf.shmeminfo == NULL) {
      sockd_stats_init(stats, (time_t)0);
      return;
   }

   socks_lock(sockscf.shmemfd, (off_t)0, 1, 1, 1);
   *stats = sockscf.shmeminfo->stats;
   socks_unlock(sockscf.shmemfd, (off_t)0, 1);
}
#endif /* !SOCKD_STATS_TEST */

typedef struct {
   char   *data;
   size_t size;
   size_t used;
   int    failed;
} stats_buffer_t;

static void stats_buffer_append(stats_buffer_t *buffer, const char *format, ...)
   __ATTRIBUTE__((FORMAT(printf, 2, 3)));

static void
stats_buffer_append(stats_buffer_t *buffer, const char *format, ...)
{
   va_list ap;
   size_t available;
   int length;

   if (buffer->failed)
      return;

   if (buffer->used >= buffer->size) {
      buffer->failed = 1;
      errno = ENOSPC;
      return;
   }

   available = buffer->size - buffer->used;
   va_start(ap, format);
   length = vsnprintf(buffer->data + buffer->used, available, format, ap);
   va_end(ap);

   if (length < 0) {
      buffer->failed = 1;
      if (errno == 0)
         errno = EINVAL;
      return;
   }

   if ((size_t)length >= available) {
      buffer->failed = 1;
      errno = ENOSPC;
      return;
   }

   buffer->used += (size_t)length;
}

ssize_t
sockd_stats_json(const sockd_stats_t *stats, const time_t now,
                 char *response, const size_t responsesize)
{
   static const char *const negotiation_names[] = {
      "success", "eof", "error", "timeout", "unknown"
   };
   static const char *const command_names[] = {
      "connect", "bind", "udp_associate", "unknown"
   };
   static const char *const result_names[] = {
      "success", "blocked", "timeout", "network_error", "internal_error",
      "closed", "admin", "other"
   };
   static const char *const protocol_names[] = {
      "tcp", "udp", "unknown"
   };
   static const char *const close_names[] = {
      "normal", "blocked", "timeout", "network_error", "internal_error",
      "peer_closed", "admin", "other"
   };
   const intmax_t uptime = now > stats->started_at ?
                              (intmax_t)(now - stats->started_at) : 0;
   stats_buffer_t output = { response, responsesize, 0, 0 };
   size_t command, negotiation, protocol, result, reason;

   SASSERTX(ELEMENTS(negotiation_names) == SOCKD_STATS_NEGOTIATION_COUNT);
   SASSERTX(ELEMENTS(command_names) == SOCKD_STATS_COMMAND_COUNT);
   SASSERTX(ELEMENTS(result_names) == SOCKD_STATS_RESULT_COUNT);
   SASSERTX(ELEMENTS(protocol_names) == SOCKD_STATS_PROTOCOL_COUNT);
   SASSERTX(ELEMENTS(close_names) == SOCKD_STATS_CLOSE_COUNT);

   stats_buffer_append(&output,
      "{\"schema_version\":%u,"
      "\"schema_revision\":%u,"
      "\"server_version\":\"%s\","
      "\"snapshot_time\":%"PRIdMAX","
      "\"started_at\":%"PRIdMAX","
      "\"uptime_seconds\":%"PRIdMAX","
      "\"counters\":{"
         "\"client_connections_accepted_total\":%"PRIu64","
         "\"client_connections_dropped_total\":%"PRIu64","
         "\"negotiation_failures_total\":%"PRIu64","
         "\"request_failures_total\":%"PRIu64","
         "\"sessions_established_total\":%"PRIu64","
         "\"sessions_closed_total\":%"PRIu64","
         "\"session_errors_total\":%"PRIu64","
         "\"client_read_bytes_total\":%"PRIu64","
         "\"client_written_bytes_total\":%"PRIu64","
         "\"target_read_bytes_total\":%"PRIu64","
         "\"target_written_bytes_total\":%"PRIu64
      "},"
      "\"gauges\":{\"sessions_active\":%"PRIu64"},"
      "\"details\":{\"negotiations\":{",
      stats->schema_version,
      stats->schema_revision,
      VERSION,
      (intmax_t)now,
      (intmax_t)stats->started_at,
      uptime,
      stats->client_connections_accepted,
      stats->client_connections_dropped,
      stats->negotiation_failures,
      stats->request_failures,
      stats->sessions_established,
      stats->sessions_closed,
      stats->session_errors,
      stats->client_read_bytes,
      stats->client_written_bytes,
      stats->target_read_bytes,
      stats->target_written_bytes,
      stats->sessions_active);

   for (negotiation = 0;
        negotiation < SOCKD_STATS_NEGOTIATION_COUNT;
        ++negotiation) {
      stats_buffer_append(&output,
                          "%s\"%s_total\":%"PRIu64,
                          negotiation == 0 ? "" : ",",
                          negotiation_names[negotiation],
                          stats->negotiation_outcome[negotiation]);
   }

   stats_buffer_append(&output, "},\"requests\":{");
   for (command = 0; command < SOCKD_STATS_COMMAND_COUNT; ++command) {
      stats_buffer_append(&output,
                          "%s\"%s\":{",
                          command == 0 ? "" : ",",
                          command_names[command]);

      for (result = 0; result < SOCKD_STATS_RESULT_COUNT; ++result) {
         stats_buffer_append(&output,
                             "%s\"%s_total\":%"PRIu64,
                             result == 0 ? "" : ",",
                             result_names[result],
                             stats->request_outcome[command][result]);
      }

      stats_buffer_append(&output, "}");
   }

   stats_buffer_append(&output, "},\"sessions\":{");
   for (protocol = 0; protocol < SOCKD_STATS_PROTOCOL_COUNT; ++protocol) {
      const sockd_stats_session_t *session = &stats->sessions[protocol];

      stats_buffer_append(&output,
                          "%s\"%s\":{"
                          "\"started_total\":%"PRIu64","
                          "\"active\":%"PRIu64","
                          "\"closed_total\":%"PRIu64","
                          "\"errors_total\":%"PRIu64"}",
                          protocol == 0 ? "" : ",",
                          protocol_names[protocol],
                          session->started,
                          session->active,
                          session->closed,
                          session->errors);
   }

   stats_buffer_append(&output, "},\"session_closures\":{");
   for (reason = 0; reason < SOCKD_STATS_CLOSE_COUNT; ++reason) {
      stats_buffer_append(&output,
                          "%s\"%s_total\":%"PRIu64,
                          reason == 0 ? "" : ",",
                          close_names[reason],
                          stats->session_close_reason[reason]);
   }

   stats_buffer_append(&output, "}}}\n");

   if (output.failed)
      return -1;

   return (ssize_t)output.used;
}

static ssize_t
format_http_response(const char *status, const char *extra_headers,
                     const char *body, char *response,
                     const size_t responsesize)
{
   int length;

   length = snprintf(response, responsesize,
                     "HTTP/1.1 %s\r\n"
                     "Content-Type: application/json\r\n"
                     "Content-Length: %lu\r\n"
                     "Cache-Control: no-store\r\n"
                     "Connection: close\r\n"
                     "%s"
                     "\r\n"
                     "%s",
                     status,
                     (unsigned long)strlen(body),
                     extra_headers == NULL ? "" : extra_headers,
                     body);

   if (length < 0 || (size_t)length >= responsesize) {
      errno = ENOSPC;
      return -1;
   }

   return length;
}

static int
stats_setnonblocking(const int s)
{
   int flags;

   if ((flags = fcntl(s, F_GETFL, 0)) == -1)
      return -1;

   return fcntl(s, F_SETFL, flags | O_NONBLOCK);
}

static int
stats_wait_readable(const int s)
{
   struct timeval timeout;
   fd_set rset;
   int rc;

   do {
      bzero(&rset, sizeof(rset));
      FD_SET(s, &rset);
      timeout.tv_sec  = SOCKD_STATS_IO_TIMEOUT_SECONDS;
      timeout.tv_usec = 0;
      rc = select(s + 1, &rset, NULL, NULL, &timeout);
   } while (rc == -1 && errno == EINTR);

   if (rc == 0) {
      errno = ETIMEDOUT;
      return -1;
   }

   return rc;
}

ssize_t
sockd_stats_http_response(const char *request, const size_t requestlen,
                          const sockd_stats_t *stats, const time_t now,
                          char *response, const size_t responsesize)
{
   static const char bad_request[] = "{\"error\":\"bad request\"}\n";
   static const char not_found[] = "{\"error\":\"not found\"}\n";
   static const char method_not_allowed[] =
      "{\"error\":\"method not allowed\"}\n";
   char line[512], method[16], path[256], version[16];
   char body[SOCKD_STATS_BODY_MAX];
   size_t linelen;
   ssize_t bodylen;

   for (linelen = 0; linelen < requestlen && request[linelen] != '\n';
        ++linelen)
      /* LINTED */ /* EMPTY */;

   if (linelen == requestlen || linelen == 0 || linelen >= sizeof(line))
      return format_http_response("400 Bad Request", NULL, bad_request,
                                  response, responsesize);

   if (request[linelen - 1] == '\r')
      --linelen;

   memcpy(line, request, linelen);
   line[linelen] = NUL;

   if (sscanf(line, "%15s %255s %15s", method, path, version) != 3
   ||  (strcmp(version, "HTTP/1.0") != 0
     && strcmp(version, "HTTP/1.1") != 0))
      return format_http_response("400 Bad Request", NULL, bad_request,
                                  response, responsesize);

   if (strcmp(method, "GET") != 0)
      return format_http_response("405 Method Not Allowed", "Allow: GET\r\n",
                                  method_not_allowed, response, responsesize);

   if (strcmp(path, "/v1/stats") != 0)
      return format_http_response("404 Not Found", NULL, not_found,
                                  response, responsesize);

   bodylen = sockd_stats_json(stats, now, body, sizeof(body));
   if (bodylen == -1)
      return -1;

   return format_http_response("200 OK", NULL, body, response, responsesize);
}

int
sockd_stats_api_open(const char *path)
{
   struct sockaddr_un address;
   struct stat st;
   int s, errno_s;

   if (path == NULL || *path == NUL) {
      errno = EINVAL;
      return -1;
   }

   if (strlen(path) >= sizeof(address.sun_path)) {
      errno = ENAMETOOLONG;
      return -1;
   }

   if (lstat(path, &st) == 0) {
      if (!S_ISSOCK(st.st_mode)) {
         errno = EEXIST;
         return -1;
      }

      if (unlink(path) == -1)
         return -1;
   }
   else if (errno != ENOENT)
      return -1;

   if ((s = socket(AF_UNIX, SOCK_STREAM, 0)) == -1)
      return -1;

   bzero(&address, sizeof(address));
   address.sun_family = AF_UNIX;
#if HAVE_SOCKADDR_SA_LEN
   address.sun_len = sizeof(address);
#endif /* HAVE_SOCKADDR_SA_LEN */
   memcpy(address.sun_path, path, strlen(path) + 1);

   if (bind(s, (struct sockaddr *)&address, sizeof(address)) == -1
   ||  chmod(path, S_IRUSR | S_IWUSR) == -1
   ||  listen(s, 8) == -1
   ||  stats_setnonblocking(s) == -1) {
      errno_s = errno;
      close(s);
      unlink(path);
      errno = errno_s;
      return -1;
   }

   return s;
}

int
sockd_stats_api_serve(const int s, const sockd_stats_t *stats,
                      const time_t now)
{
   char request[SOCKD_STATS_REQUEST_MAX];
   char response[SOCKD_STATS_RESPONSE_MAX];
   ssize_t len, sent, written;
   int client, errno_s, rc = 0;

   if ((client = accept(s, NULL, NULL)) == -1)
      return -1;

   if (stats_setnonblocking(client) == -1) {
      errno_s = errno;
      close(client);
      errno = errno_s;
      return -1;
   }

   if (stats_wait_readable(client) == -1) {
      errno_s = errno;
      close(client);
      errno = errno_s;
      return -1;
   }

   len = recv(client, request, sizeof(request), 0);
   if (len <= 0)
      rc = -1;
   else {
      len = sockd_stats_http_response(request, (size_t)len, stats, now,
                                      response, sizeof(response));
      if (len == -1)
         rc = -1;
      else {
         for (sent = 0; sent < len; sent += written) {
            written = send(client, response + sent, (size_t)(len - sent), 0);
            if (written == -1) {
               if (errno == EINTR) {
                  written = 0;
                  continue;
               }

               rc = -1;
               break;
            }
            else if (written == 0) {
               errno = EPIPE;
               rc = -1;
               break;
            }
         }
      }
   }

   errno_s = errno;
   close(client);
   errno = errno_s;
   return rc;
}

void
sockd_stats_api_close(const int s, const char *path)
{
   if (s != -1)
      close(s);

   sockd_stats_api_cleanup(path);
}

void
sockd_stats_api_cleanup(const char *path)
{
   struct stat st;

   if (path != NULL && lstat(path, &st) == 0 && S_ISSOCK(st.st_mode))
      unlink(path);
}


int
sockd_check_ipclatency(description, tsent, treceived, tnow)
   const char *description;
   const struct timeval *tsent;
   const struct timeval *treceived;
   const struct timeval *tnow;
{
#if DIAGNOSTIC
   const char *function = "sockd_check_ipclatency()";
   const size_t samplesneeded             = 1000,
                minoccurences             = 10;
   const time_t tseconds_between_warnings = 60;
   static struct timeval tmaxdelay, tmaxdelay_so_far;
   static size_t samplec;
   struct timeval tdiff;

   timersub(treceived, tsent, &tdiff);

   if (tdiff.tv_sec < 0) {
      swarnx("%s: strange ... received ts (%ld.%06ld) is earlier than "
             "sent ts (%ld.%06ld) .  Did the clock step backwards?",
             function,
             (long)treceived->tv_sec,
             (long)treceived->tv_usec,
             (long)tsent->tv_sec,
             (long)tsent->tv_usec);

      return 0;
   }
   else
      slog(LOG_DEBUG, "%s: %s: used %luus to receive object",
           function, description, tv2us(&tdiff));

   if (timerisset(&tmaxdelay)) {
      if (timercmp(&tdiff, &tmaxdelay, >)) {
         static time_t tlastwarn, tlongest;
         static size_t overloadc;

         ++overloadc;

         if ((time_t)tv2us(&tdiff) > tlongest)
            tlongest = (time_t)tv2us(&tdiff);

         if (socks_difftime(tnow->tv_sec, tlastwarn)
         >= tseconds_between_warnings) {
            if (overloadc >= minoccurences)
               slog(LOG_NOTICE,
                    "server overload condition detected %lu time%s regarding "
                    "%s.  Used up to %ldus to receive new client objects "
                    "during the last %lds, but expected maximum was "
                    "calibrated to %luus",
                    (unsigned long)overloadc,
                    (unsigned long)overloadc == 1 ? "" : "s",
                    description,
                    (long)tlongest,
                    (long)socks_difftime(tnow->tv_sec, tlastwarn),
                    tv2us(&tmaxdelay));

            tlastwarn = tnow->tv_sec;
            overloadc = 0;
            tlongest  = 0;
         }

         return 1;
      }

      return 0;
   }

   if (timercmp(&tdiff, &tmaxdelay_so_far, >))
      tmaxdelay_so_far = tdiff;

   if (samplec == samplesneeded) {
      tmaxdelay = tmaxdelay_so_far;

      slog(DEBUG ? LOG_INFO : LOG_DEBUG,
           "%s: max IPC delay for this %s process calibrated to be %ld.%06lds",
           function,
           childtype2string(sockscf.state.type),
           (long)tmaxdelay.tv_sec,
           (long)tmaxdelay.tv_usec);
   }
   else
      ++samplec;
#endif /* DIAGNOSTIC */

   return 0;
}
