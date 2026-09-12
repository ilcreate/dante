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
   socklog_event_t event = {SOCKLOG_MESSAGE, input, 0, 0, NULL};
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

   if (strcmp(argv[2], "metadata") == 0)
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
