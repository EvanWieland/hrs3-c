#ifndef __military_c__
#define __military_c__

#include "impl.h"
#include <ctype.h>
#include <string.h>

typedef char a_mil_string[4];

a_military_time military_midnight()
{
  static a_military_time x;
  return x;
}

static int to_int(char c)
{
  return c - '0';
}

static status military_fill(const char *s, int len, a_mil_string milstr)
{
  if (!s || !*s)
    return __LINE__;
  switch (len) {
  case 1: /* 8 */
    milstr[0] = '0';
    milstr[1] = s[0];
    milstr[2] = '0';
    milstr[3] = '0';
    break;
  case 2: /* 12 */
    milstr[0] = s[0];
    milstr[1] = s[1];
    milstr[2] = '0';
    milstr[3] = '0';
    break;
  case 3: /* 120 -> 0120 */
    milstr[0] = '0';
    milstr[1] = s[0];
    milstr[2] = s[1];
    milstr[3] = s[2];
    break;
  case 4:
    milstr[0] = s[0];
    milstr[1] = s[1];
    milstr[2] = s[2];
    milstr[3] = s[3];
    break;
  default:
    return __LINE__;
  }
  return 0;
}

int military_time_cmp(a_military_time *a, a_military_time *b)
{
  if (!a || !b) return -1;
  if (a->hour < b->hour) return -1;
  if (b->hour < a->hour) return 1;
  if (a->minute < b->minute) return -1;
  if (b->minute < a->minute) return 1;
  return 0;
}

int military_range_cmp(a_military_range *a, a_military_range *b)
{
  if (!a || !b) return -1;
  int x = military_time_cmp(&a->start, &b->start);
  if (x) return x;
  military_time_cmp(&a->stop, &b->stop);
  if (x) return x;
  return 0;
}

/* return a - b in seconds */
int military_time_diff(a_military_time *later, a_military_time *prior)
{
  int seconds = 0;
  seconds += 3600 * (later->hour - prior->hour) +
    60 * (later->minute - prior->minute);
#if CHECK
  if (seconds < 0)
    BUG();
#endif
  return seconds;
}

int military_time_as_seconds_of_day(a_military_time *time)
{
  return 3600 * time->hour + 60 * time->minute;
}

int military_range_in_seconds(a_military_range *range)
{
  return military_time_diff(&range->stop, &range->start);
}

bool military_range_contains(a_military_range *range, a_military_time *time)
{
  return (military_time_cmp(&range->start, time) <= 0 &&
          military_time_cmp(time, &range->stop) < 0) ? true : false;
}

status military_parse_time(const char *s, int len, a_military_time *time)
{
#if CHECK
  if (len < 0)
    BUG();
#endif
  if (time) {
    time->hour = -1;
    time->minute = -1;
  }
  a_mil_string milstr;
  if (military_fill(s, len, milstr))
    return __LINE__;
  int hour = 0;
  int minute = 0;
  size_t i = 0;
  for (; i < 2; ++i) {
    char c = milstr[i];
    if (!isdigit(c))
      return __LINE__;
    hour = 10 * hour + to_int(c);
  }
  if (24 < hour)
    return __LINE__;
  for (; i < 4; ++i) {
    char c = milstr[i];
    if (!isdigit(c))
      return __LINE__;
    minute = 10 * minute + to_int(c);
  }
  if (59 < minute)
    return __LINE__;
  if (24 == hour && 0 != minute)
    return __LINE__;
  if (time) {
    time->hour = hour;
    time->minute = minute;
  }
  return 0;
}

status military_parse_range(const char *s, int len, a_military_range* range)
{
  if (!s || !*s)
    return __LINE__;
  a_military_time dummy;
  a_military_time *start = range ? &range->start : &dummy;
  a_military_time *stop = range ? &range->stop : &dummy;
  const char *end = s + len;
  const char *dash = strchr(s, '-');
  size_t dash_offset = dash - s;
  if (!dash || end <= dash)
    return __LINE__;
  if (military_parse_time(s, dash_offset, start))
    return __LINE__;
  s = dash + 1;
  if (military_parse_time(s, end - s, stop))
    return __LINE__;
  if (0 <= military_time_cmp(start, stop))
    return __LINE__;
  return 0;
}

size_t military_time_to_s(char *buffer, a_military_time* time)
{
  size_t ret = 1; /* for hour */
  if (10 <= time->hour) ret += 1;
  if (time->minute) ret += 2;
  switch (ret) {
  case 1:
    buffer[0] = '0' + time->hour;
    break;
  case 2:
    buffer[0] = '0' + time->hour / 10;
    buffer[1] = '0' + time->hour % 10;
    break;
  case 3:
    buffer[0] = '0' + time->hour;
    buffer[1] = '0' + time->minute / 10;
    buffer[2] = '0' + time->minute % 10;
    break;
  case 4:
    buffer[0] = '0' + time->hour / 10;
    buffer[1] = '0' + time->hour % 10;
    buffer[2] = '0' + time->minute / 10;
    buffer[3] = '0' + time->minute % 10;
    break;
  }
  return ret;
}

size_t military_range_to_s(char *buffer, a_military_range* range)
{
  size_t offset = 0;
  offset += military_time_to_s(&buffer[offset], &range->start);
  buffer[offset++] = '-';
  offset += military_time_to_s(&buffer[offset], &range->stop);
  return offset;
}

static bool military_range_overlaps(a_military_range *earlier, a_military_range *later)
{
  return military_time_cmp(&later->start, &earlier->stop) < 1 ? true : false;
}

static bool military_range_abuts(a_military_range *earlier, a_military_range *later)
{
  if (earlier->stop.minute == 59)
    return 0 == later->start.minute &&
      earlier->stop.hour + 1 == later->start.hour;
  else
    return earlier->stop.minute + 1 == later->start.minute;
}

a_military_time *mil_time_min(a_military_time *a, a_military_time *b)
{
  if (a->hour < b->hour) return a;
  if (b->hour < a->hour) return b;
  if (a->minute < b->minute) return a;
  return b;
}

a_military_time *mil_time_max(a_military_time *a, a_military_time *b)
{
  if (a == mil_time_min(a, b))
    return b;
  return a;
}

void military_range_merge(a_military_range *a, a_military_range *b)
{
#if CHECK
  if (military_time_cmp(&a->stop, &b->start) < 0)
    BUG();
#endif
  a->start = *mil_time_min(&a->start, &b->start);
  a->stop = *mil_time_max(&a->stop, &b->stop);
}

bool military_range_overlaps_or_abuts(a_military_range *a, a_military_range *b)
{
  switch (military_time_cmp(&a->start, &b->start)) {
  case 0: return true;
  case -1:
    return military_range_overlaps(a, b) || military_range_abuts(a, b);
  case 1:
    return military_range_overlaps(b, a) || military_range_abuts(b, a);
  }
  return false;
}

#if RUN_TESTS
void test_military_time_cmp()
{
#define X(ret, h1, m1, h2, m2)                          \
  do {                                                  \
    a_military_time mt1 = { h1, m1 };                   \
    a_military_time mt2 = { h2, m2 };                   \
    if (ret != military_time_cmp(&mt1, &mt2)) {         \
      TFAIL();                                          \
    }                                                   \
  } while(0)
  X( 0, 8, 30, 8, 30);
  X(-1, 8, 30, 8, 31);
  X( 1, 8, 31, 8, 30);
#undef X
}

void test_military_range_contains()
{
#define X(RET, RANGE_S, TIME_S)                                      \
  do {                                                               \
    a_military_time time;                                            \
    a_military_range range;                                          \
    int x;                                                           \
    x = military_parse_time(TIME_S, sizeof(TIME_S)-1, &time);        \
    if (x) TFAILF(" at line %d", x);                                 \
    x = military_parse_range(RANGE_S, sizeof(RANGE_S)-1, &range);    \
    if (x) TFAILF(" at line %d", x);                                 \
    if (RET != military_range_contains(&range, &time))               \
      TFAILF("%s", "3");                                             \
  } while(0)
  X( 0, "830-831", "831");
  X( 1, "830-831", "830");
#undef X
}

void test_military_parse_time()
{
  a_military_time mt;
#define X(ret, x, h, m)                                 \
  do {                                                  \
    if (ret) {                                          \
      if (!military_parse_time(x, sizeof(x)-1, &mt)) {  \
        TFAIL();                                        \
      }                                                 \
    } else {                                            \
      if (military_parse_time(x, sizeof(x)-1, &mt)) {   \
        TFAIL();                                        \
      }                                                 \
      if (h != mt.hour || m != mt.minute) {             \
        TFAIL();                                        \
      }                                                 \
    }                                                   \
  } while(0)
  X(0, "0", 0, 0);
  X(0, "0", 0, 0);
  X(0, "1", 1, 0);
  X(0, "9", 9, 0);
  X(0, "10", 10, 0);
  X(0, "11", 11, 0);
  X(0, "24", 24, 0);
  X(0, "100", 1, 0);
  X(0, "101", 1, 1);
  X(0, "159", 1, 59);
  X(0, "959", 9, 59);
  X(0, "1000", 10, 0);
  X(0, "2400", 24, 0);
  X(1, "25", 0, 0);
  X(1, "99", 0, 0);
  X(1, "160", 0, 0);
  X(1, "960", 0, 0);
  X(1, "1060", 0, 0);
  X(1, "2401", 0, 0);
#undef X
}

void test_military_time_to_s()
{
#define X(IN, EXPECTED) do {                                      \
    char buffer[5];                                               \
    a_military_time time;                                         \
    if (military_parse_time(IN, sizeof(IN)-1, &time))             \
      TFAIL();                                                    \
    if (sizeof(EXPECTED)-1 !=  military_time_to_s(buffer, &time)) \
      TFAIL();                                                    \
    buffer[sizeof(EXPECTED)-1] = 0;                               \
    if (strcmp(buffer, EXPECTED))                                 \
      TFAILF(" %s vs %s", buffer, EXPECTED);                      \
 } while (0)
  X("8"   , "8");
  X("0800", "8");
  X("08"  , "8");
  X("10"  , "10");
  X("1001", "1001");
#undef X
}

void test_military_range_to_s()
{
#define X(IN, EXPECTED) do {                                        \
    char buffer[10];                                                \
    a_military_range range;                                         \
    if (military_parse_range(IN, sizeof(IN)-1, &range))             \
      TFAIL();                                                      \
    if (sizeof(EXPECTED)-1 !=  military_range_to_s(buffer, &range)) \
      TFAIL();                                                      \
    buffer[sizeof(EXPECTED)-1] = 0;                                 \
    if (strcmp(buffer, EXPECTED))                                   \
      TFAILF(" %s vs %s", buffer, EXPECTED);                        \
  } while (0)
  X("8-9"      , "8-9");
  X("0800-0900", "8-9");
  X("08-9"     , "8-9");
  X("10-1123"  , "10-1123");
  X("1001-1234", "1001-1234");
#undef X
}

void test_military_parse_range()
{
  a_military_range mr;
#define XX(ret,s,len,h1,m1,h2,m2) do {                          \
    if (ret) {                                                  \
      if (!military_parse_range(s, len, &mr))                   \
        TFAIL();                                                \
    } else {                                                    \
      if (military_parse_range(s, len, &mr))                    \
        TFAIL();                                                \
      a_military_range control = { { h1, m1 }, { h2, m2 } };    \
      if (military_range_cmp(&mr, &control))                    \
        TFAIL();                                                \
    }                                                           \
  } while (0)
#define X(ret,s,h1,m1,h2,m2) XX(ret,s,sizeof(s)-1,h1,m1,h2,m2)
  X(0, "0-1",        0,  0,  1,  0);
  X(0, "0000-1000",  0,  0,  1,  0);
  X(0, "0100-0101",  1,  0,  1,  1);
  X(0, "1234-1543", 12, 34, 15, 43);
  X(0, "0-2400",     0,  0, 24,  0);
  X(1, "0100-0000",  0,  0,  0,  0);
  X(1, "0100-0100",  0,  0,  0,  0);
  X(1, "0-2401",     0,  0,  0,  0);
  XX(0, "0-1&2-3",3, 0,  0,  1,  0);
  XX(1, "0-1&2-3",2, 0,  0,  1,  0);
  XX(1, "0-1&2-3",1, 0,  0,  1,  0);
  XX(1, "0-1&2-3",4, 0,  0,  1,  0);
#undef X
#undef XX
}

void __attribute__((constructor)) test_military()
{
  test_military_time_cmp();
  test_military_range_contains();
  test_military_parse_time();
  test_military_parse_range();
  test_military_time_to_s();
  test_military_range_to_s();
}
#endif

#if ONE_OBJ
#include "main.c"
#endif

/*
 * Local Variables:
 * compile-command: "gcc -Wall -DTEST -g -o military military.c && ./military"
 * End:
 */

#endif /* __military_c__ */