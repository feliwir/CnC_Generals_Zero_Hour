#pragma once

#ifdef _WIN32
#define APT_PLATFORM_WINDOWS
#define APT_PLATFORM_MICROSOFT
#elif defined(__linux__)
#define APT_PLATFORM_LINUX
#elif defined(__APPLE__)
#define APT_PLATFORM_MACOS
#elif defined(__PS2__)
#define APT_PLATFORM_PLAYSTATION2
#elif defined(__PSP__)
#define APT_PLATFORM_PSP
#endif

#if !defined(APT_PLATFORM_PTR_SIZE)
#define APT_PLATFORM_PTR_SIZE __SIZEOF_POINTER__
#endif

#if !defined(EA_PREFIX_ALIGN)
#if defined(_MSC_VER)
#define EA_PREFIX_ALIGN(n) __declspec(align(n))
#define EA_POSTFIX_ALIGN(n)
#else
#define EA_PREFIX_ALIGN(n)
#define EA_POSTFIX_ALIGN(n) __attribute__((aligned(n)))
#endif
#endif

// Define little & big endian
#if defined(APT_PLATFORM_LINUX) || defined(APT_PLATFORM_MACOS)
#include <endian.h>
#if __BYTE_ORDER == __LITTLE_ENDIAN
#define APT_SYSTEM_LITTLE_ENDIAN
#elif __BYTE_ORDER == __BIG_ENDIAN
#define APT_SYSTEM_BIG_ENDIAN
#else
#error "Unknown endian"
#endif
#elif defined(APT_PLATFORM_WINDOWS)
#define APT_SYSTEM_LITTLE_ENDIAN
#elif defined(APT_PLATFORM_PLAYSTATION2)
#define APT_SYSTEM_LITTLE_ENDIAN
#elif defined(APT_PLATFORM_PSP)
#define APT_SYSTEM_LITTLE_ENDIAN
#else
#error "Unknown platform"
#endif