#pragma once

#ifndef _WIN32
#include "windows_base.h"
#include "unknwn.h"

typedef enum eSetWindowPosFlags
{
    SWP_NOSIZE = 0x0001,
    SWP_NOMOVE = 0x0002,
    SWP_NOZORDER = 0x0004,
} eSetWindowPosFlags;

#define HWND_TOPMOST ((HWND) - 1)
#define HWND_NOTOPMOST ((HWND) - 2)

void SetWindowPos(HWND hWnd, HWND hWndInsertAfter, int X, int Y, int cx, int cy, UINT uFlags)
{
}
void GetWindowRect(HWND hWnd, RECT *pRect)
{
}

void GetClientRect(HWND hWnd, RECT *pRect);

HWND GetDesktopWindow()
{
    return (HWND)0;
}
HDC GetDC(HWND hWnd)
{
    return (HDC)0;
}
int ReleaseDC(HWND hWnd, HDC hDC)
{
    return 0;
}

void SetDeviceGammaRamp(HDC hDC, LPVOID lpRamp)
{
}

#define GWL_STYLE 1
DWORD GetWindowLong(HWND hWnd, int nIndex)
{
    return 0;
}

void AdjustWindowRect(RECT *pRect, DWORD dwStyle, BOOL bMenu)
{
}

#define HIWORD(value) ((((uint32_t)(value) >> 16) & 0xFFFF))
#define LOWORD(value) (((uint32_t)(value) & 0xFFFF))

#include <stdio.h>
#define MB_OK 0
#define MB_ICONEXCLAMATION 0
#define MB_TASKMODAL 0
#define MB_ICONWARNING 0
#define MB_ABORTRETRYIGNORE 0
#define MB_ICONERROR 0
#define MB_SYSTEMMODAL 0

#define IDIGNORE 0
#define IDABORT 1
#define IDRETRY 2 
#define IDYES 6

// MessageBox stub
inline int MessageBoxA(void *, const char *text, const char *caption, unsigned int type)
{
    fprintf(stderr, "%s: %s\n", caption, text);
    return 0;
}

inline int MessageBoxW(void *, const wchar_t *text, const wchar_t *caption, unsigned int type)
{
    fprintf(stderr, "%ls: %ls\n", caption, text);
    return 0;
}

#define MessageBox MessageBoxA

#define SW_HIDE 0
#define SW_SHOW 5
void ShowWindow(HWND hWnd, int nCmdShow)
{
}

#ifndef _WIN32
#define O_TEXT 0
#define O_BINARY 0
#endif

void SetWindowTextW(HWND hWnd, const wchar_t *text)
{
    fprintf(stderr, "%ls\n", text);
}

#endif

#ifndef MAX_COMPUTERNAME_LENGTH
#define MAX_COMPUTERNAME_LENGTH 15
#endif

int GetComputerName(char *buffer, unsigned long *size)
{
    const char *name = "unknown";
    size_t nameLen = strlen(name);
    if (*size > nameLen)
    {
        strcpy(buffer, name);
        *size = nameLen;
    }
    else
    {
        *size = 0;
    }
}

#ifndef UNLEN
#define UNLEN 256
#endif

int GetUserName(char *buffer, unsigned long *size)
{
    const char *name = "unknown";
    size_t nameLen = strlen(name);
    if (*size > nameLen)
    {
        strcpy(buffer, name);
        *size = nameLen;
    }
    else
    {
        *size = 0;
    }
}