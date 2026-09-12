/* Real special logger paths, with guards at unsafe OS/library boundaries. */
#include "common.h"
#include <assert.h>
#if HAVE_EXECINFO_H
#include <execinfo.h>
#endif

/* Exercise the ring even in normal builds where live debugging is disabled. */
#undef HAVE_LIVEDEBUG
#define HAVE_LIVEDEBUG 1

static volatile sig_atomic_t guarded;
static int interrupt_write, short_write, fail_write, writes;
static ssize_t checked_write(int fd, const void *buf, size_t len);
static int checked_snprintf(char *s, size_t n, const char *f, ...);
static struct tm *checked_localtime(const time_t *t);
static size_t checked_strftime(char *s, size_t n, const char *f, const struct tm *t);
static void *checked_malloc(size_t n);
static void checked_free(void *p);
static int checked_vsnprintf(char *s, size_t n, const char *f, va_list ap);
static int checked_fileno(FILE *f);
static long checked_fpathconf(int fd, int name);
static int fixed_gettimeofday(struct timeval *tv, void *tz);
static void capture_syslog(int priority, const char *format, ...);
#if HAVE_BACKTRACE
static int checked_backtrace(void **buffer, int size);
static char **checked_backtrace_symbols(void *const *buffer, int size);
#endif

#define write checked_write
#undef snprintf
#define snprintf checked_snprintf
#define localtime checked_localtime
#define strftime checked_strftime
#define malloc checked_malloc
#undef free
#define free checked_free
#undef vsnprintf
#define vsnprintf checked_vsnprintf
#define fileno checked_fileno
#define fpathconf checked_fpathconf
#define gettimeofday fixed_gettimeofday
#define syslog capture_syslog
#if HAVE_BACKTRACE
#define backtrace checked_backtrace
#define backtrace_symbols checked_backtrace_symbols
#endif
#include "../../lib/log.c"
#undef write
#undef snprintf
#undef localtime
#undef strftime
#undef malloc
#undef free
#undef vsnprintf
#undef fileno
#undef fpathconf
#undef gettimeofday
#undef syslog
#undef backtrace
#undef backtrace_symbols

static void unsafe_call(void) { if (guarded) _exit(90); }
static ssize_t checked_write(int fd, const void *buf, size_t len)
{
   ++writes;
   if (fail_write) { errno = ENOSPC; return -1; }
   if (interrupt_write) { --interrupt_write; errno = EINTR; return -1; }
   if (short_write && len > 7) len = 7;
   return write(fd, buf, len);
}
static int checked_snprintf(char *s, size_t n, const char *f, ...)
{
   int result;
   va_list ap;
   unsafe_call();
   va_start(ap, f); result = vsnprintf(s, n, f, ap); va_end(ap);
   return result;
}
static struct tm *checked_localtime(const time_t *t)
{ unsafe_call(); return localtime(t); }
static size_t checked_strftime(char *s, size_t n, const char *f, const struct tm *t)
{ unsafe_call(); return strftime(s, n, f, t); }
static void *checked_malloc(size_t n) { unsafe_call(); return malloc(n); }
static void checked_free(void *p) { unsafe_call(); free(p); }
static int checked_vsnprintf(char *s, size_t n, const char *f, va_list ap)
{ unsafe_call(); return vsnprintf(s, n, f, ap); }
static int checked_fileno(FILE *f) { unsafe_call(); return fileno(f); }
static long checked_fpathconf(int fd, int name)
{ unsafe_call(); return fpathconf(fd, name); }
static int fixed_gettimeofday(struct timeval *tv, void *tz)
{ (void)tz; tv->tv_sec = 1789257600; tv->tv_usec = 123456; return 0; }
static void capture_syslog(int priority, const char *format, ...)
{
   va_list ap;
   /* Test transport only: dolog's existing critical-signal exception permits it. */
   printf("SYSLOG %d ", priority);
   va_start(ap, format); vprintf(format, ap); va_end(ap); putchar('\n');
}
#if HAVE_BACKTRACE
static int checked_backtrace(void **buffer, int size)
{ unsafe_call(); return backtrace(buffer, size); }
static char **checked_backtrace_symbols(void *const *buffer, int size)
{ unsafe_call(); return backtrace_symbols(buffer, size); }
#endif

static const char *parts[] = { "quote\" slash\\\n", "tab\t \303\251\360\237\230\200 ", "\377\300\200", NULL };
static void handle_signal(int sig)
{
   const int oldsignal = sockscf.state.insignal;
   sockscf.state.insignal = sig;
   guarded = sockscf.logformat == LOGFORMAT_JSON;
   signalslog(LOG_WARNING, parts);
   guarded = 0;
   sockscf.state.insignal = oldsignal;
}

int main(int argc, char **argv)
{
   int out = STDOUT_FILENO, err = STDERR_FILENO;
   const char *msg[] = { "first\n", "second\"\n", NULL };
   char huge[100001];
   size_t i, oldoffset;
   assert(argc == 3);
   if (strcmp(argv[2], "features") == 0) {
      printf("%d\n", HAVE_BACKTRACE);
      return 0;
   }
   __progname = "danted";
   sockscf.state.pid = 1234;
   sockscf.state.type = PROC_MOTHER;
   sockscf.option.debug = 1;
   sockscf.logformat = strcmp(argv[1], "json") == 0 ? LOGFORMAT_JSON : LOGFORMAT_RAW;
   sockscf.log.type = LOGTYPE_FILE;
   sockscf.log.filenoc = 1;
   sockscf.log.filenov = &out;
   memset(huge, 'x', sizeof(huge) - 1); huge[sizeof(huge) - 1] = NUL;
   errno = EDOM;
   if (strcmp(argv[2], "ring") == 0 || strcmp(argv[2], "ring-wrap") == 0) {
      sockscf.option.debug = 0;
      if (strcmp(argv[2], "ring-wrap") == 0)
         for (i = 0; i < 2000; ++i)
            slog(LOG_DEBUG, "old-%lu", (unsigned long)i);
      slog(LOG_DEBUG, "suppressed ordinary");
      signalslog(LOG_DEBUG, msg);
      oldoffset = ringbuf_curroff;
      guarded = sockscf.logformat == LOGFORMAT_JSON;
      sockscf.state.insignal = SIGUSR1;
      socks_flushrb();
      assert(ringbuf_curroff == oldoffset);
      socks_flushrb();
      guarded = 0;
      assert(dont_add_to_rb == 0);
   }
   else if (strcmp(argv[2], "ring-emitted") == 0) {
      slog(LOG_INFO, "ordinary saved");
      signalslog(LOG_INFO, msg);
      socks_flushrb();
   }
   else if (strcmp(argv[2], "stack") == 0)
      slogstack();
   else if (strcmp(argv[2], "stack-signal") == 0) {
      guarded = sockscf.logformat == LOGFORMAT_JSON;
      sockscf.state.insignal = SIGUSR1;
      slogstack();
      guarded = 0;
   }
   else {
      if (strcmp(argv[2], "long") == 0) { parts[0] = huge; parts[1] = NULL; }
      if (strcmp(argv[2], "routing") == 0) {
         sockscf.errlog.type = LOGTYPE_FILE;
         sockscf.errlog.filenoc = 1; sockscf.errlog.filenov = &err;
      }
      if (strcmp(argv[2], "fallback") == 0) sockscf.log.type = 0;
      if (strcmp(argv[2], "syslog") == 0) {
         sockscf.log.type = LOGTYPE_SYSLOG; sockscf.log.facility = LOG_LOCAL0;
         sockscf.errlog.type = LOGTYPE_SYSLOG; sockscf.errlog.facility = LOG_LOCAL1;
         sockscf.state.inited = 1;
      }
      if (strcmp(argv[2], "eintr-short") == 0) {
         interrupt_write = 2; short_write = 1;
      }
      if (strcmp(argv[2], "write-fail") == 0) fail_write = 1;
      if (strcmp(argv[2], "empty") == 0) parts[0] = NULL;
      signal(SIGUSR1, handle_signal);
      raise(SIGUSR1);
      if (strcmp(argv[2], "syslog") == 0) {
         guarded = sockscf.logformat == LOGFORMAT_JSON;
         sockscf.state.insignal = SIGUSR1;
         signalslog(LOG_CRIT, parts);
         guarded = 0;
      }
      signalslog(LOG_INFO, NULL);
      if (fail_write) assert(writes == 1);
   }
   assert(errno == EDOM);
   return 0;
}
