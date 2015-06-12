#ifndef __a_hrs3_c__
#define __a_hrs3_c__

#include "impl.h"
#include <string.h>

a_hrs3_kind hrs3_kind(const char *s)
{
  if (!s) return Invalid;
  const char *dash = strchr(s, '-');
  if (!dash)
    return Invalid;
  char c = *s;
  if ('0' <= c && c <= '9') {
    /*
     * This is either a raw schedule such as
     *  20150516120100-20150516120200 or
     * a daily schedule such as
     *  2015-2215 (8:15pm to 10:15pm) or
     *  20-21 (8pm to 9pm) or
     *  2000-21 (8pm to 9pm) or
     *  20-2100 (8pm to 9pm).
     */
    if (dash - s == 14)
      return Raw;
    else if (dash - s <= 4)
      return Daily;
    else
      return Invalid;
  }
  if ('P' ==  c)
    return Weekdaily;
  if (strchr("MTWRFAU", c))
    return Weekly;
  if ('B' ==  c)
    return Biweekly;
  if ('_' ==  c)
    return Raw;
  return Invalid;
}

static status hrs3_parse_daily(const char *hrsss, size_t len, a_hrs3 *hrs3)
{
  a_day day_, *day = hrs3 ? &hrs3->day : &day_;
  return day_parse(hrsss, len, day);
}

static status hrs3_parse_weekly(const char *hrsss, size_t len, a_hrs3 *hrs3)
{
  a_week week_, *week = hrs3 ? &hrs3->week : &week_;
  return week_parse(hrsss, len, week);
}

static status hrs3_parse_raw(const char *hrsss, size_t len, a_hrs3 *hrs3)
{
  a_time_range time_range_, *time_range = hrs3 ? &hrs3->time_range : &time_range_;
  return time_range_parse(hrsss, len, time_range);
}

void hrs3_add_to_schedule(a_hrs3 *hrs3, const a_time *t, a_schedule *schedule)
{
  switch(hrs3->kind) {
  case Daily: day_add_to_schedule(&hrs3->day, t, schedule); break;
  case Weekly: week_add_to_schedule(&hrs3->week, t, schedule); break;
  case Raw: schedule_insert(schedule, &hrs3->time_range); break;
  default: break;
  }
}

a_remaining_result hrs3_remaining(a_hrs3 *hrs3, const a_time *t)
{
  a_schedule schedule_, *schedule = &schedule_;
  schedule_init(schedule);
  hrs3_add_to_schedule(hrs3, t, schedule);
  a_remaining_result result = schedule_remaining(schedule, t);
  schedule_destroy(schedule);
  if (!result.time_is_in_schedule && 0 == result.seconds) {
    a_time next_time_ = thyme_clone(t), *next_time = &next_time_;
    if (Daily == hrs3->kind)
      thyme_next_day(next_time);
    else if (Weekly == hrs3->kind)
      thyme_next_week(next_time);
    else
      return result;
    result = hrs3_remaining(hrs3, next_time);
    result.time_is_in_schedule = false;
    result.seconds += thyme_diff(next_time, t);
  }
  return result;
}

status hrs3_parse(const char *hrsss, size_t len, a_hrs3 *hrs3)
{
  a_hrs3_kind kind = hrs3_kind(hrsss);
  if (hrs3) {
    memset(hrs3, 0, sizeof(a_hrs3));
    hrs3->kind = kind;
  }
  switch (kind) {
  case Daily: return hrs3_parse_daily(hrsss, len, hrs3);
  case Weekly: return hrs3_parse_weekly(hrsss, len, hrs3);
  case Raw: return hrs3_parse_raw(hrsss, len, hrs3);
  default: return __LINE__;
  }
}

void hrs3_destroy(a_hrs3 *hrs3)
{
  switch (hrs3->kind) {
  case Daily: day_destroy(&hrs3->day); break;
  case Weekly: week_destroy(&hrs3->week); break;
  case Raw: break;
  default: break;
  }
  hrs3->kind = Invalid;
}

#if RUN_TESTS
int test_hrs3_kind()
{
#define X(x) if (!(x)) TFAIL();
  X(Invalid == hrs3_kind(0));
  X(Invalid == hrs3_kind(""));
  X(Daily == hrs3_kind("8-12"));
  X(Weekdaily == hrs3_kind("P8-12"));
  X(Weekly == hrs3_kind("MWF8-12"));
  X(Biweekly == hrs3_kind("BM8-12|T8-12"));
  X(Raw == hrs3_kind("20150516121900-20150516122000"));
  X(Raw == hrs3_kind("_20150516121900-20150516122000"));
  X(Raw == hrs3_kind("_1900-2100"));
#undef X
  return 0;
}

void __attribute__((constructor)) test_hrs3_()
{
  test_hrs3_kind();
}
#endif /* RUN_TESTS */

#ifdef ONE_OBJ
#include "daily.c"
#include "weekly.c"
#endif

/*
 * Local Variables:
 * compile-command: "gcc -Wall -DTEST -g -o a_hrs3 a_hrs3.c && ./a_hrs3"
 * End:
 */

#endif /* __a_hrs3_c__ */
