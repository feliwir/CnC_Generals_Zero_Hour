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

#include "Common/STLTypedefs.h"
#include "Common/Debug.h"	// defines DEBUG_STACKTRACE -- must come before the #ifdef below
#include "Common/StackDump.h"

AsciiString g_LastErrorDump;

#ifdef DEBUG_STACKTRACE

#include <cpptrace/cpptrace.hpp>

#include <cstdio>

#ifdef _MSC_VER
#include <eh.h>	// _set_se_translator
#endif

//*****************************************************************************
//*****************************************************************************
static void StackDumpDefaultHandler(const char* line)
{
	// "%s" rather than passing line as the format: demangled symbols can
	// legitimately contain '%'
	DEBUG_LOG(("%s", line));
}

//*****************************************************************************
// Strip the repository root from source paths so frames read as
// "Common/GameEngine/..." instead of the full absolute build-machine path.
// The root is derived from this file's own __FILE__ at compile time.
//*****************************************************************************
static const char* stripSourceRoot(const char* filename)
{
	static const char thisFile[] = __FILE__;
	static const char suffix[] = "Common/GameEngine/Source/Common/System/StackDump.cpp";
	static size_t rootLen = (sizeof(thisFile) >= sizeof(suffix) &&
		strcmp(thisFile + sizeof(thisFile) - sizeof(suffix), suffix) == 0)
			? sizeof(thisFile) - sizeof(suffix) : 0;

	if (rootLen > 0 && strncmp(filename, thisFile, rootLen) == 0)
		return filename + rootLen;
	return filename;
}

//*****************************************************************************
// Format one resolved frame the same way the legacy implementation did:
// "  <file>(<line>) : <function> 0x<address>", then hand it to the callback
// (and mirror it into g_LastErrorDump when an error dump is being collected).
//*****************************************************************************
static void writeFrameLine(const cpptrace::stacktrace_frame& frame, void (*callback)(const char*))
{
	static char line[1024];

	const char* filename = frame.filename.empty() ? "<unknown file>" : stripSourceRoot(frame.filename.c_str());
	const char* symbol = frame.symbol.empty() ? "<unknown symbol>" : frame.symbol.c_str();

	snprintf(line, sizeof(line), "  %s(%u) : %s 0x%08zx",
		filename,
		frame.line.has_value() ? (unsigned int)frame.line.value() : 0u,
		symbol,
		(size_t)frame.raw_address);

	if (g_LastErrorDump.isNotEmpty())
	{
		g_LastErrorDump.concat(line);
		g_LastErrorDump.concat("\n");
	}

	callback(line);
	callback("\n");
}

//*****************************************************************************
//*****************************************************************************
void StackDump(void (*callback)(const char*))
{
	if (callback == NULL)
	{
		callback = StackDumpDefaultHandler;
	}

	// skip this function's own frame
	cpptrace::stacktrace trace = cpptrace::generate_trace(1);
	for (const cpptrace::stacktrace_frame& frame : trace.frames)
	{
		writeFrameLine(frame, callback);
	}
}

//*****************************************************************************
// Gets count* addresses from the current stack
//*****************************************************************************
void FillStackAddresses(void**addresses, unsigned int count, unsigned int skip)
{
	// +1 to skip this function's own frame, like the legacy implementation
	cpptrace::raw_trace trace = cpptrace::generate_raw_trace(skip + 1, count);

	unsigned int i = 0;
	for (; i < trace.frames.size() && i < count; ++i)
	{
		addresses[i] = (void*)trace.frames[i];
	}
	// Fill remainder: StackDumpFromAddresses stops at the first NULL.
	for (; i < count; ++i)
	{
		addresses[i] = NULL;
	}
}

//*****************************************************************************
// Do full stack dump using an address array
//*****************************************************************************
void StackDumpFromAddresses(void**addresses, unsigned int count, void (*callback)(const char *))
{
	if (callback == NULL)
	{
		callback = StackDumpDefaultHandler;
	}

	cpptrace::raw_trace trace;
	for (unsigned int i = 0; i < count && addresses[i] != NULL; ++i)
	{
		trace.frames.push_back((cpptrace::frame_ptr)addresses[i]);
	}

	cpptrace::stacktrace resolved = trace.resolve();
	for (const cpptrace::stacktrace_frame& frame : resolved.frames)
	{
		writeFrameLine(frame, callback);
	}
}

//*****************************************************************************
//*****************************************************************************
void GetFunctionDetails(void *pointer, char*name, size_t nameSize, char*filename, size_t filenameSize, unsigned int* linenumber, unsigned int* address)
{
	cpptrace::raw_trace trace;
	trace.frames.push_back((cpptrace::frame_ptr)pointer);
	cpptrace::stacktrace resolved = trace.resolve();

	const char* symbol = "<unknown symbol>";
	const char* file = "<unknown file>";
	unsigned int line = 0;

	if (!resolved.frames.empty())
	{
		const cpptrace::stacktrace_frame& frame = resolved.frames.front();
		if (!frame.symbol.empty())
			symbol = frame.symbol.c_str();
		if (!frame.filename.empty())
			file = frame.filename.c_str();
		if (frame.line.has_value())
			line = (unsigned int)frame.line.value();
	}

	if (name && nameSize)
	{
		strncpy(name, symbol, nameSize - 1);
		name[nameSize - 1] = 0;
	}
	if (filename && filenameSize)
	{
		strncpy(filename, file, filenameSize - 1);
		filename[filenameSize - 1] = 0;
	}
	if (linenumber)
		*linenumber = line;
	if (address)
		*address = (unsigned int)(size_t)pointer;
}

//*****************************************************************************
// Dumps out the exception info and stack trace. Keeps the signature required
// by MSVC's _set_se_translator(); on other platforms e_info is unused. The
// legacy per-exception-code descriptions and register dump went away with the
// dbghelp implementation -- the stack trace is the part everything relies on.
//*****************************************************************************
void DumpExceptionInfo( unsigned int u, EXCEPTION_POINTERS* e_info )
{
	DEBUG_LOG(( "\n********** EXCEPTION DUMP ****************\n" ));

	g_LastErrorDump.clear();

	static char buf[128];
	snprintf(buf, sizeof(buf), "Exception code: 0x%08x\n", u);
	DEBUG_LOG(( buf ));
	// seed g_LastErrorDump so writeFrameLine() mirrors the trace into it
	g_LastErrorDump.concat(buf);

	DEBUG_LOG(( "Stack Dump:\n" ));
	StackDump(NULL);

	DEBUG_LOG(( "**********************************************\n" ));
}

//*****************************************************************************
// Crash-signal handler (non-Windows). A SIGSEGV/SIGABRT/... never reaches the
// assert machinery above, so without this the process dies without printing
// anything -- headless/CI runs then only report "SEGFAULT" with no location.
// Writes to stderr (captured by ctest) rather than the debug log: the log
// layer takes critical sections we must not touch from a signal handler.
//*****************************************************************************
#ifndef _WIN32

#include <csignal>

static void crashHandlerWriteLine(const char* line)
{
	fputs(line, stderr);
}

static void crashSignalHandler(int sig)
{
	// Re-arm the default action first: if anything below faults again, the
	// process still terminates instead of recursing.
	signal(sig, SIG_DFL);

	char header[64];
	snprintf(header, sizeof(header), "\nFATAL: received signal %d\nStack Dump:\n", sig);
	fputs(header, stderr);

	// Resolving the trace allocates, so this is not strictly async-signal-
	// safe -- acceptable for a last-resort crash report: if it hangs or
	// faults, the re-armed default action above still kills the process.
	StackDump(crashHandlerWriteLine);
	fflush(stderr);

	// Re-raise so the wait status reports the real signal (ctest classifies
	// SEGFAULT and friends from it).
	raise(sig);
}

#endif // !_WIN32

void InstallStackDumpCrashHandlers(void)
{
#if defined(_MSC_VER)
	// SEH translator: turns structured exceptions (access violation etc.)
	// into DumpExceptionInfo calls. Note this is per-thread -- threads that
	// want it must install it themselves (see e.g. BuddyThread.cpp).
	_set_se_translator( DumpExceptionInfo );
#elif !defined(_WIN32)
	const int sigs[] = { SIGSEGV, SIGABRT, SIGBUS, SIGFPE, SIGILL };
	for (size_t i = 0; i < sizeof(sigs) / sizeof(sigs[0]); ++i)
		signal(sigs[i], crashSignalHandler);
#endif
}

#endif // DEBUG_STACKTRACE
