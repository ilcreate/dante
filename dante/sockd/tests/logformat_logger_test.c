/* Exercise the real logger, replacing only OS boundaries for fault injection. */
#include "common.h"
#include <assert.h>

static int failalloc, allocations, interrupt_write, short_write;
static void *test_malloc(size_t size);
static ssize_t test_write(int fd, const void *buf, size_t len);
static int test_gettimeofday(struct timeval *tv, void *tz);
static void test_syslog(int priority, const char *format, ...);

#define malloc test_malloc
#define write test_write
#define gettimeofday test_gettimeofday
#define syslog test_syslog
#include "../../lib/log.c"
#undef malloc
#undef write
#undef gettimeofday
#undef syslog

static void *test_malloc(size_t size)
{
   if (failalloc && ++allocations == failalloc) {
      errno = ENOMEM;
      return NULL;
   }
   return malloc(size);
}

static ssize_t test_write(int fd, const void *buf, size_t len)
{
   if (interrupt_write) {
      --interrupt_write;
      errno = EINTR;
      return -1;
   }
   if (short_write && len > 7)
      len = 7;
   return write(fd, buf, len);
}

static int test_gettimeofday(struct timeval *tv, void *tz)
{
   (void)tz;
   tv->tv_sec = 1789257600;
   tv->tv_usec = 123456;
   return 0;
}

static void test_syslog(int priority, const char *format, ...)
{
   va_list ap;
   printf("SYSLOG %d ", priority);
   va_start(ap, format);
   vprintf(format, ap);
   va_end(ap);
   putchar('\n');
}

int main(int argc, char **argv)
{
   int out = STDOUT_FILENO, err = STDERR_FILENO, i;
   char longmessage[100001];
   assert(argc == 3);
   __progname = "danted";
   sockscf.state.pid = 1234;
   sockscf.state.type = PROC_MOTHER;
   sockscf.option.debug = 1;
   sockscf.logformat = strcmp(argv[1], "json") == 0 ? LOGFORMAT_JSON : LOGFORMAT_RAW;
   sockscf.log.type = LOGTYPE_FILE;
   sockscf.log.filenoc = 1;
   sockscf.log.filenov = &out;
   memset(longmessage, 'x', sizeof(longmessage) - 1);
   longmessage[sizeof(longmessage) - 1] = '\0';
   if (strcmp(argv[2], "routing") == 0 || strcmp(argv[2], "routing-long") == 0
   ||  strcmp(argv[2], "repeat-routing") == 0) {
      sockscf.errlog.type = LOGTYPE_FILE;
      sockscf.errlog.filenoc = 1;
      sockscf.errlog.filenov = &err;
   }
   else if (strcmp(argv[2], "syslog") == 0) {
      sockscf.log.type = sockscf.errlog.type = LOGTYPE_SYSLOG;
      sockscf.log.facility = LOG_LOCAL0;
      sockscf.errlog.facility = LOG_LOCAL1;
   }
   else if (strcmp(argv[2], "fallback") == 0)
      sockscf.log.type = 0;
   else if (strcmp(argv[2], "fallback-empty") == 0)
      sockscf.log.filenoc = 0;
   else if (strcmp(argv[2], "debug") == 0)
      sockscf.option.debug = 0;
   else if (strcmp(argv[2], "eintr") == 0)
      interrupt_write = 2;
   else if (strcmp(argv[2], "short") == 0)
      short_write = 1;
   else if (strcmp(argv[2], "alloc1") == 0)
      failalloc = 1;
   else if (strcmp(argv[2], "alloc2") == 0)
      failalloc = 2;

   errno = EDOM;
   if (strcmp(argv[2], "typed") == 0) {
      socklog_event_t event;
      socklog_counters_t counters;
      memset(&event, 0, sizeof(event));
      memset(&counters, 0, sizeof(counters));
      event.type = SOCKLOG_SESSION_SNAPSHOT;
      event.message = "session data";
      event.message_len = strlen(event.message);
      counters.present = SOCKLOG_COUNTER_DURATION_US | SOCKLOG_COUNTER_CLIENT_BYTES_READ;
      counters.client_bytes_read = UINT64_MAX;
      event.counters = &counters;
      for (i = PROC_MOTHER; i <= PROC_IO; ++i) {
         sockscf.state.type = i;
         slogevent(LOG_INFO, &event);
      }
      assert(errno == EDOM);
      return 0;
   }
   if (strncmp(argv[2], "alloc", 5) == 0 || strcmp(argv[2], "long") == 0)
      slog(LOG_INFO, "%s", longmessage);
   else if (strcmp(argv[2], "routing-long") == 0 || strcmp(argv[2], "fallback-empty") == 0)
      slog(LOG_WARNING, "%s", longmessage);
   else if (strcmp(argv[2], "repeat") == 0 || strcmp(argv[2], "repeat-routing") == 0) {
      for (i = 0; i < 12; ++i)
         slog(strcmp(argv[2], "repeat-routing") == 0 ? LOG_WARNING : LOG_INFO,
              "%d %s", i, longmessage);
   }
   else if (strcmp(argv[2], "rawtext") == 0)
      slog(LOG_WARNING, "quote\" slash\\ newline\n tab\t trailing\n");
   else if (strcmp(argv[2], "empty") == 0)
      slog(LOG_INFO, "%s", "");
   else if (strcmp(argv[2], "bytes") == 0)
      slog(LOG_INFO, "quote\" slash\\ newline\n tab\t %s%c%s", "\303\251\360\237\230\200", 0, "\377\300\200");
   else
      for (i = LOG_EMERG; i <= LOG_DEBUG; ++i)
         slog(i, "level %d", i);
   assert(errno == EDOM);
   return 0;
}
