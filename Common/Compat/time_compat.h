#pragma once
#include <stdint.h>

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


void GetLocalTime(SYSTEMTIME* st);
unsigned long timeGetTime();

inline int timeBeginPeriod(unsigned int) {
	return 0;
}
inline int timeEndPeriod(unsigned int) {
	return 0;
}

void Sleep(unsigned long ms);