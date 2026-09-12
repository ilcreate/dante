/* Real producers and logger, with deterministic clock and TCP diagnostics. */
#define main logger_driver_main
#include "logformat_logger_test.c"
#undef main

static struct timeval *session_clock(struct timeval *tv)
{
   tv->tv_sec = 110;
   tv->tv_usec = 750000;
   return tv;
}
static time_t session_seconds(time_t *seconds)
{
   if (seconds != NULL)
      *seconds = 110;
   return 110;
}
static char session_tcpinfo_text[3001] = "native first line\nsecond \"quoted\" line";
static char *session_tcpinfo(const size_t count, int fds[], char *buf, size_t len)
{
   (void)count;
   (void)fds;
   assert(buf == NULL && len == 0);
   return session_tcpinfo_text;
}
#define get_tcpinfo session_tcpinfo
#define rcsid io_rcsid
#define gettimeofday_monotonic session_clock
#define time_monotonic session_seconds
#include "../sockd_io.c"
#undef time_monotonic
#undef gettimeofday_monotonic
#undef rcsid
#undef get_tcpinfo

static void address(struct sockaddr_storage *addr, const char *ip, int port)
{
   struct sockaddr_in *in = (struct sockaddr_in *)addr;
   memset(addr, 0, sizeof(*addr));
   in->sin_family = AF_INET;
   SET_SOCKADDRLEN(addr, sizeof(*in));
   in->sin_port = htons(port);
   assert(inet_pton(AF_INET, ip, &in->sin_addr) == 1);
}
static void direction(sockd_io_direction_t *dir, const char *ip, int port,
                      uint64_t bytes, int udp)
{
   dir->s = socket(AF_INET, udp ? SOCK_DGRAM : SOCK_STREAM, 0);
   assert(dir->s >= 0);
   address(&dir->laddr, "127.0.0.1", 1080);
   address(&dir->raddr, ip, port);
   sockaddr2sockshost(&dir->raddr, &dir->host);
   dir->auth.method = AUTHMETHOD_NONE;
   dir->state.isconnected = 1;
   dir->read.bytes = bytes;
   dir->written.bytes = bytes + 1;
   dir->read.packets = bytes + 2;
   dir->written.packets = bytes + 3;
   socks_allocbuffer(dir->s, udp ? SOCK_DGRAM : SOCK_STREAM);
}
int main(int argc, char **argv)
{
   sockd_io_t io, before;
   udptarget_t targets[2], targetbefore[2];
   int out = STDOUT_FILENO, status = IO_CLOSE, badfd, udp, snapshot, n;
   int resetpeer = -1;
   assert(argc == 3);
   memset(&io, 0, sizeof(io));
   memset(targets, 0, sizeof(targets));
   __progname = "danted";
   sockscf.state.pid = 1234;
   sockscf.state.type = PROC_IO;
   sockscf.logformat = strcmp(argv[1], "json") == 0 ? LOGFORMAT_JSON : LOGFORMAT_RAW;
   sockscf.log.type = LOGTYPE_FILE;
   sockscf.log.filenoc = 1;
   sockscf.log.filenov = &out;
   udp = strstr(argv[2], "udp") != NULL;
   snapshot = strstr(argv[2], "snapshot") != NULL;
   io.allocated = 1;
   io.control.s = -1;
   io.state.command = udp ? SOCKS_UDPASSOCIATE : SOCKS_CONNECT;
   io.state.protocol = udp ? SOCKS_UDP : SOCKS_TCP;
   io.state.proxychain.proxyprotocol = PROXY_DIRECT;
   io.state.time.accepted.tv_sec = 100;
   io.state.time.accepted.tv_usec = 250000;
   io.state.time.requestend = io.state.time.negotiateend = io.state.time.accepted;
   io.lastio.tv_sec = 108;
   io.lastio.tv_usec = 500000;
   io.srule.type = object_srule;
   io.srule.number = 7;
   io.srule.verdict = VERDICT_PASS;
   io.srule.log.disconnect = 1;
   io.srule.log.tcpinfo = strstr(argv[2], "tcpinfo") != NULL;
   io.state.tcpinfo = "stored listener diagnostic";
   if (strstr(argv[2], "longinfo") != NULL)
      memset(session_tcpinfo_text, 'x', sizeof(session_tcpinfo_text) - 1);
   io.srule.timeout.tcpio = io.srule.timeout.udpio = 1;
   io.srule.timeout.connect = 1;
   io.crule.type = object_crule;
   io.crule.number = 3;
   io.crule.verdict = VERDICT_PASS;
   io.cauth.method = AUTHMETHOD_NONE;
   direction(&io.src, "198.51.100.1", 12345, 11, udp);
   direction(&io.dst, "203.0.113.1", 443, 21, udp);
   if (strstr(argv[2], "bind") != NULL) {
      io.state.command = SOCKS_BINDREPLY;
      io.cmd.bind.rule = io.srule;
      io.cmd.bind.host = io.dst.host;
      if (strstr(argv[2], "listeneroff") != NULL)
         io.cmd.bind.rule.log.tcpinfo = 0;
      if (strstr(argv[2], "bindquiet") != NULL)
         io.cmd.bind.rule.log.disconnect = 0;
   }
   if (udp) {
      close(io.dst.s);
      socks_freebuffer(io.dst.s);
      io.dst.s = -1;
      direction(&io.control, "198.51.100.1", 12346, 31, 0);
      io.crule.log.disconnect = 1;
      if (strstr(argv[2], "empty") == NULL) {
         io.dst.dstc = 2;
         io.dst.dstv = targets;
         for (n = 0; n < 2; ++n) {
            targets[n].s = socket(AF_INET, SOCK_DGRAM, 0);
            socks_allocbuffer(targets[n].s, SOCK_DGRAM);
            address(&targets[n].laddr, "127.0.0.1", 2000 + n);
            address(&targets[n].raddr, "203.0.113.2", 5000 + n);
            sockaddr2sockshost(&targets[n].raddr, &targets[n].raddrhost);
            targets[n].isconnected = 1;
            targets[n].client_read.bytes = 100 + n;
            targets[n].client_written.bytes = 200 + n;
            targets[n].target_read.bytes = 300 + n;
            targets[n].target_written.bytes = 400 + n;
            targets[n].client_read.packets = 10 + n;
            targets[n].client_written.packets = 20 + n;
            targets[n].target_read.packets = 30 + n;
            targets[n].target_written.packets = 40 + n;
            targets[n].firstio.tv_sec = 102 + n;
            targets[n].lastio.tv_sec = 109;
         }
         io.dst.s = targets[1].s;
      }
      else
         memset(&io.dst.read, 0, sizeof(io.dst.read)),
         memset(&io.dst.written, 0, sizeof(io.dst.written));
   }
   badfd = TARGETIO(&io)->s;
   if (strstr(argv[2], "control") != NULL) badfd = io.control.s;
   if (strstr(argv[2], "client") != NULL) badfd = CLIENTIO(&io)->s;
   if (strstr(argv[2], "session") != NULL) badfd = -1;
   if (strstr(argv[2], "block") != NULL) status = IO_BLOCK;
   if (strstr(argv[2], "ioerror") != NULL) status = IO_IOERROR;
   else if (strstr(argv[2], "error") != NULL) status = IO_ERROR;
   if (strstr(argv[2], "timeout") != NULL) status = IO_TIMEOUT;
   if (strstr(argv[2], "connecttimeout") != NULL) io.dst.state.isconnected = 0;
   if (strstr(argv[2], "fintimeout") != NULL) {
      io.dst.state.fin_received = 1;
      io.srule.timeout.tcp_fin_wait = 1;
      io.srule.timeout.tcpio = 0;
   }
   if (strstr(argv[2], "admin") != NULL) status = IO_ADMINTERMINATION;
   if (strstr(argv[2], "quiet") != NULL && strstr(argv[2], "bindquiet") == NULL)
      io.srule.log.disconnect = 0;
   if (strstr(argv[2], "erroronly") != NULL) {
      io.srule.log.disconnect = 0;
      io.srule.log.error = 1;
   }
   if (strstr(argv[2], "max") != NULL)
      io.src.read.bytes = UINT64_MAX;
   if (strstr(argv[2], "unknowntime") != NULL) {
      memset(&io.state.time.accepted, 0, sizeof(io.state.time.accepted));
      memset(&io.lastio, 0, sizeof(io.lastio));
      for (n = 0; n < 2; ++n) {
         memset(&targets[n].firstio, 0, sizeof(targets[n].firstio));
         memset(&targets[n].lastio, 0, sizeof(targets[n].lastio));
      }
   }
   if (strstr(argv[2], "zerotime") != NULL) {
      session_clock(&io.state.time.accepted);
      session_clock(&io.lastio);
      for (n = 0; n < 2; ++n) {
         session_clock(&targets[n].firstio);
         session_clock(&targets[n].lastio);
      }
   }
   if (strstr(argv[2], "reset") != NULL) {
      resetpeer = dup(badfd == io.dst.s ? io.src.s : io.dst.s);
      assert(resetpeer >= 0);
      status = IO_IOERROR;
   }
   iov[0] = io;
   errno = strstr(argv[2], "zeroerrno") == NULL ? ECONNREFUSED : 0;
   if (resetpeer >= 0) errno = ECONNRESET;
   if (snapshot) {
      before = io;
      memcpy(targetbefore, targets, sizeof(targets));
      siginfo(-SIGUSR1, NULL, NULL);
      assert(memcmp(&before, &iov[0], sizeof(io)) == 0);
      assert(memcmp(targetbefore, targets, sizeof(targets)) == 0);
   }
   else {
      io_delete(-1, &iov[0], badfd, status);
      assert(!iov[0].allocated);
      assert(fcntl(io.src.s, F_GETFD) == -1 && errno == EBADF);
      if (resetpeer >= 0) {
         struct linger linger;
         socklen_t len = sizeof(linger);
         assert(getsockopt(resetpeer, SOL_SOCKET, SO_LINGER, &linger, &len) == 0);
         assert(linger.l_onoff && linger.l_linger == 0);
         close(resetpeer);
      }
   }
   return 0;
}
