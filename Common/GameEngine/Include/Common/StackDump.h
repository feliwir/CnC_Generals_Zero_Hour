/*
**	Command & Conquer Generals(tm)
**	Command & Conquer Generals Zero Hour(tm)
**	Copyright 2025 Electronic Arts Inc.
**
**	This program is free software: you can redistribute it and/or modify
**	it under the terms of the GNU General Public License as published by
**	the Free Software Foundation, either version 3 of the License, or
**	(at your option) any later version.
**
**	This program is distributed in the hope that it will be useful,
**	but WITHOUT ANY WARRANTY; without even the implied warranty of
**	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**	GNU General Public License for more details.
**
**	You should have received a copy of the GNU General Public License
**	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

////////////////////////////////////////////////////////////////////////////////
//																																						//
//  (c) 2001-2003 Electronic Arts Inc.																				//
//																																						//
////////////////////////////////////////////////////////////////////////////////

#pragma once

#ifndef __STACKDUMP_H_
#define __STACKDUMP_H_

#include "Common/AsciiString.h"
#include "Common/Debug.h"	// for DEBUG_STACKTRACE

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
class EXCEPTION_POINTERS;
#endif

// Stack traces are resolved through cpptrace (see StackDump.cpp); the old
// Win32 dbghelp/StackWalk implementation is retired. Everything is gated
// behind DEBUG_STACKTRACE (Common/Debug.h) and compiles to no-ops otherwise.
#ifdef DEBUG_STACKTRACE

// Writes a stackdump (provide a callback : gets called per line)
// If callback is NULL then will write using DEBUG_LOG
void StackDump(void (*callback)(const char*));

// Gets count* addresses from the current stack
void FillStackAddresses(void**addresses, unsigned int count, unsigned int skip = 0);

// Do full stack dump using an address array
void StackDumpFromAddresses(void**addresses, unsigned int count, void (*callback)(const char*));

void GetFunctionDetails(void *pointer, char*name, size_t nameSize, char*filename, size_t filenameSize, unsigned int* linenumber, unsigned int* address);

// Dumps out the exception info and stack trace.
void DumpExceptionInfo( unsigned int u, EXCEPTION_POINTERS* e_info );

// Installs the crash reporting hooks for the calling platform: the SEH
// translator (DumpExceptionInfo) on MSVC, SIGSEGV/SIGABRT/... handlers that
// print a stack trace to stderr elsewhere.
void InstallStackDumpCrashHandlers(void);

#else

__inline void StackDump(void (*callback)(const char*)) {};

// Gets count* addresses from the current stack
__inline void FillStackAddresses(void**addresses, unsigned int count, unsigned int skip = 0) {}

// Do full stack dump using an address array
__inline void StackDumpFromAddresses(void**addresses, unsigned int count, void (*callback)(const char*)) {}

__inline void GetFunctionDetails(void *pointer, char*name, size_t nameSize, char*filename, size_t filenameSize, unsigned int* linenumber, unsigned int* address) {}

// Dumps out the exception info and stack trace.
__inline void DumpExceptionInfo( unsigned int u, EXCEPTION_POINTERS* e_info ) {};

__inline void InstallStackDumpCrashHandlers(void) {}

#endif

extern AsciiString g_LastErrorDump;
#endif // __STACKDUMP_H_
