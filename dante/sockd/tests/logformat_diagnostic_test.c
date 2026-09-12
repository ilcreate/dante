/* Exercise public diagnostic entry points and a real scanner input failure. */
#include "common.h"
#include <assert.h>

extern FILE *socks_yyin;
extern int socks_yylex(void);
extern void socks_yyrestart(FILE *);

int
main(int argc, char **argv)
{
   const char *message = "diagnostic quote\" slash\\ line\nnext";
   int errfd = STDERR_FILENO;

   assert(argc == 3);
   __progname = "danted";
   sockscf.state.pid = getpid();
   sockscf.state.type = PROC_IO;
   sockscf.logformat = strcmp(argv[1], "json") == 0
                    ? LOGFORMAT_JSON : LOGFORMAT_RAW;
   if (strstr(argv[2], "fallback") == NULL) {
      sockscf.log.type = LOGTYPE_FILE;
      sockscf.log.filenoc = 1;
      sockscf.log.filenov = &errfd;
   }
   errno = EDOM;
   if (strcmp(argv[2], "warnings") == 0) {
      swarn("%s", message);
      assert(errno == EDOM);
      swarnx("%s", message);
      assert(errno == EDOM);
      return 0;
   }
   if (strncmp(argv[2], "serrx", 5) == 0)
      serrx("%s", message);
   else if (strncmp(argv[2], "serr", 4) == 0)
      serr("%s", message);
   else if (strncmp(argv[2], "scanner", 7) == 0) {
      /* fread on a write-only stream enters flex's real YY_FATAL_ERROR. */
      int pipefd[2];
      assert(pipe(pipefd) == 0);
      close(pipefd[0]);
      socks_yyin = fdopen(pipefd[1], "w");
      assert(socks_yyin != NULL);
      socks_yyrestart(socks_yyin);
      (void)socks_yylex();
   }
   assert(!"fatal diagnostic returned");
   return 99;
}
