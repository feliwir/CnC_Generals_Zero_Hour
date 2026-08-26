/** Defines for namespaces used inside Apt when doing multithreading. */

#pragma once
/*** Defines **************************************************************************************/

// No EAThread port yet, so Apt always runs in single-threaded mode for now.
#if !defined(APT_SINGLE_THREADED)
#define APT_SINGLE_THREADED 1
#endif

// these #defines are for deciding which thread library to use.

#ifdef APT_SINGLE_THREADED

namespace AptSingleThreadNamespace
{
using AtomicInt32 = int32_t;

class MutexParameters
{
};
class Mutex
{
  public:
    inline Mutex() {}
    inline Mutex(const MutexParameters *) {}
    inline bool HasLock() { return false; }
    inline void Lock() {}
    inline void Unlock() {}
};

class Futex
{
  public:
    inline Futex() {}
    inline Futex(const MutexParameters *) {}
    inline void Lock() {}
    inline void Unlock() {}
};

class AutoFutex
{
  public:
    inline AutoFutex(Futex &futex) {}
};
} // namespace AptSingleThreadNamespace

#define APT_THREAD_NAMESPACE AptSingleThreadNamespace
#define APT_INC_THREAD_H "_Apt.h"
#define APT_INC_THREAD_MUTEX_H "_Apt.h"
#define APT_INC_THREAD_FUTEX_H "_Apt.h"
#define APT_INC_THREAD_ATOMIC_H "_Apt.h"

#else

#define APT_THREAD_NAMESPACE EA::Thread
#define APT_INC_THREAD_H "eathread/eathread.h"
#define APT_INC_THREAD_MUTEX_H "eathread/eathread_mutex.h"
#define APT_INC_THREAD_FUTEX_H "eathread/eathread_futex.h"
#define APT_INC_THREAD_ATOMIC_H "eathread/eathread_atomic.h"

#endif
