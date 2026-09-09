#include "common.h"

#include <sys/stat.h>
#include <sys/un.h>
#include <sys/wait.h>

#undef close
#undef snprintf
#undef socket

static int failures;

#define TEST_CHECK(expression)                                                 \
do {                                                                           \
   if (!(expression)) {                                                        \
      fprintf(stderr, "%s:%d: check failed: %s\n",                           \
              __FILE__, __LINE__, #expression);                               \
      ++failures;                                                              \
   }                                                                           \
} while (0)

static void
test_counter_updates(void)
{
   sockd_stats_t stats;

   sockd_stats_init(&stats, (time_t)100);
   TEST_CHECK(stats.schema_version == SOCKD_STATS_SCHEMA_VERSION);
   TEST_CHECK(stats.started_at == (time_t)100);
   TEST_CHECK(stats.sessions_active == 0);

   sockd_stats_add(&stats, SOCKD_STAT_CLIENT_ACCEPTED, 2);
   sockd_stats_add(&stats, SOCKD_STAT_CLIENT_DROPPED, 1);
   sockd_stats_add(&stats, SOCKD_STAT_NEGOTIATION_FAILED, 3);
   sockd_stats_add(&stats, SOCKD_STAT_REQUEST_FAILED, 4);
   sockd_stats_add(&stats, SOCKD_STAT_SESSION_ESTABLISHED, 2);
   sockd_stats_add(&stats, SOCKD_STAT_SESSION_CLOSED, 1);
   sockd_stats_add(&stats, SOCKD_STAT_SESSION_ERROR, 1);
   sockd_stats_add(&stats, SOCKD_STAT_CLIENT_READ_BYTES, 10);
   sockd_stats_add(&stats, SOCKD_STAT_CLIENT_WRITTEN_BYTES, 20);
   sockd_stats_add(&stats, SOCKD_STAT_TARGET_READ_BYTES, 30);
   sockd_stats_add(&stats, SOCKD_STAT_TARGET_WRITTEN_BYTES, 40);

   TEST_CHECK(stats.client_connections_accepted == 2);
   TEST_CHECK(stats.client_connections_dropped == 1);
   TEST_CHECK(stats.negotiation_failures == 3);
   TEST_CHECK(stats.request_failures == 4);
   TEST_CHECK(stats.sessions_established == 2);
   TEST_CHECK(stats.sessions_active == 1);
   TEST_CHECK(stats.sessions_closed == 1);
   TEST_CHECK(stats.session_errors == 1);
   TEST_CHECK(stats.client_read_bytes == 10);
   TEST_CHECK(stats.client_written_bytes == 20);
   TEST_CHECK(stats.target_read_bytes == 30);
   TEST_CHECK(stats.target_written_bytes == 40);

   sockd_stats_add(&stats, SOCKD_STAT_SESSION_CLOSED, 50);
   TEST_CHECK(stats.sessions_active == 0);
   TEST_CHECK(stats.sessions_closed == 51);

   stats.client_connections_accepted = UINT64_MAX - 1;
   sockd_stats_add(&stats, SOCKD_STAT_CLIENT_ACCEPTED, 5);
   TEST_CHECK(stats.client_connections_accepted == UINT64_MAX);
}

static void
test_detailed_negotiation_updates(void)
{
   sockd_stats_t stats;

   sockd_stats_init(&stats, (time_t)100);
   TEST_CHECK(stats.schema_revision == SOCKD_STATS_SCHEMA_REVISION);

   sockd_stats_add_negotiation(&stats, SOCKD_STATS_NEGOTIATION_SUCCESS, 2);
   sockd_stats_add_negotiation(&stats, SOCKD_STATS_NEGOTIATION_EOF, 3);
   sockd_stats_add_negotiation(&stats, SOCKD_STATS_NEGOTIATION_ERROR, 4);
   sockd_stats_add_negotiation(&stats, SOCKD_STATS_NEGOTIATION_TIMEOUT, 5);
   sockd_stats_add_negotiation(&stats,
                               (sockd_stats_negotiation_t)UINT_MAX, 6);
   sockd_stats_add_negotiation(&stats, SOCKD_STATS_NEGOTIATION_UNKNOWN, 1);
   sockd_stats_add_negotiation(&stats, SOCKD_STATS_NEGOTIATION_COUNT, 1);

   TEST_CHECK(stats.negotiation_outcome[SOCKD_STATS_NEGOTIATION_SUCCESS] == 2);
   TEST_CHECK(stats.negotiation_outcome[SOCKD_STATS_NEGOTIATION_EOF] == 3);
   TEST_CHECK(stats.negotiation_outcome[SOCKD_STATS_NEGOTIATION_ERROR] == 4);
   TEST_CHECK(stats.negotiation_outcome[SOCKD_STATS_NEGOTIATION_TIMEOUT] == 5);
   TEST_CHECK(stats.negotiation_outcome[SOCKD_STATS_NEGOTIATION_UNKNOWN] == 8);
   TEST_CHECK(stats.negotiation_failures == 20);

   stats.negotiation_outcome[SOCKD_STATS_NEGOTIATION_SUCCESS] = UINT64_MAX - 1;
   sockd_stats_add_negotiation(&stats, SOCKD_STATS_NEGOTIATION_SUCCESS, 5);
   TEST_CHECK(stats.negotiation_outcome[SOCKD_STATS_NEGOTIATION_SUCCESS]
         == UINT64_MAX);
}

static void
test_detailed_request_updates(void)
{
   sockd_stats_t stats;

   sockd_stats_init(&stats, (time_t)100);
   sockd_stats_add_request(&stats, SOCKS_CONNECT, IO_NOERROR, 2);
   sockd_stats_add_request(&stats, SOCKS_BIND, IO_BLOCK, 3);
   sockd_stats_add_request(&stats, SOCKS_UDPASSOCIATE, IO_TIMEOUT, 4);
   sockd_stats_add_request(&stats, SOCKS_UNKNOWN, IO_IOERROR, 5);
   sockd_stats_add_request(&stats, INT_MAX, (iostatus_t)INT_MAX, 6);

   TEST_CHECK(stats.request_outcome[SOCKD_STATS_COMMAND_CONNECT]
                                    [SOCKD_STATS_RESULT_SUCCESS] == 2);
   TEST_CHECK(stats.request_outcome[SOCKD_STATS_COMMAND_BIND]
                                    [SOCKD_STATS_RESULT_BLOCKED] == 3);
   TEST_CHECK(stats.request_outcome[SOCKD_STATS_COMMAND_UDP_ASSOCIATE]
                                    [SOCKD_STATS_RESULT_TIMEOUT] == 4);
   TEST_CHECK(stats.request_outcome[SOCKD_STATS_COMMAND_UNKNOWN]
                                    [SOCKD_STATS_RESULT_NETWORK_ERROR] == 5);
   TEST_CHECK(stats.request_outcome[SOCKD_STATS_COMMAND_UNKNOWN]
                                    [SOCKD_STATS_RESULT_OTHER] == 6);
   TEST_CHECK(stats.request_failures == 18);
}

static void
test_request_status_taxonomy(void)
{
   static const struct {
      iostatus_t result;
      sockd_stats_result_t expected;
   } cases[] = {
      { IO_NOERROR,          SOCKD_STATS_RESULT_SUCCESS },
      { IO_TMPERROR,         SOCKD_STATS_RESULT_OTHER },
      { IO_IOERROR,          SOCKD_STATS_RESULT_NETWORK_ERROR },
      { IO_ERROR,            SOCKD_STATS_RESULT_INTERNAL_ERROR },
      { IO_EAGAIN,           SOCKD_STATS_RESULT_OTHER },
      { IO_TIMEOUT,          SOCKD_STATS_RESULT_TIMEOUT },
      { IO_CLOSE,            SOCKD_STATS_RESULT_CLOSED },
      { IO_BLOCK,            SOCKD_STATS_RESULT_BLOCKED },
      { IO_TMPBLOCK,         SOCKD_STATS_RESULT_BLOCKED },
      { IO_ADMINTERMINATION, SOCKD_STATS_RESULT_ADMIN },
      { (iostatus_t)INT_MAX, SOCKD_STATS_RESULT_OTHER }
   };
   sockd_stats_t stats;
   size_t i;

   sockd_stats_init(&stats, (time_t)100);
   for (i = 0; i < ELEMENTS(cases); ++i) {
      sockd_stats_add_request(&stats, SOCKS_CONNECT, cases[i].result, 1);
      TEST_CHECK(stats.request_outcome[SOCKD_STATS_COMMAND_CONNECT]
                                       [cases[i].expected] != 0);
   }

   TEST_CHECK(stats.request_outcome[SOCKD_STATS_COMMAND_CONNECT]
                                    [SOCKD_STATS_RESULT_SUCCESS] == 1);
   TEST_CHECK(stats.request_outcome[SOCKD_STATS_COMMAND_CONNECT]
                                    [SOCKD_STATS_RESULT_BLOCKED] == 2);
   TEST_CHECK(stats.request_outcome[SOCKD_STATS_COMMAND_CONNECT]
                                    [SOCKD_STATS_RESULT_TIMEOUT] == 1);
   TEST_CHECK(stats.request_outcome[SOCKD_STATS_COMMAND_CONNECT]
                                    [SOCKD_STATS_RESULT_NETWORK_ERROR] == 1);
   TEST_CHECK(stats.request_outcome[SOCKD_STATS_COMMAND_CONNECT]
                                    [SOCKD_STATS_RESULT_INTERNAL_ERROR] == 1);
   TEST_CHECK(stats.request_outcome[SOCKD_STATS_COMMAND_CONNECT]
                                    [SOCKD_STATS_RESULT_CLOSED] == 1);
   TEST_CHECK(stats.request_outcome[SOCKD_STATS_COMMAND_CONNECT]
                                    [SOCKD_STATS_RESULT_ADMIN] == 1);
   TEST_CHECK(stats.request_outcome[SOCKD_STATS_COMMAND_CONNECT]
                                    [SOCKD_STATS_RESULT_OTHER] == 3);
   TEST_CHECK(stats.request_failures == ELEMENTS(cases) - 1);
}

static void
test_detailed_session_updates(void)
{
   sockd_stats_t stats;

   sockd_stats_init(&stats, (time_t)100);
   sockd_stats_add_session_started(&stats, SOCKS_TCP, 2);
   sockd_stats_add_session_started(&stats, SOCKS_UDP, 3);
   sockd_stats_add_session_started(&stats, INT_MAX, 4);

   TEST_CHECK(stats.sessions[SOCKD_STATS_PROTOCOL_TCP].started == 2);
   TEST_CHECK(stats.sessions[SOCKD_STATS_PROTOCOL_TCP].active == 2);
   TEST_CHECK(stats.sessions[SOCKD_STATS_PROTOCOL_UDP].started == 3);
   TEST_CHECK(stats.sessions[SOCKD_STATS_PROTOCOL_UDP].active == 3);
   TEST_CHECK(stats.sessions[SOCKD_STATS_PROTOCOL_UNKNOWN].started == 4);
   TEST_CHECK(stats.sessions_established == 9);
   TEST_CHECK(stats.sessions_active == 9);

   sockd_stats_add_session_closed(&stats, SOCKS_TCP, IO_CLOSE, 1);
   sockd_stats_add_session_closed(&stats, SOCKS_UDP, IO_TIMEOUT, 2);
   sockd_stats_add_session_closed(&stats, INT_MAX, IO_ERROR, 10);

   TEST_CHECK(stats.sessions[SOCKD_STATS_PROTOCOL_TCP].active == 1);
   TEST_CHECK(stats.sessions[SOCKD_STATS_PROTOCOL_TCP].closed == 1);
   TEST_CHECK(stats.sessions[SOCKD_STATS_PROTOCOL_UDP].active == 1);
   TEST_CHECK(stats.sessions[SOCKD_STATS_PROTOCOL_UDP].closed == 2);
   TEST_CHECK(stats.sessions[SOCKD_STATS_PROTOCOL_UDP].errors == 2);
   TEST_CHECK(stats.sessions[SOCKD_STATS_PROTOCOL_UNKNOWN].active == 0);
   TEST_CHECK(stats.sessions[SOCKD_STATS_PROTOCOL_UNKNOWN].closed == 10);
   TEST_CHECK(stats.sessions[SOCKD_STATS_PROTOCOL_UNKNOWN].errors == 10);
   TEST_CHECK(stats.session_close_reason[SOCKD_STATS_CLOSE_PEER] == 1);
   TEST_CHECK(stats.session_close_reason[SOCKD_STATS_CLOSE_TIMEOUT] == 2);
   TEST_CHECK(stats.session_close_reason[SOCKD_STATS_CLOSE_INTERNAL_ERROR]
         == 10);
   TEST_CHECK(stats.sessions_closed == 13);
   TEST_CHECK(stats.sessions_active == 0);
   TEST_CHECK(stats.session_errors == 12);

   sockd_stats_add_session_closed(&stats, SOCKS_TCP, IO_BLOCK, 1);
   TEST_CHECK(stats.session_close_reason[SOCKD_STATS_CLOSE_BLOCKED] == 1);
   TEST_CHECK(stats.session_errors == 12);
}

static void
test_session_close_taxonomy(void)
{
   static const iostatus_t cases[] = {
      IO_NOERROR,
      IO_TMPERROR,
      IO_IOERROR,
      IO_ERROR,
      IO_EAGAIN,
      IO_TIMEOUT,
      IO_CLOSE,
      IO_BLOCK,
      IO_TMPBLOCK,
      IO_ADMINTERMINATION,
      (iostatus_t)INT_MAX
   };
   sockd_stats_t stats;
   size_t i;

   sockd_stats_init(&stats, (time_t)100);
   sockd_stats_add_session_started(&stats, SOCKS_TCP, 1);
   for (i = 0; i < ELEMENTS(cases); ++i)
      sockd_stats_add_session_closed(&stats, SOCKS_TCP, cases[i], 1);

   TEST_CHECK(stats.session_close_reason[SOCKD_STATS_CLOSE_NORMAL] == 1);
   TEST_CHECK(stats.session_close_reason[SOCKD_STATS_CLOSE_BLOCKED] == 2);
   TEST_CHECK(stats.session_close_reason[SOCKD_STATS_CLOSE_TIMEOUT] == 1);
   TEST_CHECK(stats.session_close_reason[SOCKD_STATS_CLOSE_NETWORK_ERROR]
         == 1);
   TEST_CHECK(stats.session_close_reason[SOCKD_STATS_CLOSE_INTERNAL_ERROR]
         == 1);
   TEST_CHECK(stats.session_close_reason[SOCKD_STATS_CLOSE_PEER] == 1);
   TEST_CHECK(stats.session_close_reason[SOCKD_STATS_CLOSE_ADMIN] == 1);
   TEST_CHECK(stats.session_close_reason[SOCKD_STATS_CLOSE_OTHER] == 3);
   TEST_CHECK(stats.sessions[SOCKD_STATS_PROTOCOL_TCP].active == 0);
   TEST_CHECK(stats.sessions[SOCKD_STATS_PROTOCOL_TCP].closed
         == ELEMENTS(cases));
   TEST_CHECK(stats.sessions[SOCKD_STATS_PROTOCOL_TCP].errors == 3);
   TEST_CHECK(stats.session_errors == 3);
}

static void
test_target_connect_updates(void)
{
   sockd_stats_t stats;

   sockd_stats_init(&stats, (time_t)100);
   sockd_stats_add_target_connect_attempt(&stats, 7);
   sockd_stats_add_target_connect_result(&stats, 0, 1);
   sockd_stats_add_target_connect_result(&stats, ECONNREFUSED, 2);
   sockd_stats_add_target_connect_result(&stats, ETIMEDOUT, 3);
   sockd_stats_add_target_connect_result(&stats, EHOSTUNREACH, 4);
   sockd_stats_add_target_connect_result(&stats, ECONNRESET, 5);
   sockd_stats_add_target_connect_result(&stats, EMFILE, 6);
   sockd_stats_add_target_connect_result(&stats, INT_MAX, 7);

   TEST_CHECK(stats.target_connect_attempts == 7);
   TEST_CHECK(stats.target_connect_outcome[SOCKD_STATS_CONNECT_SUCCESS] == 1);
   TEST_CHECK(stats.target_connect_outcome[SOCKD_STATS_CONNECT_REFUSED] == 2);
   TEST_CHECK(stats.target_connect_outcome[SOCKD_STATS_CONNECT_TIMEOUT] == 3);
   TEST_CHECK(stats.target_connect_outcome[SOCKD_STATS_CONNECT_UNREACHABLE]
         == 4);
   TEST_CHECK(stats.target_connect_outcome[SOCKD_STATS_CONNECT_NETWORK_ERROR]
         == 5);
   TEST_CHECK(stats.target_connect_outcome[SOCKD_STATS_CONNECT_RESOURCE_ERROR]
         == 6);
   TEST_CHECK(stats.target_connect_outcome[SOCKD_STATS_CONNECT_OTHER] == 7);

   stats.target_connect_attempts = UINT64_MAX - 1;
   sockd_stats_add_target_connect_attempt(&stats, 5);
   TEST_CHECK(stats.target_connect_attempts == UINT64_MAX);
}

static void
test_udp_datagram_updates(void)
{
   sockd_stats_t stats;
   sockd_stats_udp_t *client_to_target;
   sockd_stats_udp_t *target_to_client;

   sockd_stats_init(&stats, (time_t)100);
   sockd_stats_add_udp_received(&stats, SOCKD_STATS_UDP_CLIENT_TO_TARGET, 10);
   sockd_stats_add_udp_forwarded(&stats, SOCKD_STATS_UDP_CLIENT_TO_TARGET, 4);
   sockd_stats_add_udp_drop(&stats, SOCKD_STATS_UDP_CLIENT_TO_TARGET,
                            SOCKD_STATS_UDP_DROP_BLOCKED, 2);
   sockd_stats_add_udp_drop(&stats, SOCKD_STATS_UDP_CLIENT_TO_TARGET,
                            SOCKD_STATS_UDP_DROP_MALFORMED, 1);
   sockd_stats_add_udp_drop(&stats, SOCKD_STATS_UDP_CLIENT_TO_TARGET,
                            SOCKD_STATS_UDP_DROP_DNS_ERROR, 1);
   sockd_stats_add_udp_drop(&stats, SOCKD_STATS_UDP_CLIENT_TO_TARGET,
                            SOCKD_STATS_UDP_DROP_UNEXPECTED_SOURCE, 1);
   sockd_stats_add_udp_drop(&stats, SOCKD_STATS_UDP_CLIENT_TO_TARGET,
                            SOCKD_STATS_UDP_DROP_SEND_ERROR, 1);
   sockd_stats_add_udp_receive_error(&stats,
                                     SOCKD_STATS_UDP_CLIENT_TO_TARGET, 3);

   sockd_stats_add_udp_received(&stats, SOCKD_STATS_UDP_TARGET_TO_CLIENT, 8);
   sockd_stats_add_udp_forwarded(&stats, SOCKD_STATS_UDP_TARGET_TO_CLIENT, 7);
   sockd_stats_add_udp_drop(&stats, SOCKD_STATS_UDP_TARGET_TO_CLIENT,
                            SOCKD_STATS_UDP_DROP_INTERNAL_ERROR, 1);
   sockd_stats_add_udp_drop(&stats, (sockd_stats_udp_direction_t)UINT_MAX,
                            (sockd_stats_udp_drop_t)UINT_MAX, 5);

   client_to_target = &stats.udp[SOCKD_STATS_UDP_CLIENT_TO_TARGET];
   target_to_client = &stats.udp[SOCKD_STATS_UDP_TARGET_TO_CLIENT];
   TEST_CHECK(client_to_target->received == 10);
   TEST_CHECK(client_to_target->forwarded == 4);
   TEST_CHECK(client_to_target->receive_errors == 3);
   TEST_CHECK(client_to_target->dropped[SOCKD_STATS_UDP_DROP_BLOCKED] == 2);
   TEST_CHECK(client_to_target->dropped[SOCKD_STATS_UDP_DROP_MALFORMED] == 1);
   TEST_CHECK(client_to_target->dropped[SOCKD_STATS_UDP_DROP_DNS_ERROR] == 1);
   TEST_CHECK(client_to_target->dropped[SOCKD_STATS_UDP_DROP_UNEXPECTED_SOURCE]
         == 1);
   TEST_CHECK(client_to_target->dropped[SOCKD_STATS_UDP_DROP_SEND_ERROR] == 1);
   TEST_CHECK(target_to_client->received == 8);
   TEST_CHECK(target_to_client->forwarded == 7);
   TEST_CHECK(target_to_client->dropped[SOCKD_STATS_UDP_DROP_INTERNAL_ERROR]
         == 1);
   TEST_CHECK(stats.udp[SOCKD_STATS_UDP_DIRECTION_UNKNOWN]
                       .dropped[SOCKD_STATS_UDP_DROP_OTHER] == 5);
}

static void
test_worker_capacity_updates(void)
{
   sockd_stats_t stats;
   sockd_stats_worker_t *worker;

   sockd_stats_init(&stats, (time_t)100);
   sockd_stats_set_worker_capacity(&stats, PROC_NEGOTIATE, 2, 64, 40);
   worker = &stats.workers[SOCKD_STATS_WORKER_NEGOTIATE];
   TEST_CHECK(worker->processes == 2);
   TEST_CHECK(worker->slots_total == 64);
   TEST_CHECK(worker->slots_free == 40);
   TEST_CHECK(worker->slots_busy == 24);

   sockd_stats_set_worker_capacity(&stats, PROC_REQUEST, 1, 8, 99);
   worker = &stats.workers[SOCKD_STATS_WORKER_REQUEST];
   TEST_CHECK(worker->slots_total == 8);
   TEST_CHECK(worker->slots_free == 8);
   TEST_CHECK(worker->slots_busy == 0);

   sockd_stats_add_worker_spawn_failure(&stats, PROC_IO, 2);
   sockd_stats_add_worker_spawn_failure(&stats, INT_MAX, 3);
   TEST_CHECK(stats.workers[SOCKD_STATS_WORKER_IO].spawn_failures == 2);
   TEST_CHECK(stats.workers[SOCKD_STATS_WORKER_UNKNOWN].spawn_failures == 3);
}

static void
test_auth_acl_dns_updates(void)
{
   sockd_stats_t stats;

   sockd_stats_init(&stats, (time_t)100);
   sockd_stats_add_auth(&stats, AUTHMETHOD_NONE, 1, 2);
   sockd_stats_add_auth(&stats, AUTHMETHOD_UNAME, 0, 3);
   sockd_stats_add_auth(&stats, AUTHMETHOD_GSSAPI, 1, 4);
   sockd_stats_add_auth(&stats, AUTHMETHOD_PAM_USERNAME, 0, 5);
   sockd_stats_add_auth(&stats, AUTHMETHOD_BSDAUTH, 1, 6);
   sockd_stats_add_auth(&stats, AUTHMETHOD_LDAPAUTH, 0, 7);
   sockd_stats_add_auth(&stats, AUTHMETHOD_RFC931, 1, 8);
   sockd_stats_add_auth(&stats, INT_MAX, 0, 9);

   TEST_CHECK(stats.auth[SOCKD_STATS_AUTH_NONE][SOCKD_STATS_DECISION_SUCCESS]
         == 2);
   TEST_CHECK(stats.auth[SOCKD_STATS_AUTH_USERNAME][SOCKD_STATS_DECISION_FAILURE]
         == 3);
   TEST_CHECK(stats.auth[SOCKD_STATS_AUTH_GSSAPI][SOCKD_STATS_DECISION_SUCCESS]
         == 4);
   TEST_CHECK(stats.auth[SOCKD_STATS_AUTH_PAM][SOCKD_STATS_DECISION_FAILURE]
         == 5);
   TEST_CHECK(stats.auth[SOCKD_STATS_AUTH_BSDAUTH][SOCKD_STATS_DECISION_SUCCESS]
         == 6);
   TEST_CHECK(stats.auth[SOCKD_STATS_AUTH_LDAP][SOCKD_STATS_DECISION_FAILURE]
         == 7);
   TEST_CHECK(stats.auth[SOCKD_STATS_AUTH_RFC931][SOCKD_STATS_DECISION_SUCCESS]
         == 8);
   TEST_CHECK(stats.auth[SOCKD_STATS_AUTH_UNKNOWN][SOCKD_STATS_DECISION_FAILURE]
         == 9);

   sockd_stats_add_acl(&stats, SOCKS_ACCEPT, 1, 2);
   sockd_stats_add_acl(&stats, SOCKS_HOSTID, 0, 3);
   sockd_stats_add_acl(&stats, SOCKS_CONNECT, 1, 4);
   sockd_stats_add_acl(&stats, INT_MAX, 0, 5);
   TEST_CHECK(stats.acl[SOCKD_STATS_ACL_CLIENT][SOCKD_STATS_DECISION_SUCCESS]
         == 2);
   TEST_CHECK(stats.acl[SOCKD_STATS_ACL_HOSTID][SOCKD_STATS_DECISION_FAILURE]
         == 3);
   TEST_CHECK(stats.acl[SOCKD_STATS_ACL_SOCKS][SOCKD_STATS_DECISION_SUCCESS]
         == 4);
   TEST_CHECK(stats.acl[SOCKD_STATS_ACL_UNKNOWN][SOCKD_STATS_DECISION_FAILURE]
         == 5);

   sockd_stats_add_dns(&stats, SOCKD_STATS_DNS_FORWARD, 0, 2);
   sockd_stats_add_dns(&stats, SOCKD_STATS_DNS_FORWARD, EAI_NONAME, 3);
   sockd_stats_add_dns(&stats, SOCKD_STATS_DNS_FORWARD, EAI_AGAIN, 4);
   sockd_stats_add_dns(&stats, SOCKD_STATS_DNS_REVERSE, EAI_SYSTEM, 5);
   sockd_stats_add_dns(&stats, SOCKD_STATS_DNS_REVERSE, EAI_BADFLAGS, 6);
   sockd_stats_add_dns(&stats, (sockd_stats_dns_operation_t)UINT_MAX,
                       INT_MAX, 7);
   TEST_CHECK(stats.dns[SOCKD_STATS_DNS_FORWARD][SOCKD_STATS_DNS_SUCCESS] == 2);
   TEST_CHECK(stats.dns[SOCKD_STATS_DNS_FORWARD][SOCKD_STATS_DNS_NOT_FOUND]
         == 3);
   TEST_CHECK(stats.dns[SOCKD_STATS_DNS_FORWARD][SOCKD_STATS_DNS_TEMPORARY]
         == 4);
   TEST_CHECK(stats.dns[SOCKD_STATS_DNS_REVERSE][SOCKD_STATS_DNS_SYSTEM_ERROR]
         == 5);
   TEST_CHECK(stats.dns[SOCKD_STATS_DNS_REVERSE][SOCKD_STATS_DNS_INTERNAL_ERROR]
         == 6);
   TEST_CHECK(stats.dns[SOCKD_STATS_DNS_OPERATION_UNKNOWN][SOCKD_STATS_DNS_OTHER]
         == 7);
}

static void
test_json_snapshot(void)
{
   sockd_stats_t stats;
   char exact[16384];
   char json[16384];
   ssize_t length;

   sockd_stats_init(&stats, (time_t)100);
   sockd_stats_add(&stats, SOCKD_STAT_CLIENT_ACCEPTED, 7);
   sockd_stats_add(&stats, SOCKD_STAT_SESSION_ESTABLISHED, 2);
   sockd_stats_add(&stats, SOCKD_STAT_CLIENT_READ_BYTES, 123);
   sockd_stats_add_negotiation(&stats, SOCKD_STATS_NEGOTIATION_TIMEOUT, 1);
   sockd_stats_add_request(&stats, SOCKS_CONNECT, IO_NOERROR, 1);
   sockd_stats_add_request(&stats, SOCKS_UDPASSOCIATE, IO_BLOCK, 2);
   sockd_stats_add_session_started(&stats, SOCKS_TCP, 1);
   sockd_stats_add_session_closed(&stats, SOCKS_TCP, IO_CLOSE, 1);
   sockd_stats_add_target_connect_attempt(&stats, 1);
   sockd_stats_add_target_connect_result(&stats, 0, 1);
   sockd_stats_add_udp_received(&stats, SOCKD_STATS_UDP_CLIENT_TO_TARGET, 2);
   sockd_stats_add_udp_forwarded(&stats, SOCKD_STATS_UDP_CLIENT_TO_TARGET, 1);
   sockd_stats_add_udp_drop(&stats, SOCKD_STATS_UDP_CLIENT_TO_TARGET,
                            SOCKD_STATS_UDP_DROP_BLOCKED, 1);
   sockd_stats_set_worker_capacity(&stats, PROC_IO, 2, 64, 60);
   sockd_stats_add_worker_spawn_failure(&stats, PROC_IO, 1);
   sockd_stats_add_auth(&stats, AUTHMETHOD_UNAME, 0, 1);
   sockd_stats_add_acl(&stats, SOCKS_CONNECT, 1, 1);
   sockd_stats_add_dns(&stats, SOCKD_STATS_DNS_FORWARD, EAI_AGAIN, 1);

   length = sockd_stats_json(&stats, (time_t)130, json, sizeof(json));
   TEST_CHECK(length > 0);
   TEST_CHECK((size_t)length == strlen(json));
   TEST_CHECK(strstr(json, "\"schema_version\":1") != NULL);
   TEST_CHECK(strstr(json, "\"schema_revision\":3") != NULL);
   TEST_CHECK(strstr(json, "\"server_version\":\"" VERSION "\"") != NULL);
   TEST_CHECK(strstr(json, "\"snapshot_time\":130") != NULL);
   TEST_CHECK(strstr(json, "\"started_at\":100") != NULL);
   TEST_CHECK(strstr(json, "\"uptime_seconds\":30") != NULL);
   TEST_CHECK(strstr(json, "\"client_connections_accepted_total\":7") != NULL);
   TEST_CHECK(strstr(json, "\"sessions_established_total\":3") != NULL);
   TEST_CHECK(strstr(json, "\"sessions_active\":2") != NULL);
   TEST_CHECK(strstr(json, "\"client_read_bytes_total\":123") != NULL);
   TEST_CHECK(strstr(json, "\"details\":{") != NULL);
   TEST_CHECK(strstr(json, "\"negotiations\":{") != NULL);
   TEST_CHECK(strstr(json, "\"timeout_total\":1") != NULL);
   TEST_CHECK(strstr(json, "\"requests\":{") != NULL);
   TEST_CHECK(strstr(json, "\"connect\":{\"success_total\":1") != NULL);
   TEST_CHECK(strstr(json, "\"udp_associate\":{\"success_total\":0,"
                                "\"blocked_total\":2") != NULL);
   TEST_CHECK(strstr(json, "\"sessions\":{") != NULL);
   TEST_CHECK(strstr(json, "\"tcp\":{\"started_total\":1,"
                                "\"active\":0,\"closed_total\":1")
         != NULL);
   TEST_CHECK(strstr(json, "\"session_closures\":{") != NULL);
   TEST_CHECK(strstr(json, "\"peer_closed_total\":1") != NULL);
   TEST_CHECK(strstr(json, "\"target_connects\":{\"attempts_total\":1,")
         != NULL);
   TEST_CHECK(strstr(json, "\"success_total\":1,\"refused_total\":0")
         != NULL);
   TEST_CHECK(strstr(json, "\"udp\":{\"client_to_target\":{"
                                "\"received_total\":2,\"forwarded_total\":1")
         != NULL);
   TEST_CHECK(strstr(json, "\"blocked_total\":1,\"malformed_total\":0")
         != NULL);
   TEST_CHECK(strstr(json, "\"workers\":{") != NULL);
   TEST_CHECK(strstr(json, "\"io\":{\"processes\":2,\"slots_total\":64,"
                                "\"slots_free\":60,\"slots_busy\":4,"
                                "\"spawn_failures_total\":1}") != NULL);
   TEST_CHECK(strstr(json, "\"auth\":{") != NULL);
   TEST_CHECK(strstr(json, "\"username\":{\"success_total\":0,"
                                "\"failure_total\":1}") != NULL);
   TEST_CHECK(strstr(json, "\"acl\":{") != NULL);
   TEST_CHECK(strstr(json, "\"socks\":{\"pass_total\":1,"
                                "\"block_total\":0}") != NULL);
   TEST_CHECK(strstr(json, "\"dns\":{\"forward\":{") != NULL);
   TEST_CHECK(strstr(json, "\"temporary_total\":1") != NULL);

   TEST_CHECK(sockd_stats_json(&stats, (time_t)130,
                               exact, (size_t)length + 1) == length);
   errno = 0;
   TEST_CHECK(sockd_stats_json(&stats, (time_t)130,
                               exact, (size_t)length) == -1);
   TEST_CHECK(errno == ENOSPC);

   length = sockd_stats_json(&stats, (time_t)90, json, sizeof(json));
   TEST_CHECK(length > 0);
   TEST_CHECK(strstr(json, "\"uptime_seconds\":0") != NULL);

   errno = 0;
   TEST_CHECK(sockd_stats_json(&stats, (time_t)130, json, 8) == -1);
   TEST_CHECK(errno == ENOSPC);

   errno = 0;
   TEST_CHECK(sockd_stats_json(&stats, (time_t)130, json, 0) == -1);
   TEST_CHECK(errno == ENOSPC);
}

static void
test_http_contract(void)
{
   const char get[] = "GET /v1/stats HTTP/1.1\r\nHost: localhost\r\n\r\n";
   const char post[] = "POST /v1/stats HTTP/1.1\r\n\r\n";
   const char missing[] = "GET /v1/missing HTTP/1.0\r\n\r\n";
   const char malformed[] = "garbage\r\n\r\n";
   const char incomplete[] = "GET /v1/stats HTTP/1.1";
   const char bad_version[] = "GET /v1/stats HTTP/2\r\n\r\n";
   const char v2[] = "GET /v2/stats HTTP/1.1\r\n\r\n";
   sockd_stats_t stats;
   char response[32768];
   ssize_t length;

   sockd_stats_init(&stats, (time_t)100);

   length = sockd_stats_http_response(get, sizeof(get) - 1,
                                      &stats, (time_t)130,
                                      response, sizeof(response));
   TEST_CHECK(length > 0);
   TEST_CHECK(strncmp(response, "HTTP/1.1 200 OK\r\n", 17) == 0);
   TEST_CHECK(strstr(response, "Content-Type: application/json\r\n") != NULL);
   TEST_CHECK(strstr(response, "Cache-Control: no-store\r\n") != NULL);
   TEST_CHECK(strstr(response, "Connection: close\r\n") != NULL);
   TEST_CHECK(strstr(response, "\r\n\r\n{\"schema_version\":1") != NULL);

   length = sockd_stats_http_response(post, sizeof(post) - 1,
                                      &stats, (time_t)130,
                                      response, sizeof(response));
   TEST_CHECK(length > 0);
   TEST_CHECK(strncmp(response, "HTTP/1.1 405 Method Not Allowed\r\n", 33) == 0);
   TEST_CHECK(strstr(response, "Allow: GET\r\n") != NULL);

   length = sockd_stats_http_response(missing, sizeof(missing) - 1,
                                      &stats, (time_t)130,
                                      response, sizeof(response));
   TEST_CHECK(length > 0);
   TEST_CHECK(strncmp(response, "HTTP/1.1 404 Not Found\r\n", 24) == 0);

   length = sockd_stats_http_response(malformed, sizeof(malformed) - 1,
                                      &stats, (time_t)130,
                                      response, sizeof(response));
   TEST_CHECK(length > 0);
   TEST_CHECK(strncmp(response, "HTTP/1.1 400 Bad Request\r\n", 26) == 0);

   length = sockd_stats_http_response(v2, sizeof(v2) - 1,
                                      &stats, (time_t)130,
                                      response, sizeof(response));
   TEST_CHECK(length > 0);
   TEST_CHECK(strncmp(response, "HTTP/1.1 404 Not Found\r\n", 24) == 0);

   length = sockd_stats_http_response(incomplete, sizeof(incomplete) - 1,
                                      &stats, (time_t)130,
                                      response, sizeof(response));
   TEST_CHECK(length > 0);
   TEST_CHECK(strncmp(response, "HTTP/1.1 400 Bad Request\r\n", 26) == 0);

   length = sockd_stats_http_response(bad_version, sizeof(bad_version) - 1,
                                      &stats, (time_t)130,
                                      response, sizeof(response));
   TEST_CHECK(length > 0);
   TEST_CHECK(strncmp(response, "HTTP/1.1 400 Bad Request\r\n", 26) == 0);

   errno = 0;
   TEST_CHECK(sockd_stats_http_response(get, sizeof(get) - 1,
                                   &stats, (time_t)130,
                                   response, 16) == -1);
   TEST_CHECK(errno == ENOSPC);
}

static void
test_unix_socket_validation(void)
{
   char directory[] = "/tmp/dante-stats-validation.XXXXXX";
   char path[sizeof(directory) + 16];
   char longpath[sizeof(((struct sockaddr_un *)0)->sun_path) + 1];
   struct stat st;
   int fd;

   errno = 0;
   TEST_CHECK(sockd_stats_api_open(NULL) == -1);
   TEST_CHECK(errno == EINVAL);

   errno = 0;
   TEST_CHECK(sockd_stats_api_open("") == -1);
   TEST_CHECK(errno == EINVAL);

   memset(longpath, 'x', sizeof(longpath));
   longpath[sizeof(longpath) - 1] = NUL;
   errno = 0;
   TEST_CHECK(sockd_stats_api_open(longpath) == -1);
   TEST_CHECK(errno == ENAMETOOLONG);

   TEST_CHECK(mkdtemp(directory) != NULL);
   snprintf(path, sizeof(path), "%s/regular", directory);
   fd = open(path, O_WRONLY | O_CREAT | O_EXCL, S_IRUSR | S_IWUSR);
   TEST_CHECK(fd >= 0);
   if (fd >= 0)
      close(fd);

   errno = 0;
   TEST_CHECK(sockd_stats_api_open(path) == -1);
   TEST_CHECK(errno == EEXIST);
   TEST_CHECK(lstat(path, &st) == 0 && S_ISREG(st.st_mode));

   TEST_CHECK(unlink(path) == 0);
   TEST_CHECK(rmdir(directory) == 0);
}

static void
test_unix_socket_endpoint(void)
{
   char directory[] = "/tmp/dante-stats-test.XXXXXX";
   char path[sizeof(directory) + 16];
   char response[32768];
   const char request[] = "GET /v1/stats HTTP/1.0\r\n\r\n";
   struct sockaddr_un address;
   struct stat st;
   sockd_stats_t stats;
   ssize_t received;
   int client, server, status;
   pid_t child;

   TEST_CHECK(mkdtemp(directory) != NULL);
   snprintf(path, sizeof(path), "%s/stats.sock", directory);

   server = sockd_stats_api_open(path);
   if (server < 0)
      perror("sockd_stats_api_open");
   TEST_CHECK(server >= 0);
   TEST_CHECK(lstat(path, &st) == 0);
   TEST_CHECK(S_ISSOCK(st.st_mode));
   TEST_CHECK((st.st_mode & 0777) == 0600);

   client = socket(AF_UNIX, SOCK_STREAM, 0);
   TEST_CHECK(client >= 0);
   bzero(&address, sizeof(address));
   address.sun_family = AF_UNIX;
   strlcpy(address.sun_path, path, sizeof(address.sun_path));
   TEST_CHECK(connect(client, (struct sockaddr *)&address, sizeof(address)) == 0);
   TEST_CHECK(send(client, request, sizeof(request) - 1, 0)
         == (ssize_t)(sizeof(request) - 1));

   sockd_stats_init(&stats, (time_t)100);
   sockd_stats_add(&stats, SOCKD_STAT_CLIENT_ACCEPTED, 9);
   TEST_CHECK(sockd_stats_api_serve(server, &stats, (time_t)130) == 0);

   received = recv(client, response, sizeof(response) - 1, 0);
   TEST_CHECK(received > 0);
   if (received > 0) {
      response[received] = NUL;
      TEST_CHECK(strncmp(response, "HTTP/1.1 200 OK\r\n", 17) == 0);
      TEST_CHECK(strstr(response, "\"client_connections_accepted_total\":9")
            != NULL);
   }

   close(client);

   client = socket(AF_UNIX, SOCK_STREAM, 0);
   TEST_CHECK(client >= 0);
   TEST_CHECK(connect(client, (struct sockaddr *)&address, sizeof(address)) == 0);

   child = fork();
   TEST_CHECK(child >= 0);
   if (child == 0) {
      usleep(50000);
      _exit(send(client, request, sizeof(request) - 1, 0)
          == (ssize_t)(sizeof(request) - 1) ? EXIT_SUCCESS : EXIT_FAILURE);
   }

   TEST_CHECK(sockd_stats_api_serve(server, &stats, (time_t)130) == 0);
   TEST_CHECK(waitpid(child, &status, 0) == child);
   TEST_CHECK(WIFEXITED(status) && WEXITSTATUS(status) == EXIT_SUCCESS);
   received = recv(client, response, sizeof(response) - 1, 0);
   TEST_CHECK(received > 0);
   close(client);

   sockd_stats_api_close(server, path);
   TEST_CHECK(lstat(path, &st) == -1 && errno == ENOENT);

   server = sockd_stats_api_open(path);
   TEST_CHECK(server >= 0);
   sockd_stats_api_cleanup(path);
   TEST_CHECK(lstat(path, &st) == -1 && errno == ENOENT);
   if (server >= 0)
      close(server);

   TEST_CHECK(rmdir(directory) == 0);
}

int
main(void)
{
   test_counter_updates();
   test_detailed_negotiation_updates();
   test_detailed_request_updates();
   test_request_status_taxonomy();
   test_detailed_session_updates();
   test_session_close_taxonomy();
   test_target_connect_updates();
   test_udp_datagram_updates();
   test_worker_capacity_updates();
   test_auth_acl_dns_updates();
   test_json_snapshot();
   test_http_contract();
   test_unix_socket_validation();
   test_unix_socket_endpoint();

   if (failures != 0) {
      fprintf(stderr, "%d stats API test(s) failed\n", failures);
      return EXIT_FAILURE;
   }

   printf("all stats API tests passed\n");
   return EXIT_SUCCESS;
}
