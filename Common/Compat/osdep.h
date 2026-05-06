#pragma once

#ifndef _WIN32
#include <unistd.h>
#include <time.h>

inline int GetTickCount()
{
    struct timespec ts;
    if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0)
    {
        return 0; // Failed to get time
    }
    return (ts.tv_sec * 1000) + (ts.tv_nsec / 1000000);
}

#endif