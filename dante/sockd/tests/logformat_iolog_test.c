/* Reuse the real logger and its deterministic OS boundary wrappers. */
#define main logger_driver_main
#include "logformat_logger_test.c"
#undef main

static void host(sockshost_t *value, int type, const char *address, int port)
{
   memset(value, 0, sizeof(*value));
   value->atype = type;
   value->port = htons(port);
   if (type == SOCKS_ADDR_IPV4)
      assert(inet_pton(AF_INET, address, &value->addr.ipv4) == 1);
   else if (type == SOCKS_ADDR_IPV6) {
      assert(inet_pton(AF_INET6, address, &value->addr.ipv6.ip) == 1);
      value->addr.ipv6.scopeid = 3;
   }
   else
      snprintf(value->addr.domain, sizeof(value->addr.domain), "%s", address);
}

int main(int argc, char **argv)
{
   rule_t rule;
   connectionstate_t state;
   iologaddr_t src, dst, sourceproxy, destproxy;
   const iologaddr_t *srcp = &src, *dstp = &dst;
   const iologaddr_t *spp = &sourceproxy, *dpp = &destproxy;
   const char payload[] = "quote\"\\\n\0\377\303\251";
   char huge[100001];
   const char *data = "detail \"quoted\"\nsecond line";
   size_t datalen = strlen(data);
   int out = STDOUT_FILENO, op, option, first = OPERATION_ACCEPT, last = OPERATION_IO;
   assert(argc == 3);
   __progname = "danted";
   sockscf.state.pid = 1234;
   sockscf.state.type = PROC_IO;
   sockscf.logformat = strcmp(argv[1], "json") == 0 ? LOGFORMAT_JSON : LOGFORMAT_RAW;
   sockscf.log.type = LOGTYPE_FILE;
   sockscf.log.filenoc = 1;
   sockscf.log.filenov = &out;
   memset(&rule, 0, sizeof(rule));
   memset(&state, 0, sizeof(state));
   memset(&src, 0, sizeof(src));
   memset(&dst, 0, sizeof(dst));
   memset(&sourceproxy, 0, sizeof(sourceproxy));
   memset(&destproxy, 0, sizeof(destproxy));
   rule.number = 7;
   rule.type = object_srule;
   rule.verdict = VERDICT_PASS;
   state.protocol = SOCKS_TCP;
   state.command = SOCKS_CONNECT;
   state.tcpinfo = "first\nsecond \"info\"\\";
   src.local_isset = src.peer_isset = src.auth_isset = 1;
   host(&src.local, SOCKS_ADDR_IPV4, "127.0.0.1", 1080);
   host(&src.peer, SOCKS_ADDR_IPV4, "198.51.100.1", 12345);
   src.auth.method = AUTHMETHOD_UNAME;
   snprintf((char *)src.auth.mdata.uname.name, sizeof(src.auth.mdata.uname.name), "alice\"\\");
   snprintf((char *)src.auth.mdata.uname.password, sizeof(src.auth.mdata.uname.password), "never-log-this-password");
   dst.local_isset = dst.peer_isset = dst.auth_isset = 1;
   host(&dst.local, SOCKS_ADDR_IPV6, "fe80::1", 0);
   host(&dst.peer, SOCKS_ADDR_DOMAIN, "target.example", 443);
   dst.auth.method = AUTHMETHOD_NONE;
   sourceproxy.peer_isset = 1;
   host(&sourceproxy.peer, SOCKS_ADDR_IPV4, "192.0.2.1", 1081);
   destproxy.local_isset = 1;
   host(&destproxy.local, SOCKS_ADDR_IPV4, "192.0.2.2", 1082);
#if HAVE_SOCKS_HOSTID
   src.hostidc = 1;
   assert(inet_pton(AF_INET, "203.0.113.1", &src.hostidv[0]) == 1);
#endif
   if (strcmp(argv[2], "missing") == 0) {
      memset(&src, 0, sizeof(src));
      dstp = spp = dpp = NULL;
      rule.number = 0;
      state.command = SOCKS_UNKNOWN;
      data = NULL;
      datalen = 0;
      first = last = OPERATION_ACCEPT;
   }
   else if (strcmp(argv[2], "error") == 0) {
      data = NULL;
      datalen = 0;
      first = last = OPERATION_ERROR;
   }
   else if (strcmp(argv[2], "payload") == 0 || strcmp(argv[2], "ioonly") == 0) {
      data = payload;
      datalen = sizeof(payload) - 1;
      first = last = OPERATION_IO;
   }
   else if (strcmp(argv[2], "empty-payload") == 0) {
      data = "";
      datalen = 0;
      first = last = OPERATION_IO;
   }
   else if (strcmp(argv[2], "long") == 0 || strcmp(argv[2], "alloc") == 0) {
      memset(huge, 'x', sizeof(huge) - 1);
      huge[sizeof(huge) - 1] = '\0';
      data = huge;
      datalen = sizeof(huge) - 1;
      first = last = OPERATION_IO;
      if (strcmp(argv[2], "alloc") == 0)
         failalloc = 1;
   }
   else if (strcmp(argv[2], "scopezero") == 0)
      dst.local.addr.ipv6.scopeid = 0;
   else if (strcmp(argv[2], "udp") == 0) {
      state.protocol = SOCKS_UDP;
      state.command = SOCKS_UDPASSOCIATE;
   }
   else if (strcmp(argv[2], "bind") == 0)
      state.command = SOCKS_BIND;

   for (option = 0; option < (strcmp(argv[2], "matrix") == 0 ? 7 : 1); ++option) {
      memset(&rule.log, 0, sizeof(rule.log));
      if (strcmp(argv[2], "matrix") == 0) {
         rule.log.connect = option == 1;
         rule.log.disconnect = option == 2;
         rule.log.error = option == 3;
         rule.log.data = option == 4;
         rule.log.iooperation = option == 5;
         rule.log.tcpinfo = option == 6;
      }
      else {
         rule.log.connect = rule.log.disconnect = rule.log.error = 1;
         rule.log.data = strcmp(argv[2], "ioonly") != 0;
         rule.log.iooperation = 1;
         rule.log.tcpinfo = 1;
      }
      for (op = first; op <= last; ++op) {
         errno = ECONNREFUSED;
         iolog(&rule, &state, op, srcp, dstp, spp, dpp, data, datalen);
         if (sockscf.logformat == LOGFORMAT_JSON)
            assert(errno == ECONNREFUSED);
      }
   }
   return 0;
}
