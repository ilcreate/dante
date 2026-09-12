/* Load the client configuration through the real client library. */
#include "common.h"

int
main(void)
{
   clientinit();
   puts("client config parsed");
   return 0;
}
