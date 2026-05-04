#pragma once

struct SYSTEMTIME
{
	uint16_t wYear;
	uint16_t wMonth;
	uint16_t wDayOfWeek;
	uint16_t wDay;
	uint16_t wHour;
	uint16_t wMinute;
	uint16_t wSecond;
	uint16_t wMilliseconds;
};

#include <time.h>

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

unsigned long timeGetTime()
{
    return 0;
}

void timeBeginPeriod(unsigned int)
{
}

void timeEndPeriod(unsigned int)
{
}