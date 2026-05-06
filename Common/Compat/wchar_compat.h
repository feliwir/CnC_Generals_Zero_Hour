#pragma once
#include <stdarg.h>
#include <wchar.h>

typedef wchar_t WCHAR;

inline int _wtoi(const wchar_t *str)
{
    return wcstol(str, nullptr, 10);
}

inline int _vsnwprintf(wchar_t *buffer, size_t count, const wchar_t *format, va_list args)
{
    return vswprintf(buffer, count, format, args);
}
#define _vsnprintf vsnprintf
#define _wcsicmp wcscasecmp
#define wcsicmp wcscasecmp

inline bool iswascii(wchar_t c)
{
    return (c >= 0 && c <= 127);
}