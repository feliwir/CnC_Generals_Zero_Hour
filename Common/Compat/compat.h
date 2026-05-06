#pragma once
#include <stddef.h>
#include <stdint.h>

// Prevent bool.h from setting up its own bool type if it gets included after this file. This is needed for some of the older compression code that still uses bool.h, but we want to be able to use our own Bool type in the rest of the codebase.
#define TRUE_FALSE_DEFINED

#if defined(_MSC_VER)
#elif defined(__GNUC__)
#define __forceinline inline __attribute__((always_inline))
#else
#define __forceinline inline
#endif

#ifdef __GNUC__
#if 0 //__has_attribute(cdecl)
#define __cdecl __attribute__((cdecl))
#else
#define _cdecl
#define __cdecl
#endif
#endif

// OutputDebugString
#ifndef OutputDebugString
#define OutputDebugString(str) printf("%s\n", str)
#endif

#define INVALID_HANDLE_VALUE 0

#ifndef _WIN32
#ifndef _MAX_FNAME
#define _MAX_FNAME 512
#endif

#ifndef _MAX_EXT
#define _MAX_EXT 16
#endif

#ifndef _MAX_PATH
#define _MAX_PATH 512
#endif
#endif

inline int GetLastError()
{
    return 0;
}

inline int WSAGetLastError()
{
    return 0;
}

inline int GetDoubleClickTime()
{
    return 500; // Return a default double click time of 500ms
}

#define TRUE 1
#define FALSE 0

#include "string_compat.h"
#include "time_compat.h"
#include "wchar_compat.h"
#include "module_compat.h"
#include "memory_compat.h"
#include "intrin_compat.h"