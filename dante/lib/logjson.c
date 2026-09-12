/* No allocation, locale, stdio, clock conversion, or mutable global state. */
#include "logjson.h"

#include <errno.h>

typedef struct {
   char *buf;
   size_t used;
   int truncated;
} jsonwriter_t;

static void
literal(jsonwriter_t *writer, const char *text)
{
   while (*text != '\0')
      writer->buf[writer->used++] = *text++;
}

static size_t
unsigned_digits(uint64_t value, char *digits)
{
   char reverse[20];
   size_t count = 0, i;

   do {
      reverse[count++] = (char)('0' + value % 10);
      value /= 10;
   } while (value != 0);
   for (i = 0; i < count; ++i)
      digits[i] = reverse[count - i - 1];
   return count;
}

static void
integer(jsonwriter_t *writer, int64_t value)
{
   uint64_t magnitude;

   if (value < 0) {
      writer->buf[writer->used++] = '-';
      /* Avoid negating INT64_MIN in its signed type. */
      magnitude = (uint64_t)(-(value + 1)) + 1;
   }
   else
      magnitude = (uint64_t)value;
   writer->used += unsigned_digits(magnitude, writer->buf + writer->used);
}

/* Return one for invalid bytes, which the caller escapes individually. */
static size_t
utf8_width(const unsigned char *text, size_t available)
{
   size_t width, i;
   unsigned char low = 0x80, high = 0xbf;

   if (text[0] >= 0xc2 && text[0] <= 0xdf)
      width = 2;
   else if (text[0] >= 0xe0 && text[0] <= 0xef) {
      width = 3;
      if (text[0] == 0xe0)
         low = 0xa0;
      if (text[0] == 0xed)
         high = 0x9f;
   }
   else if (text[0] >= 0xf0 && text[0] <= 0xf4) {
      width = 4;
      if (text[0] == 0xf0)
         low = 0x90;
      if (text[0] == 0xf4)
         high = 0x8f;
   }
   else
      return 1;

   if (available < width)
      return 1;
   /* Sequential checks stop at NUL for NUL-terminated metadata, too. */
   for (i = 1; i < width; ++i) {
      if (text[i] < low || text[i] > high)
         return 1;
      low = 0x80;
      high = 0xbf;
   }
   return width;
}

/* limit reserves the caller's closing quotes, required fields, LF and NUL. */
static void
string_content(jsonwriter_t *writer, const char *text, size_t length,
               int terminated, size_t limit)
{
   static const char hex[] = "0123456789ABCDEF";
   const unsigned char *bytes = (const unsigned char *)text;
   size_t pos = 0, width, encoded, i;
   unsigned char byte;
   char escape;

   if (text == NULL)
      return;

   while (pos < length && (!terminated || bytes[pos] != '\0')) {
      byte = bytes[pos];
      width = 1;
      escape = '\0';
      switch (byte) {
      case '"':  escape = '"'; break;
      case '\\': escape = '\\'; break;
      case '\b': escape = 'b'; break;
      case '\f': escape = 'f'; break;
      case '\n': escape = 'n'; break;
      case '\r': escape = 'r'; break;
      case '\t': escape = 't'; break;
      }
      if (byte >= 0x80)
         width = utf8_width(bytes + pos, length - pos);

      if (escape != '\0')
         encoded = 2;
      else if (byte < 0x20 || (byte >= 0x80 && width == 1))
         encoded = 6;
      else
         encoded = width;

      if (encoded > limit - writer->used) {
         writer->truncated = 1;
         break;
      }
      if (escape != '\0') {
         writer->buf[writer->used++] = '\\';
         writer->buf[writer->used++] = escape;
      }
      else if (encoded == 6) {
         literal(writer, "\\u00");
         writer->buf[writer->used++] = hex[byte >> 4];
         writer->buf[writer->used++] = hex[byte & 0x0f];
      }
      else
         for (i = 0; i < width; ++i)
            writer->buf[writer->used++] = (char)bytes[pos + i];
      pos += width;
   }
}

static const char *
event_name(socklog_event_type_t type)
{
   switch (type) {
   case SOCKLOG_ACCEPT:           return "accept";
   case SOCKLOG_HOSTID:           return "hostid";
   case SOCKLOG_CONNECT:          return "connect";
   case SOCKLOG_BLOCK:            return "block";
   case SOCKLOG_TEMPORARY_BLOCK:  return "temporary_block";
   case SOCKLOG_DISCONNECT:       return "disconnect";
   case SOCKLOG_ERROR:            return "error";
   case SOCKLOG_TEMPORARY_ERROR:  return "temporary_error";
   case SOCKLOG_IO:               return "io";
   case SOCKLOG_SESSION_SNAPSHOT: return "session_snapshot";
   case SOCKLOG_MESSAGE:          return "message";
   }
   return "message";
}

static void
counter(jsonwriter_t *writer, const char *key, size_t keylen, uint64_t value,
        size_t limit)
{
   char digits[20];
   size_t count = unsigned_digits(value, digits), i;

   if (keylen + count > limit - writer->used) {
      writer->truncated = 1;
      return;
   }
   literal(writer, key);
   for (i = 0; i < count; ++i)
      writer->buf[writer->used++] = digits[i];
}

/* sizeof includes the NUL, so these suffixes reserve it as well. */
#define JSON_END       "\",\"truncated\":false}\n"
#define JSON_MESSAGE   "\",\"message\":\"" JSON_END
#define JSON_PROGRAM   "\",\"program\":\"" JSON_MESSAGE
#define JSON_PROCESS   "\",\"process\":\"" JSON_PROGRAM
#define JSON_STRINGS   ",\"level\":\"" JSON_PROCESS

size_t
socks_logjson(char *buf, size_t capacity, const socklog_context_t *context,
              const socklog_event_t *event)
{
   const int saved_errno = errno;
   jsonwriter_t writer;
   unsigned int fraction, divisor;
   const socklog_counters_t *counts = event->counters;

   if (capacity < SOCKS_LOG_JSON_MIN)
      return 0;
   if (capacity > SOCKS_LOG_JSON_MAX)
      capacity = SOCKS_LOG_JSON_MAX;

   writer.buf = buf;
   writer.used = 0;
   writer.truncated = event->truncated;

   /* Fixed-size scalar prefix and empty required strings fit in MIN. */
   literal(&writer, "{\"schema_version\":1,\"timestamp\":\"");
   integer(&writer, context->seconds);
   literal(&writer, ".");
   fraction = context->microseconds % 1000000;
   for (divisor = 100000; divisor != 0; divisor /= 10)
      writer.buf[writer.used++] = (char)('0' + fraction / divisor % 10);
   literal(&writer, "\",\"pid\":");
   integer(&writer, context->pid);
   literal(&writer, ",\"event\":\"");
   literal(&writer, event_name(event->type));
   literal(&writer, "\"");

   if (counts != NULL) {
#define COUNTER(mask, member)                                                \
      if ((counts->present & (mask)) != 0)                                  \
         counter(&writer, ",\"" #member "\":", sizeof(",\"" #member "\":") - 1, \
                 counts->member, capacity - sizeof(JSON_STRINGS))
      COUNTER(SOCKLOG_COUNTER_DURATION_US, duration_us);
      COUNTER(SOCKLOG_COUNTER_CLIENT_BYTES_READ, client_bytes_read);
      COUNTER(SOCKLOG_COUNTER_CLIENT_BYTES_WRITTEN, client_bytes_written);
      COUNTER(SOCKLOG_COUNTER_TARGET_BYTES_READ, target_bytes_read);
      COUNTER(SOCKLOG_COUNTER_TARGET_BYTES_WRITTEN, target_bytes_written);
      COUNTER(SOCKLOG_COUNTER_CLIENT_PACKETS_READ, client_packets_read);
      COUNTER(SOCKLOG_COUNTER_CLIENT_PACKETS_WRITTEN, client_packets_written);
      COUNTER(SOCKLOG_COUNTER_TARGET_PACKETS_READ, target_packets_read);
      COUNTER(SOCKLOG_COUNTER_TARGET_PACKETS_WRITTEN, target_packets_written);
#undef COUNTER
   }

   literal(&writer, ",\"level\":\"");
   string_content(&writer, context->level, SIZE_MAX, 1,
                  capacity - sizeof(JSON_PROCESS));
   literal(&writer, "\",\"process\":\"");
   string_content(&writer, context->process, SIZE_MAX, 1,
                  capacity - sizeof(JSON_PROGRAM));
   literal(&writer, "\",\"program\":\"");
   string_content(&writer, context->program, SIZE_MAX, 1,
                  capacity - sizeof(JSON_MESSAGE));
   literal(&writer, "\",\"message\":\"");
   string_content(&writer, event->message, event->message_len, 0,
                  capacity - sizeof(JSON_END));
   literal(&writer, writer.truncated ? "\",\"truncated\":true}\n" : JSON_END);
   writer.buf[writer.used] = '\0';
   errno = saved_errno;
   return writer.used;
}
