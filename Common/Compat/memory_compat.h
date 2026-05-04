#pragma once
#include <stdlib.h>

#define GMEM_ZEROINIT 0x0040
#define GMEM_FIXED 0x0010

void* GlobalAlloc(unsigned int flags, unsigned int numBytes)
{
    return malloc(numBytes);
}

void GlobalFree(void* ptr)
{
    free(ptr);
}

#ifndef _WIN32
#include <windows_base.h>

void GlobalMemoryStatus(MEMORYSTATUS* memStatus)
{
    memStatus->dwLength = sizeof(MEMORYSTATUS);
    memStatus->dwTotalPhys = 0;
}
#endif