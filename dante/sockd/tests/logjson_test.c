/* Standalone serializer driver; Python checks the resulting JSON values. */
#include "logjson.h"

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int
main(int argc, char **argv)
{
   unsigned char output[SOCKS_LOG_JSON_MAX + 64];
   char input[SOCKS_LOG_JSON_MAX * 3], metadata[4097], *message;
   socklog_context_t context = {1789257600, 123456, 1234,
                               "info", "mother", "danted"};
   socklog_counters_t counters = {0};
   socklog_event_t event = {SOCKLOG_MESSAGE, input, 0, 0, NULL, NULL};
   socklog_endpoint_t ipv4 = {"ipv4", "192.0.2.3", 1080, 0, 0};
   socklog_endpoint_t ipv6 = {"ipv6", "fe80::1", 0, 9, 1};
   socklog_endpoint_t domain = {"domain", "例え.example", 443, 0, 0};
   const char *hostids[] = {"192.0.2.8", "é\"\n\377"};
   socklog_address_t source = {&ipv4, &ipv6, "username", "é\"\n\377",
                               hostids, 2};
   socklog_address_t destination = {NULL, &domain, NULL, NULL, NULL, 0};
   socklog_connection_t connection = {0};
   size_t capacity, length, i;

   assert(argc == 3);
   capacity = (size_t)strtoul(argv[1], NULL, 10);
   assert(capacity <= sizeof(output) - 2);
   event.message_len = fread(input, 1, sizeof(input), stdin);
   assert(!ferror(stdin));
   /* ASan can detect reads past the supplied message length. */
   message = malloc(event.message_len == 0 ? 1 : event.message_len);
   assert(message != NULL);
   memcpy(message, input, event.message_len);
   event.message = message;
   memset(output, 0xa5, sizeof(output));
   memset(metadata, 'M', sizeof(metadata) - 1);
   metadata[sizeof(metadata) - 1] = '\0';

   if (strncmp(argv[2], "connection", 10) == 0) {
      event.connection = &connection;
      event.type = SOCKLOG_CONNECT;
      if (strcmp(argv[2], "connection_zero") != 0) {
         connection.rule_number = UINT64_MAX;
         connection.rule_type = "socks";
         connection.verdict = "pass";
         connection.protocol = "tcp";
         connection.command = "connect";
         connection.source = &source;
         connection.destination = &destination;
         connection.source_proxy = &destination;
         connection.destination_proxy = &source;
         connection.error_isset = 1;
         connection.error_code = -123;
         connection.detail = message;
         connection.detail_len = event.message_len;
         connection.io_bytes_isset = 1;
         connection.io_bytes = UINT64_MAX;
         connection.payload_isset = 1;
         connection.payload = message;
         connection.payload_len = event.message_len;
         connection.tcp_info = "rtt: 3\nquote: \"\\";
         if (strcmp(argv[2], "connection_pressure") == 0) {
            ipv4.address = metadata;
            connection.rule_type = metadata;
            connection.command = metadata;
            source.auth_user = metadata;
            connection.tcp_info = metadata;
         }
      }
      else {
         connection.error_isset = 1;
         connection.io_bytes_isset = 1;
         connection.payload_isset = 1;
         connection.detail = "";
         connection.source = &destination;
         destination.peer = &ipv6;
         destination.auth_method = "none";
         ipv6.scope_id = 0;
      }
      if (strcmp(argv[2], "connection_sparse") == 0) {
         memset(&connection, 0, sizeof(connection));
         memset(&source, 0, sizeof(source));
         memset(&destination, 0, sizeof(destination));
         connection.source = &source;
         connection.destination = &destination;
         source.auth_user = "user-only";
      }
   }
   else if (strncmp(argv[2], "session", 7) == 0) {
      event.type = SOCKLOG_SESSION_CLOSE;
      event.connection = &connection;
      event.counters = &counters;
      connection.scope = "session";
      connection.reason = "io_error";
      connection.side = "target";
      connection.timeout = "quoted\"\n\377";
      connection.error_isset = 1;
      connection.error_code = 61;
      counters.present = SOCKLOG_COUNTER_ALL;
      counters.duration_us = UINT64_MAX;
      counters.client_bytes_read = 1;
      counters.client_bytes_written = 2;
      counters.target_bytes_read = 3;
      counters.target_bytes_written = 4;
      counters.client_packets_read = 5;
      counters.client_packets_written = 6;
      counters.target_packets_read = 7;
      counters.target_packets_written = 8;
      if (strcmp(argv[2], "session_snapshot") == 0) {
         event.type = SOCKLOG_SESSION_SNAPSHOT;
         connection.scope = "udp_target";
         connection.reason = connection.side = connection.timeout = NULL;
         connection.error_isset = 0;
         connection.idle_isset = 1;
      }
      if (strcmp(argv[2], "session_pressure") == 0)
         connection.reason = connection.side = connection.timeout = metadata;
   }
   else if (strcmp(argv[2], "metadata") == 0)
      context.level = context.process = context.program = metadata;
   else if (strcmp(argv[2], "metadata_unicode") == 0) {
      context.level = "info\"\n\377";
      context.process = "\360";
      context.program = "\303\251\342\202\254\360\237\230\200";
   }
   else if (strcmp(argv[2], "counters") == 0) {
      counters.present = SOCKLOG_COUNTER_ALL;
      counters.duration_us = counters.client_bytes_read = UINT64_MAX;
      counters.client_bytes_written = counters.target_bytes_read = UINT64_MAX;
      counters.target_bytes_written = counters.client_packets_read = UINT64_MAX;
      counters.client_packets_written = counters.target_packets_read = UINT64_MAX;
      counters.target_packets_written = UINT64_MAX;
      event.counters = &counters;
      event.type = SOCKLOG_DISCONNECT;
   }
   else if (strcmp(argv[2], "zero") == 0) {
      counters.present = SOCKLOG_COUNTER_DURATION_US
                       | SOCKLOG_COUNTER_TARGET_BYTES_WRITTEN;
      event.counters = &counters;
      event.type = SOCKLOG_SESSION_SNAPSHOT;
   }
   else if (strcmp(argv[2], "truncated") == 0)
      event.truncated = 1;
   else if (strcmp(argv[2], "negative") == 0) {
      context.seconds = context.pid = INT64_MIN;
      context.microseconds = 1;
   }
   else if (strcmp(argv[2], "null") == 0) {
      context.level = context.process = context.program = NULL;
      event.message = NULL;
      event.message_len = 0;
   }
   else if (strncmp(argv[2], "event", 5) == 0)
      event.type = (socklog_event_type_t)atoi(argv[2] + 5);
   else
      assert(strcmp(argv[2], "message") == 0);

   errno = EDOM;
   length = socks_logjson((char *)output + 1, capacity, &context, &event);
   assert(errno == EDOM);
   assert(output[0] == 0xa5);
   for (i = capacity + 1; i < sizeof(output); ++i)
      assert(output[i] == 0xa5);
   if (capacity < SOCKS_LOG_JSON_MIN)
      assert(length == 0);
   else {
      assert(length > 0 && length < capacity);
      assert(length < SOCKS_LOG_JSON_MAX);
      assert(output[length + 1] == '\0');
      assert(output[length] == '\n');
      assert(memchr(output + 1, '\n', length - 1) == NULL);
      assert(fwrite(output + 1, 1, length, stdout) == length);
   }
   free(message);
   return 0;
}
