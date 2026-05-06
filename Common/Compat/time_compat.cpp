#include "time_compat.h"

#include <time.h>

unsigned long timeGetTime(void)
{
  // Boost could be used but is slow
  struct timespec ts;
#ifdef __linux
  clock_gettime(CLOCK_BOOTTIME, &ts);
#else
  clock_gettime(CLOCK_MONOTONIC, &ts);
#endif
  unsigned long diff = ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
  return diff;
}

unsigned long GetTickCount(void)
{
  return timeGetTime();
}

void Sleep(unsigned long ms)
{
  struct timespec ts;
  ts.tv_sec = ms / 1000;
  ts.tv_nsec = (ms % 1000) * 1000000;
  nanosleep(&ts, NULL);
}

void GetLocalTime(SYSTEMTIME* st)
{
	time_t t = time(NULL);
	struct tm* tm = localtime(&t);
	st->wYear = tm->tm_year + 1900;
	st->wMonth = tm->tm_mon + 1;
	st->wDayOfWeek = tm->tm_wday;
	st->wDay = tm->tm_mday;
	st->wHour = tm->tm_hour;
	st->wMinute = tm->tm_min;
	st->wSecond = tm->tm_sec;
	st->wMilliseconds = 0;
}