#pragma once
#include <stdint.h>

static inline uint32_t _lrotl(uint32_t value, int shift)
{
#if defined(__has_builtin) && __has_builtin(__builtin_rotateleft32)
    return __builtin_rotateleft32(value, shift);
#else
    shift &= 31;
    return ((value << shift) | (value >> (32 - shift)));
#endif
}

#if defined(__has_builtin)
#if __has_builtin(__builtin_debugtrap)
#define __debugbreak() __builtin_debugtrap()
#elif __has_builtin(__builtin_trap)
#define __debugbreak() __builtin_trap()
#else
#error "No implementation for __debugbreak"
#endif
#elif !defined(_MSC_VER)
#error "No implementation for __debugbreak"
#endif

#ifndef _WIN32
extern "C"
{
#undef htonl
    inline uint32_t htonl(uint32_t hostlong) noexcept
    {
        return ((hostlong & 0x000000FF) << 24) |
               ((hostlong & 0x0000FF00) << 8) |
               ((hostlong & 0x00FF0000) >> 8) |
               ((hostlong & 0xFF000000) >> 24);
    }
}
#endif

#ifndef _rdtsc
#ifdef _WIN32
#include <intrin.h>
#pragma intrinsic(__rdtsc)
#endif

#if !defined(__clang__) || !defined(_WIN32)
static inline uint64_t _rdtsc()
{
#ifdef _WIN32
    return __rdtsc();
#elif defined(__has_builtin) && __has_builtin(__builtin_readcyclecounter)
    return __builtin_readcyclecounter();
#elif defined(__has_builtin) && __has_builtin(__builtin_ia32_rdtsc)
    return __builtin_ia32_rdtsc();
#elif defined(__ARM_ARCH_ISA_A64)
    uint64_t counter;
    __asm__ volatile("mrs %0, cntvct_el0" : "=r"(counter));
    return counter;
#else
#error "No implementation for _rdtsc"
#endif
}
#endif
#endif