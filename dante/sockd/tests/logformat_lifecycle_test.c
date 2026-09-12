/* Exercise the real server initialization, parser and config copy routines. */
#define main sockd_program_main
#include "../sockd.c"
#undef main

#include <assert.h>

static logformat_t expected_at_error;

static void
check_error_format(void)
{
   _exit(sockscf.logformat == expected_at_error ? 0 : 3);
}

static void
check_parse_error(const char *text, logformat_t expected)
{
   char filename[] = "/tmp/dante-logformat-invalid.XXXXXX";
   int fd, status;
   pid_t child;

   fd = mkstemp(filename);
   assert(fd != -1);
   assert(write(fd, text, strlen(text)) == (ssize_t)strlen(text));
   assert(close(fd) == 0);
   child = fork();
   assert(child != -1);
   if (child == 0) {
      /* Keep the error exit's mother cleanup local to this test process. */
      sockscf.state.pid = *sockscf.state.motherpidv = getpid();
      expected_at_error = expected;
      assert(atexit(check_error_format) == 0);
      parseconfig(filename);
      _exit(2); /* A syntax error must exit through the error handler. */
   }
   assert(waitpid(child, &status, 0) == child);
   assert(unlink(filename) == 0);
   assert(WIFEXITED(status) && WEXITSTATUS(status) == 0);
}

int
main(int argc, char *argv[])
{
   struct config copy;
   void *memory;
   size_t size;
   const char *filename;
   FILE *fp;
   pid_t child;
   int status;

   assert(sockscf.logformat == LOGFORMAT_RAW);
   serverinit(argc, argv);
   assert(sockscf.option.verifyonly);
   assert(sockscf.logformat == LOGFORMAT_JSON);
   filename = sockscf.option.configfile;

   check_parse_error("invalid-setting: yes\n", LOGFORMAT_JSON);
   check_parse_error("logformat: xml\n", LOGFORMAT_JSON);
   check_parse_error("logformat: raw\ninvalid-setting: yes\n", LOGFORMAT_RAW);
   sockscf.logformat = LOGFORMAT_RAW;
   check_parse_error("logformat: json\ninvalid-setting: yes\n", LOGFORMAT_JSON);
   sockscf.logformat = LOGFORMAT_JSON;

   /* New children inherit the selected format through fork(). */
   child = fork();
   assert(child != -1);
   if (child == 0)
      _exit(sockscf.logformat == LOGFORMAT_JSON ? 0 : 1);
   assert(waitpid(child, &status, 0) == child);
   assert(WIFEXITED(status) && WEXITSTATUS(status) == 0);

   /* The shared-memory path copies scalars before relocating pointers. */
   copy = sockscf;
   size = pointer_size(&sockscf);
   memory = malloc(size);
   assert(memory != NULL);
   assert(pointer_copy(&sockscf, 0, &copy, memory, size) == 0);
   assert(copy.logformat == LOGFORMAT_JSON);
   assert(compareconfigs(&sockscf, &copy) != 0);
   copy.logformat = LOGFORMAT_RAW;
   assert(compareconfigs(&sockscf, &copy) == 0);
   free(memory);

   resetconfig(&sockscf, 0);
   assert(sockscf.logformat == LOGFORMAT_JSON);

   /* A failed open must not install the default. */
   assert(parseconfig("/nonexistent/dante-logformat-test.conf") == -1);
   assert(sockscf.logformat == LOGFORMAT_JSON);

   /* Re-reading the same directive is not a duplicate across parses. */
   assert(parseconfig(filename) == 0);
   assert(sockscf.logformat == LOGFORMAT_JSON);
   resetconfig(&sockscf, 0);
   assert(sockscf.logformat == LOGFORMAT_JSON);

   /* A successful parse without the directive installs raw. */
   fp = fopen(filename, "w");
   assert(fp != NULL);
   assert(fputs("# logformat deliberately omitted\n", fp) >= 0);
   assert(fclose(fp) == 0);
   assert(parseconfig(filename) == 0);
   assert(sockscf.logformat == LOGFORMAT_RAW);

   resetconfig(&sockscf, 1);
   puts("logformat lifecycle and config copy tests passed");
   return 0;
}
