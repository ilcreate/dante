/* Real logger benchmark: no wrappers, deterministic inputs, one recipient. */
#include "common.h"
#include "logjson.h"
#include <assert.h>
#include <sys/resource.h>

static double seconds(const struct timeval *tv)
{
   return (double)tv->tv_sec + (double)tv->tv_usec / 1000000.0;
}

int main(int argc, char **argv)
{
   int out = STDOUT_FILENO;
   unsigned long i, count;
   char *end;
   struct timeval start, finish;
   struct rusage before, after;
   socklog_event_t event;
   socklog_connection_t connection;
   socklog_counters_t counters;
   socklog_endpoint_t client = { "ipv4", "127.0.0.1", 49152, 0, 0 };
   socklog_endpoint_t target = { "ipv4", "127.0.0.1", 8080, 0, 0 };
   socklog_address_t source, destination;

   assert(argc == 3);
   assert(strcmp(argv[1], "raw") == 0 || strcmp(argv[1], "json") == 0);
   count = strtoul(argv[2], &end, 10);
   assert(*end == '\0' && count > 0);
   __progname = "logformat-benchmark";
   sockscf.state.pid = getpid();
   sockscf.state.type = PROC_IO;
   sockscf.logformat = strcmp(argv[1], "json") == 0 ? LOGFORMAT_JSON : LOGFORMAT_RAW;
   sockscf.log.type = LOGTYPE_FILE;
   sockscf.log.filenoc = 1;
   sockscf.log.filenov = &out;
   memset(&event, 0, sizeof(event));
   memset(&connection, 0, sizeof(connection));
   memset(&counters, 0, sizeof(counters));
   memset(&source, 0, sizeof(source));
   memset(&destination, 0, sizeof(destination));
   source.peer = &client;
   destination.peer = &target;
   connection.rule_number = 1;
   connection.rule_type = "socks-rule";
   connection.verdict = "pass";
   connection.protocol = "tcp";
   connection.command = "connect";
   connection.reason = "closed";
   connection.side = "client";
   connection.scope = "session";
   connection.source = &source;
   connection.destination = &destination;
   counters.present = SOCKLOG_COUNTER_DURATION_US
                    | SOCKLOG_COUNTER_CLIENT_BYTES_READ
                    | SOCKLOG_COUNTER_CLIENT_BYTES_WRITTEN
                    | SOCKLOG_COUNTER_TARGET_BYTES_READ
                    | SOCKLOG_COUNTER_TARGET_BYTES_WRITTEN;
   counters.duration_us = 123456;
   counters.client_bytes_read = counters.target_bytes_written = 4096;
   counters.target_bytes_read = counters.client_bytes_written = 1024;
   event.type = SOCKLOG_SESSION_CLOSE;
   event.message = "pass(1): tcp/connect ]: 4096 -> 127.0.0.1.49152 -> 1024: local client closed";
   event.message_len = strlen(event.message);
   event.connection = &connection;
   event.counters = &counters;
   assert(getrusage(RUSAGE_SELF, &before) == 0);
   assert(gettimeofday(&start, NULL) == 0);
   for (i = 0; i < count; ++i) {
      if ((i & 1) == 0)
         slog(LOG_INFO, "benchmark event %lu: quote\" slash\\ UTF-8 \303\251", i);
      else
         slogevent(LOG_INFO, &event);
   }
   assert(gettimeofday(&finish, NULL) == 0);
   assert(getrusage(RUSAGE_SELF, &after) == 0);
   fprintf(stderr,
           "{\"format\":\"%s\",\"events\":%lu,\"wall_seconds\":%.6f,\"cpu_seconds\":%.6f}\n",
           argv[1], count, seconds(&finish) - seconds(&start),
           seconds(&after.ru_utime) + seconds(&after.ru_stime)
            - seconds(&before.ru_utime) - seconds(&before.ru_stime));
   return 0;
}
