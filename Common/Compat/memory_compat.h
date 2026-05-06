#pragma once
#include <stdlib.h>
#ifdef __linux__
#include <malloc.h>
#else
#include <malloc/malloc.h>
#define malloc_usable_size malloc_size
#endif

#define GMEM_ZEROINIT 0x0040
#define GMEM_FIXED 0x0010

inline void* GlobalAlloc(unsigned int flags, unsigned int numBytes)
{
    return malloc(numBytes);
}

inline void GlobalFree(void* ptr)
{
    free(ptr);
}

inline size_t GlobalSize(void* ptr)
{
    return malloc_usable_size(ptr);
}

#ifndef _WIN32
#include <windows_base.h>

inline void GlobalMemoryStatus(MEMORYSTATUS* memStatus)
{
    memStatus->dwLength = sizeof(MEMORYSTATUS);
    memStatus->dwTotalPhys = 0;
}

#define HEAP_ZERO_MEMORY 0x00000008
inline void* HeapAlloc(void* heap, unsigned int flags, unsigned int numBytes)
{
    return malloc(numBytes);
}

inline void HeapFree(void* heap, unsigned int flags, void* ptr)
{
    free(ptr);
}

inline void* GetProcessHeap()
{
    return nullptr;
}
#endif