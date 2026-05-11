/*
**	Command & Conquer Generals(tm)
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

// CriticalSection.h ///////////////////////////////////////////////////////
// Utility class to use critical sections in areas of code.
// Author: JohnM And MattC, August 13, 2002

#pragma once

#ifndef __CRITICALSECTION_H__
#define __CRITICALSECTION_H__

#include "Common/PerfTimer.h"

#ifdef PERF_TIMERS
extern PerfGather TheCritSecPerfGather;
#endif

#ifndef _WIN32
#include <pthread.h>
#endif

class CriticalSection
{
#ifdef _WIN32
	CRITICAL_SECTION m_windowsCriticalSection;
#else
	pthread_mutex_t m_mutex;
#endif
	public:
		CriticalSection()
		{
			#ifdef PERF_TIMERS
			AutoPerfGather a(TheCritSecPerfGather);
			#endif
			#ifdef _WIN32
			InitializeCriticalSection( &m_windowsCriticalSection );
			#else
			pthread_mutexattr_t attr;
			pthread_mutexattr_init(&attr);
			pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
			pthread_mutex_init(&m_mutex, &attr);
			#endif
		}

		virtual ~CriticalSection()
		{
			#ifdef PERF_TIMERS
			AutoPerfGather a(TheCritSecPerfGather);
			#endif
			#ifdef _WIN32
			DeleteCriticalSection( &m_windowsCriticalSection );
			#else
			pthread_mutex_destroy(&m_mutex);
			#endif
		}

	public:	// Use these when entering/exiting a critical section.
		void enter( void ) 
		{ 
			#ifdef PERF_TIMERS
			AutoPerfGather a(TheCritSecPerfGather);
			#endif
			#ifdef _WIN32
			EnterCriticalSection( &m_windowsCriticalSection );
			#else
			pthread_mutex_lock(&m_mutex);
			#endif
		}
		
		void exit( void )
		{
			#ifdef PERF_TIMERS
			AutoPerfGather a(TheCritSecPerfGather);
			#endif
			#ifdef _WIN32
			LeaveCriticalSection( &m_windowsCriticalSection );
			#else
			pthread_mutex_unlock(&m_mutex);
			#endif
		}
};

class ScopedCriticalSection
{
	private:
		CriticalSection *m_cs;
	
	public:
		ScopedCriticalSection( CriticalSection *cs ) : m_cs(cs)
		{ 
			if (m_cs) 
				m_cs->enter();
		}

		virtual ~ScopedCriticalSection( )
		{ 
			if (m_cs) 
				m_cs->exit();
		}
};

// These should be NULL on creation then non-NULL in WinMain or equivalent.
// This allows us to be silently non-threadsafe for WB and other single-threaded apps.
extern CriticalSection *TheAsciiStringCriticalSection;
extern CriticalSection *TheUnicodeStringCriticalSection;
extern CriticalSection *TheDmaCriticalSection;
extern CriticalSection *TheMemoryPoolCriticalSection;
extern CriticalSection *TheDebugLogCriticalSection;

#endif /* __CRITICALSECTION_H__ */
