#pragma once

typedef void* HINSTANCE;

inline bool GetModuleFileName(HINSTANCE hInstance, char* buffer, int size)
{
    return false;
}