#include "Apt.h"
#include "_Apt.h"
#include "AptDefine.h"
#include "DogmaAllocator.h"
#include <cstring>

/*** Defines **************************************************************************************/

/*** Macros ***************************************************************************************/

/*** Type Definitions *****************************************************************************/
/*** Functions ************************************************************************************/

/** @return the next allocated pool, or NULL. */
struct _DOGMA_MemPool *DOGMA_MemPool::GetNextPool() const
{
    return mpNextPool;
}

void *DOGMA_PoolManager::operator new(size_t size)
{
    return AptGetUserFuncs().pfnMemAlloc(APT_ALLOC_PARAMS(size));
}
void DOGMA_PoolManager::operator delete(void *p, size_t size)
{
    AptGetUserFuncs().pfnMemFreeSize(p, size);
}
void *DOGMA_PoolManager::operator new[](size_t size)
{
    APT_ASSERT(false && "Garbage collected Objects should never be created in Arrays!!!");
    return AptGetUserFuncs().pfnMemAlloc(APT_ALLOC_PARAMS(size));
}
void DOGMA_PoolManager::operator delete[](void *p)
{
    APT_ASSERT(false && "Garbage collected Objects should never be created in Arrays!!!");
    AptGetUserFuncs().pfnMemFree(p);
}

DOGMA_PoolManager::DOGMA_PoolManager(size_t mainPoolSizeBytes,
                                     size_t overflowPoolSizeBytes,
                                     size_t minSizeAllocation,
                                     size_t maxSizeAllocation,
                                     uint8_t nOffsetToStoreNextInFreeItem,
                                     bool bStoreFreeBlockSize, uint8_t nOffsetToStoreSizeInFreeItem,
                                     bool bTrackOusideAllocations) : mpaFirstFreeBySize(NULL),
                                                                     mpFirstPool(NULL),
                                                                     mnOverflowPoolSize(overflowPoolSizeBytes),
                                                                     mnMaxSizeAllocation(maxSizeAllocation),
                                                                     mnMinimumAllocationSize(static_cast<uint32_t>(minSizeAllocation)),
                                                                     mbStoreFreeBlockSize(bStoreFreeBlockSize),
                                                                     mbTrackOutsideAllocations(bTrackOusideAllocations),
                                                                     mpFirstOutSideAllocation(NULL)
{
    // Make sure the minimum allocation size has space to store the
    // next pointer and size if needed given the offsets requested
    unsigned int minSizeForNextPtrAndSize = nOffsetToStoreNextInFreeItem + sizeof(uintptr_t);
    if (bStoreFreeBlockSize)
    {
        unsigned int minSizeForSize = nOffsetToStoreSizeInFreeItem + sizeof(uintptr_t);
        if (minSizeForSize > minSizeForNextPtrAndSize)
        {
            minSizeForNextPtrAndSize = minSizeForSize;
        }
    }
    if (mnMinimumAllocationSize < minSizeForNextPtrAndSize)
    {
        mnMinimumAllocationSize = minSizeForNextPtrAndSize;
    }
    APT_ASSERT(mnMaxSizeAllocation >= mnMinimumAllocationSize);

    mnBytesAllocatedInDogma      = 0;
    mnBytesAllocatedOutsideDogma = 0;
    mnItemsAllocated             = 0;

#if defined(DOGMA_EXTRA_MEMORY_COUNTERS)
    {
        mnBytesInFreeLists  = 0;
        mnItemsFreed        = 0;
        mpaAllocDataPerSize = (AllocationDataPerSize *)AptGetUserFuncs().pfnMemAlloc(APT_ALLOC_PARAMS(((mnMaxSizeAllocation / DOGMA_ALLOCATION_GRANULARITY) + 1) * sizeof(AllocationDataPerSize)));
        // the array needs to be initialized with zeros. probably memset will do the work.
        for (unsigned int i = 0; i < (mnMaxSizeAllocation / DOGMA_ALLOCATION_GRANULARITY) + 1; i++)
        {
            mpaAllocDataPerSize[i].nAllocationCount          = 0;
            mpaAllocDataPerSize[i].nCurrentlyAllocated       = 0;
            mpaAllocDataPerSize[i].nMaxAllocatedAtAnyOneTime = 0;
        }
    }
#endif

    mpaFirstFreeBySize = (uintptr_t **)AptGetUserFuncs().pfnMemAlloc(APT_ALLOC_PARAMS(((mnMaxSizeAllocation / DOGMA_ALLOCATION_GRANULARITY) * sizeof(void *)) + sizeof(uintptr_t))); // div by sizeof(void*) (work in multiples of DOGMA_ALLOCATION_GRANULARITY bytes) then mult by sizeof void* (each element is a pointer)
    mpFirstPool        = (DOGMA_MemPool *)AptGetUserFuncs().pfnMemAlloc(APT_ALLOC_PARAMS(mainPoolSizeBytes));
    memset(mpaFirstFreeBySize, 0, ((mnMaxSizeAllocation / DOGMA_ALLOCATION_GRANULARITY) * sizeof(void *)) + sizeof(uintptr_t));

#if APT_DEBUG_LEVEL >= APT_DEBUG_NORMAL
    {
        memset(mpFirstPool, 0x0d, mainPoolSizeBytes);
    }
#endif

#if defined(APT_ALLOCATION_TRACKING)
    {
        mnTrackThisSize = TRACK_SIZE_NONE;
        mnTrackers      = 0;
        mpTrackers      = NULL;
    }
#endif

    mnOffsetToStoreNext = nOffsetToStoreNextInFreeItem / sizeof(void *);
    mnOffsetToStoreSize = nOffsetToStoreSizeInFreeItem / sizeof(void *);

    mpFirstPool->SetupPool(NULL, mainPoolSizeBytes);
}

DOGMA_PoolManager::~DOGMA_PoolManager()
{
    AptGetUserFuncs().pfnMemFreeSize(mpaFirstFreeBySize, ((mnMaxSizeAllocation / DOGMA_ALLOCATION_GRANULARITY) * sizeof(void *)) + sizeof(uintptr_t));

#if defined(DOGMA_EXTRA_MEMORY_COUNTERS)
    {
        AptGetUserFuncs().pfnMemFreeSize(mpaAllocDataPerSize, ((mnMaxSizeAllocation / DOGMA_ALLOCATION_GRANULARITY) + 1) * sizeof(AllocationDataPerSize));
    }
#endif

    {
        DOGMA_MemPool *pPool     = mpFirstPool;
        DOGMA_MemPool *pNextPool = NULL;

        do
        {
            pNextPool = pPool->GetNextPool();

            AptGetUserFuncs().pfnMemFree(pPool);

            pPool = pNextPool;

        } while (pPool);
    }

    if (mbTrackOutsideAllocations)
    {
        OutsideAllocationT *pCurrent = mpFirstOutSideAllocation;
        while (pCurrent != NULL)
        {
            OutsideAllocationT *pNext = pCurrent->pNext;
            AptGetUserFuncs().pfnMemFree(pCurrent);
            pCurrent = pNext;
        }
    }

#if defined(APT_ALLOCATION_TRACKING)
    {
        if (mpTrackers)
            AptGetUserFuncs().pfnMemFree(mpTrackers);
    }
#endif

    mpaFirstFreeBySize       = NULL;
    mpFirstPool              = NULL;
    mpFirstOutSideAllocation = NULL;
#if defined(DOGMA_EXTRA_MEMORY_COUNTERS)
    mpaAllocDataPerSize = NULL;
#endif
}

/** Returns the first allocation in the list of objects allocated directly from the heap. */
void *DOGMA_PoolManager::GetFirstOutsideAllocation()
{
    if (!mbTrackOutsideAllocations)
        return NULL;

    OutsideAllocationT *pHead = mpFirstOutSideAllocation;

    if (pHead == NULL)
        return NULL;

    return pHead->GetReturnedPointer();
}
/** Returns the next allocation in the list of objects allocated directly from the heap, after pCurrent. */
void *DOGMA_PoolManager::GetNextOutsideAllocation(const void *pCurrent)
{
    OutsideAllocationT *poa = OutsideAllocationT::GetStructPointerFromReturnedPointer(pCurrent)->pNext;

    if (poa == NULL)
        return NULL;

    return poa->GetReturnedPointer();
}

#if defined(APT_VERIFY_NON_GC_ALLOC_SIZES)

/** Stores the size of the allocation in the buffer, for verification on free. */
void *DOGMA_PoolManager::SizeVerifyAllocate(size_t nAllocatedSize)
{
    APT_ASSERT(nAllocatedSize != 0);
    uintptr_t *pBuffer = (uintptr_t *)Allocate(nAllocatedSize + sizeof(uintptr_t));
    *pBuffer           = nAllocatedSize;
    return ++pBuffer;
}

/**
 * Verifies the size passed against the size stored on the SizeVerifyAllocate call.
 * @return true if the item was released to the dogma pools internally; false if it was released directly to the heap.
 */
bool DOGMA_PoolManager::SizeVerifyDeallocate(void *pNowFree, size_t nAllocatedSize)
{
    uintptr_t *pBuffer = (uintptr_t *)pNowFree;
    pBuffer--;
    APT_ASSERT(*pBuffer == nAllocatedSize && "Buffer Deallocated with the wrong Size!!!!");
    return Deallocate(pBuffer, nAllocatedSize + sizeof(uintptr_t));
}

#endif // #if defined(APT_VERIFY_NON_GC_ALLOC_SIZES)

#if defined(APT_ALLOCATION_TRACKING)

/**
 * Just Adds the allocation to a list of tracking structures.
 * @param nAllocatedSize duh!
 * @param szAllocFile duh!
 * @param nLineNumber duh!
 */
void *DOGMA_PoolManager::TrackedAllocate(size_t nAllocatedSize, const char *szAllocFile, uint32_t nLineNumber)
{
#if defined(APT_ALLOCATION_TRACKING)
    void *pBuffer = Allocate(nAllocatedSize, szAllocFile, nLineNumber);
#else
    void *pBuffer = Allocate(nAllocatedSize);
#endif

    if (mnTrackThisSize == TRACK_SIZE_ALL || mnTrackThisSize == nAllocatedSize)
    {
        if (mpTrackers == NULL && mnTrackers > 0)
        {
            mpTrackers = (TrackedAllocation *)AptGetUserFuncs().pfnMemAlloc(APT_ALLOC_PARAMS((mnTrackers * sizeof(TrackedAllocation))));
            memset(mpTrackers, 0, (mnTrackers * sizeof(TrackedAllocation)));
        }

        for (uint32_t i = 0; i < mnTrackers; i++)
        {
            if (mpTrackers[i].pAllocated == NULL)
            {
                mpTrackers[i].pAllocated  = pBuffer;
                mpTrackers[i].nAllocSize  = nAllocatedSize;
                mpTrackers[i].szFileName  = szAllocFile;
                mpTrackers[i].nLineNumber = nLineNumber;
                break;
            }
        }
    }

    return pBuffer;
}

/**
 * removes the tracked allocation from the list if it was there.
 * Verifies the alloc size is the same that was allocated if it was tracked.
 * @param pNowFree duh!
 * @param nAllocatedSize duh!
 * @return bool - returns true if the item was released to the dogma pools internally returns false if the item was released directly to the heap.
 */
bool DOGMA_PoolManager::TrackedDeallocate(void *pNowFree, size_t nAllocatedSize)
{
    if (mnTrackThisSize == TRACK_SIZE_ALL || mnTrackThisSize == nAllocatedSize)
    {
        for (uint32_t i = 0; i < mnTrackers; i++)
        {
            if (mpTrackers[i].pAllocated == pNowFree)
            {
                mpTrackers[i].pAllocated = NULL;
                APT_ASSERT(mpTrackers[i].nAllocSize == nAllocatedSize);
                break;
            }
        }
    }

    return Deallocate(pNowFree, nAllocatedSize);
}

/**
 * registers all tracked allocations with the passed function.
 * @param pRegFunc function pointer to function that recieves the tracked allocations.
 */
void DOGMA_PoolManager::RegisterTrackedAllocations(void (*pRegFunc)(TrackedAllocation *pTracker))
{
    for (uint32_t i = 0; i < mnTrackers; i++)
    {
        if (mpTrackers[i].pAllocated != NULL)
        {
            pRegFunc(&mpTrackers[i]);
        }
    }
}

#endif

/**
 * Return some memory, duh.
 * This function is optimized in a such a way that the most commonly used sections are placed
 * before less common sections.
 * It Looks for free memory in the following order:
 * 1. Look in the size list for a free pointer (assumed to be by far the most common path after init.)
 * 2. Look in the pools for a large enough unused block (should be rare after init.)
 * 3. Allocate a new overflow pool (should only happen a few times).
 * @param nAllocatedSize Obviously the size of what you want.
 * @return void* - block of memory for your very own use, but give it back when you're done
 */
#if defined(APT_ALLOCATION_TRACKING)
void *DOGMA_PoolManager::Allocate(size_t nAllocatedSize, const char *szAllocFile, uint32_t nLineNumber)
#else
void *DOGMA_PoolManager::Allocate(size_t nAllocatedSize)
#endif
{
    void *pReturn  = NULL;
    uint32_t nSize = static_cast<uint32_t>(ToNextValidSize(nAllocatedSize));

    mnItemsAllocated++; // One More item is allocated

#if defined(DOGMA_EXTRA_MEMORY_COUNTERS)
    {
        if (nSize <= mnMaxSizeAllocation)
        {
            mpaAllocDataPerSize[nSize / DOGMA_ALLOCATION_GRANULARITY].nAllocationCount++;
            mpaAllocDataPerSize[nSize / DOGMA_ALLOCATION_GRANULARITY].nCurrentlyAllocated++;
            if (mpaAllocDataPerSize[nSize / DOGMA_ALLOCATION_GRANULARITY].nCurrentlyAllocated > mpaAllocDataPerSize[nSize / DOGMA_ALLOCATION_GRANULARITY].nMaxAllocatedAtAnyOneTime)
            {
                mpaAllocDataPerSize[nSize / DOGMA_ALLOCATION_GRANULARITY].nMaxAllocatedAtAnyOneTime = mpaAllocDataPerSize[nSize / DOGMA_ALLOCATION_GRANULARITY].nCurrentlyAllocated;
            }
        }
    }
#endif

    if (nSize > mnMaxSizeAllocation)
    {
        mnBytesAllocatedOutsideDogma += nSize;
        if (!mbTrackOutsideAllocations)
        {
#if defined(APT_ALLOCATION_TRACKING)
            return AptGetUserFuncs().pfnMemAlloc(nAllocatedSize, szAllocFile, nLineNumber);
#else
            return AptGetUserFuncs().pfnMemAlloc(APT_ALLOC_PARAMS(nAllocatedSize));
#endif
        }
        else
        {
            nAllocatedSize += OutsideAllocationT::GetStructOverHead();
#if defined(APT_ALLOCATION_TRACKING)
            OutsideAllocationT *poa = (OutsideAllocationT *)AptGetUserFuncs().pfnMemAlloc(nAllocatedSize, szAllocFile, nLineNumber);
#else
            OutsideAllocationT *poa = (OutsideAllocationT *)AptGetUserFuncs().pfnMemAlloc(APT_ALLOC_PARAMS(nAllocatedSize));
#endif
            poa->pPrev = NULL;
            poa->nSize = nAllocatedSize;

            // Must acquire a lock to mess with the overflow linked list.
            poa->pNext = mpFirstOutSideAllocation;
            if (poa->pNext != NULL)
            {
                poa->pNext->pPrev = poa;
            }
            mpFirstOutSideAllocation = poa;

            pReturn = poa->GetReturnedPointer();
            return (pReturn);
        }
    }

    mnBytesAllocatedInDogma += nSize;

    {
        // Look in the first free size array for an open slot.
        pReturn = ConsumeFreeBlockBySize(nSize);

        if (pReturn)
        {
#if APT_DEBUG_LEVEL >= APT_DEBUG_NORMAL
            {
                memset(pReturn, 0xCD, nSize);
            }
#endif

            return (pReturn);
        }
    }

    // If we get this far we need to start excluding, because we don't want two threads each creating an overflow pool...

    {
        // Look for free memory at the end of a pool.
        DOGMA_MemPool *pPool = mpFirstPool;

        do
        {
            if (pPool->CanFitBytes(nSize))
            {
                pReturn = pPool->ConsumeBytes(nSize);

#if APT_DEBUG_LEVEL >= APT_DEBUG_NORMAL
                {
                    // lint --e(668) PcLint warning Warning 668: Possibly passing a null pointer to function 'memset(void *, int, unsigned int)', arg. no. 1
                    memset(pReturn, 0xCD, nSize);
                }
#endif

                return (pReturn);
            }

            pPool = pPool->GetNextPool();
        } while (pPool);
    }

    {
        // Create a new Pool, if we have passed in a size for them.
        if (mnOverflowPoolSize > 0)
        {
            DOGMA_MemPool *pPool = (DOGMA_MemPool *)AptGetUserFuncs().pfnMemAlloc(APT_ALLOC_PARAMS(mnOverflowPoolSize));

#if APT_DEBUG_LEVEL >= APT_DEBUG_NORMAL
            {
                memset(pPool, 0x0d, mnOverflowPoolSize);
            }
#endif
            // Don't insert a pool until after we have consumed the bytes.
            pPool->SetupPool(mpFirstPool, mnOverflowPoolSize);
            APT_ASSERT(pPool->CanFitBytes(nSize));
            pReturn     = pPool->ConsumeBytes(nSize);
            mpFirstPool = pPool;
        }
        else
        {
            // whoever called Allocate will probably crash as soon as it tries to access this
            //  failed allocation.  However, when we're not in ship, we want to be a little more
            //  explicit to help people track down the issue (assuming they're forcing DOGMA to
            //  not create any overflow pools).
            //
            // NOTE: this is ultimately not what we want to do, as Ryan Burkett points out:
            //
            //    Yeah I think this is a good debugging technique. Long-term, though I think we might want to handle this differently.  The reason being that we traditionally don't use EA_SHIP in Apt (well OK it's in exactly one place in code, but it shouldn't be) because some titles don't define it.
            //
            //    Here are some options:
            //
            //    Staid
            //        - Add a new callback, gAptFuncts.pfnOverflowMemAlloc and a corresponding ea_aptaux_core AllocationContext enum (AllocationContext_AptOverflowMem?)
            //            - Then you can still crash, but it'll be outside of Apt code. Bonus: You can track overflow allocations more easily.
            //    Outlandish
            //        - Make Apt take a CoreAllocator interface instead of hard-coding Dogma.  Then you can use any allocator you want. Dogma could implement ICoreAllocator and be used by default.
            //        - Make one of the AptInitParms be a template typedef.  Same idea as CoreAllocator interface, but the allocator type will be compiled in.

            pReturn = NULL;

#if defined(APT_FORCE_CRASHES)
            // Ben B.  (4/18/2009)
            // If you want to enforce that your game does not create any overflow pools,
            //  you can set mnOverflowPoolSize to 0 and then you'll hit this crash if any pools
            //  would be created.  For Madden, at least, during game we never want to create
            //  any overflow pools (we'd rather have the memory reserved up front).
#if _MSC_VER
            __debugbreak();
#elif defined(__clang__) || defined(__GNUC__)
            __builtin_trap();
#else
            bool *willTrashMemory = (bool *)NULL;
            *willTrashMemory      = true;
#endif
#endif // APT_FORCE_CRASHES

            APT_ASSERT(false && "Attempting to allocate a overflow pool of 0 bytes");
        }

#if APT_DEBUG_LEVEL >= APT_DEBUG_NORMAL
        {
            if (pReturn)
            {
                memset(pReturn, 0xCD, nSize);
            }
        }
#endif

        return (pReturn);
    }
}

/**
 * frees a pointer that was allocated from this pool manager.
 * This just makes this the new head of the free list for that size of object.
 * @param pNowFree pointer, must have been allocated from this pool manager.
 * @param nAllocatedSize size of the object allocated.
 * @return bool - returns true if the item was released to the dogma pools internally returns false if the item was released directly to the heap.
 */
bool DOGMA_PoolManager::Deallocate(void *pNowFree, size_t nAllocatedSize)
{
    APT_ASSERT(pNowFree != NULL && "Attempting to Deallocate NULL pointer!")
    uint32_t nSize = static_cast<uint32_t>(ToNextValidSize(nAllocatedSize));

    mnItemsAllocated--; // One Less item is allocated

#if defined(DOGMA_EXTRA_MEMORY_COUNTERS)
    {
        if (nSize <= mnMaxSizeAllocation)
        {
            mpaAllocDataPerSize[nSize / DOGMA_ALLOCATION_GRANULARITY].nCurrentlyAllocated--;
        }
    }
#endif

    if (nSize > mnMaxSizeAllocation)
    {
        mnBytesAllocatedOutsideDogma -= nSize;
        if (!mbTrackOutsideAllocations)
        {
            AptGetUserFuncs().pfnMemFreeSize(pNowFree, nAllocatedSize);
        }
        else
        {
            nAllocatedSize += OutsideAllocationT::GetStructOverHead();

            OutsideAllocationT *poa = OutsideAllocationT::GetStructPointerFromReturnedPointer(pNowFree);

            if (poa->pNext)
                poa->pNext->pPrev = poa->pPrev;
            if (poa->pPrev)
                poa->pPrev->pNext = poa->pNext;

            if (mpFirstOutSideAllocation == poa)
                mpFirstOutSideAllocation = poa->pNext;

            AptGetUserFuncs().pfnMemFreeSize(poa, nAllocatedSize);
        }
        return false;
    }

#if APT_DEBUG_LEVEL >= APT_DEBUG_NORMAL
    {
        // Walk the pools, make sure that pointer is one of ours.
        DOGMA_MemPool *pPool = mpFirstPool;

        bool bFound = false;
        do
        {
            if (pPool->PtrIsInThisPool(pNowFree))
            {
                bFound = true;
                break;
            }

            pPool = pPool->GetNextPool();
        } while (pPool);

        APT_ASSERT(bFound && "Error! Deallocate recieved pointer to object that was not allocated here!");

        if (bFound == false)
        {
            // Crash me, this is bad.
#ifdef __GNUC__
            __builtin_trap();
#else
            unsigned int *ptr = NULL;
            *ptr              = 0xdeadbeef;
#endif // __GNUC__
        }
    }
#endif

#if APT_DEBUG_LEVEL >= APT_DEBUG_NORMAL
    {
        // lint --e(668) PcLint warning Warning 668: Possibly passing a null pointer to function 'memset(void *, int, unsigned int)', arg. no. 1 [Reference: file C:\MyWork\Perforce\mullet\sw10\code\mainline\ion\Apt\2.06.03-ion_dev\source\Apt\DogmaAllocator.cpp: line 671]
        memset(pNowFree, 0xFE, nSize);
    }
#endif

    mnBytesAllocatedInDogma -= nSize;

    // Add the block to the free pool for that size.
    AddFreeBlockBySize(pNowFree, nSize);

    return true;
}

/**
 * consumes (returns) a block from the free list of this size.
 * This internally updates the linked list of free items of that size.
 * @param nSize object size to pull.
 * @return void*             - pointer to the object
 */
void *DOGMA_PoolManager::ConsumeFreeBlockBySize(size_t nSize)
{
    size_t nArrayIndex = (nSize / DOGMA_ALLOCATION_GRANULARITY); // We have entries in our array for sizes % 4
    uintptr_t *pReturn;

    pReturn = mpaFirstFreeBySize[nArrayIndex];

    // Memory? We ain't got no stinkin' memory here.
    if (pReturn == NULL)
        return NULL;

    uintptr_t *pNextBuffer = (uintptr_t *)pReturn[mnOffsetToStoreNext];

    mpaFirstFreeBySize[nArrayIndex] = pNextBuffer;

    if (mbStoreFreeBlockSize)
    {
        APT_ASSERT(pReturn[mnOffsetToStoreSize] == (nSize));
    }

    // If Storing the size of the buffers
#if defined(DOGMA_EXTRA_MEMORY_COUNTERS)
    {
        mnBytesInFreeLists -= static_cast<uint32_t>(nSize);
        mnItemsFreed--; // one less item is in the free lists.
    }
#endif

    return (void *)pReturn;
}

/**
 * Adds the block to the free list of that sized objects.
 * Internally this updated the linked list of objects with that size. It also conditionally writes
 * requested data to the free area including the size of the free block, and or doubly links the
 * list.
 * @param pNowFree pointer to region that is being added to the free list
 * @param nSize size of the region being added.
 */
void DOGMA_PoolManager::AddFreeBlockBySize(void *pNowFree, size_t nSize)
{
    APT_ASSERT(pNowFree != NULL && "Attempting to Deallocate a NULL Pointer!");
    size_t nArrayIndex  = nSize / DOGMA_ALLOCATION_GRANULARITY;
    uintptr_t *pNewHead = (uintptr_t *)pNowFree;
    uintptr_t *pOldHead;

#if defined(DOGMA_EXTRA_MEMORY_COUNTERS)
    mnBytesInFreeLists += static_cast<uint32_t>(nSize);
    mnItemsFreed++; // One more item is in the Free Lists
#endif

    pOldHead                      = mpaFirstFreeBySize[nArrayIndex];
    pNewHead[mnOffsetToStoreNext] = (uintptr_t)pOldHead;

    // Be sure to put finish with the block before we link it into the list.
    if (mbStoreFreeBlockSize)
    {
        pNewHead[mnOffsetToStoreSize] = (nSize);
    }

    mpaFirstFreeBySize[nArrayIndex] = pNewHead;

    return;
}

/**
 * aligns and rounds up to the next valid size.
 * Basically This just enforces the minimum size requirements and alignes the request to a 4 byte
 * boundary.
 * @param nSize Size requested
 * @return size_t          - Validated / updated size.
 */
size_t DOGMA_PoolManager::ToNextValidSize(size_t nSize)
{
    // Align the block to the next 4-byte boundary.
    if (nSize & (DOGMA_ALLOCATION_GRANULARITY - 1))
    {
        nSize &= ~(DOGMA_ALLOCATION_GRANULARITY - 1);
        nSize += DOGMA_ALLOCATION_GRANULARITY;
    }

    if (nSize < mnMinimumAllocationSize)
    {
        nSize = mnMinimumAllocationSize;
    }
    return nSize;
}

/**
 * Returns the total size of memory used by its pools. Does not include any overhead, only object
 * allocations; the memory is probably split across multiple pools.
 * @return the number of bytes used.
 */
size_t DOGMA_PoolManager::GetTotalBytesUsed()
{
    // instead of looking for number of bytes used from pools and subtracting mnBytesInFreeLists lets just
    // return mnBytesAllocatedInDogma
    return (mnBytesAllocatedInDogma);
}

/**
 * Returns the total size of external memory allocations that are *NOT* part of the normal Apt
 * heap. Includes tracking overhead for external blocks.
 * @return the number of bytes used.
 */
size_t DOGMA_PoolManager::GetExternalBytesUsed()
{
    return (mnBytesAllocatedOutsideDogma);
}

/**
 * Returns the number of overflow pools present (ideally 0). Useful for debugging or metrics.
 * @return the number of overflow pools.
 */
int32_t DOGMA_PoolManager::GetNumOverflowPools()
{
    AptUpdateLock();
    int32_t count = 0;
    for (DOGMA_MemPool *pool = mpFirstPool; pool; pool = pool->GetNextPool())
    {
        ++count;
    }
    AptUpdateUnlock();
    return (count > 0) ? (count - 1) : 0; // don't count the main pool
}

/**
 * Returns the size of each overflow pool. Useful for debugging or metrics purposes.
 * @return the overflow pool size.
 */
size_t DOGMA_PoolManager::GetOverflowPoolSize() const
{
    return mnOverflowPoolSize;
}

#if defined(XBOX_FASTCAP)
/** Writes out a text file containing the maximum number of bytes (high water mark) used during execution. */
void DOGMA_PoolManager::WriteDynamicMemoryUsageFile(void)
{
    FILE *pFile = fopen("d:\\dynmem.txt", "wt");
    if (pFile)
    {
        // create buffer for output
        char szBuf[256];

        // total dynamic memory usage
        int nTotalDynamicMemory = 0;

        //////////////////////////////////////////////////////////////////////////
        // TODO: Change code below to work with DOGMA. This code works with SOA.
        //////////////////////////////////////////////////////////////////////////

        //// Loop through each pool and sum up high water points
        // for (int i = AptViewer::PoolParameters::MIN_BLOCK_SIZE;
        //     i <= AptViewer::PoolParameters::MAX_BLOCK_SIZE;
        //     i += EA::Allocator::DefaultPolicies::PoolParameters::MIN_BLOCK_SIZE)
        //{
        //     int nNumItems = gpSOA->GetNumItemsAllocated(i);
        //     if (nNumItems > 0)
        //     {
        //         //sprintf(szBuf, "Pool of size %4d: total dynamic memory used %d\n", i, gpSOA->GetHighwater(i)*nNumItems);
        //         //fputs(szBuf,pFile);
        //         nTotalDynamicMemory+=(gpSOA->GetHighwater(i)*nNumItems);
        //     }
        // }

        //// Output total memory usage data
        ////sprintf(szBuf, "Total dynamic memory used %d\n", nTotalDynamicMemory);
        // sprintf(szBuf, "%d\n", nTotalDynamicMemory); // keep it simple so it's easy to grep for the value
        // fputs(szBuf,pFile);

        // close file
        fflush(pFile);
        fclose(pFile);
    }
}
#endif
