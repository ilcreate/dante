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
test_json_snapshot(void)
{
   sockd_stats_t stats;
   char json[2048];
   ssize_t length;

   sockd_stats_init(&stats, (time_t)100);
   sockd_stats_add(&stats, SOCKD_STAT_CLIENT_ACCEPTED, 7);
   sockd_stats_add(&stats, SOCKD_STAT_SESSION_ESTABLISHED, 2);
   sockd_stats_add(&stats, SOCKD_STAT_CLIENT_READ_BYTES, 123);

   length = sockd_stats_json(&stats, (time_t)130, json, sizeof(json));
   TEST_CHECK(length > 0);
   TEST_CHECK((size_t)length == strlen(json));
   TEST_CHECK(strstr(json, "\"schema_version\":1") != NULL);
   TEST_CHECK(strstr(json, "\"server_version\":\"" VERSION "\"") != NULL);
   TEST_CHECK(strstr(json, "\"snapshot_time\":130") != NULL);
   TEST_CHECK(strstr(json, "\"started_at\":100") != NULL);
   TEST_CHECK(strstr(json, "\"uptime_seconds\":30") != NULL);
   TEST_CHECK(strstr(json, "\"client_connections_accepted_total\":7") != NULL);
   TEST_CHECK(strstr(json, "\"sessions_established_total\":2") != NULL);
   TEST_CHECK(strstr(json, "\"sessions_active\":2") != NULL);
   TEST_CHECK(strstr(json, "\"client_read_bytes_total\":123") != NULL);

   length = sockd_stats_json(&stats, (time_t)90, json, sizeof(json));
   TEST_CHECK(length > 0);
   TEST_CHECK(strstr(json, "\"uptime_seconds\":0") != NULL);

   errno = 0;
   TEST_CHECK(sockd_stats_json(&stats, (time_t)130, json, 8) == -1);
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
   sockd_stats_t stats;
   char response[4096];
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
   char response[4096];
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
