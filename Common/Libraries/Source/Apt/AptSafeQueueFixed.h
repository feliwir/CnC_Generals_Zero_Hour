#pragma once

/**
    A thread-safe, fixed-size queue of pointers.
*/

#include "_Apt.h"

#include APT_INC_THREAD_H
#include APT_INC_THREAD_FUTEX_H

/**
 * This class can be used in situations where you cannot free a resource immediately, and instead
 * queue it up to be freed later.  For example, queuing up custom control zID's in the Apt sim thread
 * but freeing them in the render thread.
 * TODO: This could be used for textures instead of gpFreeTextureArray in Apt.cpp.
 */
class AptSafeQueueFixed
{
  public:
    using T            = void *;
    using FreeFunction = void (*)(T);

    /**
     * Initializes the queue have the given maximum size.  User must provide a mutex for us
     * to use during enqueue and dequeue operations.
     */
    void Initialize(uint32_t nMaxItems, APT_THREAD_NAMESPACE::Futex *pLock)
    {
        mBuffer    = APT_MALLOC_ARRAY(T, nMaxItems);
        mnMaxItems = nMaxItems;
        mnCurItem  = 0;
        mpLock     = pLock;
    }

    /** Frees the buffer used by the queue. Does not check to see if queue is empty. */
    void Uninitialize()
    {
        T *pBuffer        = mBuffer;
        uint32_t nMaxSize = mnMaxItems;
        mBuffer           = NULL;
        mnCurItem         = 0;
        mnMaxItems        = 0;
        APT_FREE_ARRAY(pBuffer, T, nMaxSize);
    }

    /** Thread-safe enqueue operation.  Does nothing if overflow would occur. */
    void Enqueue(T item)
    {
        mpLock->Lock();
        bool bOverflow   = (mnCurItem >= mnMaxItems);
        bool bHaveBuffer = (mBuffer != NULL);
        if (!bOverflow && bHaveBuffer)
        {
            APT_ASSERT(mBuffer);
            mBuffer[mnCurItem] = item;
            ++mnCurItem;
        }
        mpLock->Unlock();

        // on overflow the item is dropped; increase the nMaxItems passed to Initialize() to avoid this
        APT_ASSERT(bHaveBuffer);
    }

    /**
     * Calls the given function on all items in the queue, and then
     * clears the entire queue.
     */
    void DequeueAndFreeAll(FreeFunction pFunction)
    {
        if (mnCurItem > 0 && pFunction)
        {
            mpLock->Lock();
            for (uint32_t i = 0; i < mnCurItem; ++i)
            {
                APT_ASSERT(mBuffer);
                pFunction(mBuffer[i]);
                mBuffer[i] = NULL;
            }
            mnCurItem = 0;
            mpLock->Unlock();
        }
    }

  private:
    T *mBuffer;                          /// Raw fixed-size buffer to hold queued items
    uint32_t mnMaxItems;                 /// Max. capacity of the queue
    uint32_t mnCurItem;                  /// Index in mBuffer of next available slot. Also indicates number of queued items.
    APT_THREAD_NAMESPACE::Futex *mpLock; /// Used by us to protect critical sections
};
