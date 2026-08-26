#pragma once

/*** Include files ********************************************************************************/
#include <cstring>
#include <cstdint>

/*** Defines **************************************************************************************/

/*** Macros ***************************************************************************************/
// Allocation granularity doubles as the alignment guarantee of every pool
// allocation, so it must be at least the pointer size: serialized Apt data and
// the action interpreter (_TONEXTALIGNED) rely on pointer-aligned blocks.
#define DOGMA_ALLOCATION_GRANULARITY sizeof(uintptr_t)
/*** Type Definitions *****************************************************************************/

/*** Functions ************************************************************************************/

/**
 * Manages data associated with a pool, all required maintenance variables are placed within the pool.
 */
using DOGMA_MemPool = struct _DOGMA_MemPool
{

    /**
     * This setups the given block of memory as a pool to be allocated from.
     * Since this is actually a large block of memory we are making into a Mem pool, we don't have a
     * constructor, This serves the purpose of initializing the pseudo object.
     * @param pNextPool -
     * @param nSize -
     */
    APT_INLINE void SetupPool(struct _DOGMA_MemPool *pNextPool, size_t nSize)
    {
        mnPoolFree = mnPoolSize = (nSize - sizeof(struct _DOGMA_MemPool) + 1);
        mpNextPool              = pNextPool;
    }

    struct _DOGMA_MemPool *GetNextPool() const;

    /**
     * returns true if the number of bytes passed will fit in the unused portion of this pool.
     * @param nBytes Number of bytes we will use.
     * @return bool   - true = will fit.
     */
    APT_INLINE bool CanFitBytes(size_t nBytes) const
    {
        return (mnPoolFree >= nBytes);
    }

    /**
     * consumes the passed number of bytes from the unused portion of the pool.
     * @param nBytes Number of bytes to use.
     * @return void* - pointer to allocated chunk of memory.
     */
    APT_INLINE void *ConsumeBytes(size_t nBytes)
    {
        size_t mOldFree = mnPoolFree;
        mnPoolFree -= nBytes;
        return &(&mPoolStart)[mnPoolSize - mOldFree];
    }

    /**
     * Return a pointer to the first byte of allocatable pool memory.
     * Warning: this does not know if that chunk is allocated or not it is up to the caller to know
     * what they are going to do with it and be sure they do not corrupt memory.
     * @param None .
     * @return void* - pointer to first byte of allocatable pool memory.
     */
    APT_INLINE void *GetFirstItem()
    {
        return &mPoolStart;
    }

    /**
     * returns true if the pointer passed is within this pools allocatable region.
     * This can be used to find out which pool a pointer belongs to.
     * @param None .
     * @return bool              -
     */
    APT_INLINE bool PtrIsInThisPool(const void *ptr) const
    {
        if (ptr >= &mPoolStart && ptr < &((&mPoolStart)[mnPoolSize - mnPoolFree]))
            return true;
        else
            return false;
    }

    /**
     * Returns the number of bytes that are used in this pool.
     * This does not include pool management overhead or unused space.
     * @param None .
     * @return APT_INLINE size_t  -
     */
    APT_INLINE size_t GetBytesUsed()
    {
        return mnPoolSize - mnPoolFree;
    }

  protected:
    struct _DOGMA_MemPool *mpNextPool;                        // Pointer to the next pool.
    size_t mnPoolSize;                                        // Size in Bytes of the allocatable portion of the pool
    size_t mnPoolFree;                                        // Number of bytes left free in the pool
    alignas(DOGMA_ALLOCATION_GRANULARITY) uint8_t mPoolStart; // Placeholder, used for its address as the base of the allocatable pool memory.
};

using AllocationDataPerSize = struct _AllocationDataPerSize
{
    uint32_t nAllocationCount;
    size_t nCurrentlyAllocated;
    size_t nMaxAllocatedAtAnyOneTime;
};

using TrackedAllocation = struct _TrackedAllocation
{
    void *pAllocated;
    uint32_t nLineNumber;
    const char *szFileName;
    size_t nAllocSize;
};

/**
 * Quickly Manages allocation pools of variably sized objects.
 */
class DOGMA_PoolManager
{
  public:
    static void *operator new(size_t size);
    static void operator delete(void *p, size_t size);
    static void *operator new[](size_t size);
    static void operator delete[](void *p);

    DOGMA_PoolManager(size_t mainPoolSizeBytes,
                      size_t overflowPoolSizeBytes,
                      size_t minSizeAllocation,
                      size_t maxSizeAllocation,
                      uint8_t nOffsetToStoreNextInFreeItem,
                      bool bStoreFreeBlockSize, uint8_t nOffsetToStoreSizeInFreeItem,
                      bool bTrackOusideAllocations);

    ~DOGMA_PoolManager();

#if defined(APT_ALLOCATION_TRACKING)
    void *Allocate(size_t nAllocatedSize, const char *szAllocFile, uint32_t nLineNumber);
#else
    void *Allocate(size_t nAllocatedSize);
#endif
    bool Deallocate(void *pNowFree, size_t nAllocatedSize);

    size_t GetTotalBytesUsed();
    size_t GetExternalBytesUsed();
    int32_t GetNumOverflowPools();

    void *GetFirstOutsideAllocation();
    void *GetNextOutsideAllocation(const void *pCurrent);

    size_t GetOverflowPoolSize() const;

  protected:
    void *ConsumeFreeBlockBySize(size_t nSize);
    void AddFreeBlockBySize(void *pNowFree, size_t nSize);
    size_t ToNextValidSize(size_t nSize);

    uintptr_t **mpaFirstFreeBySize; // Base of Array where each sizes' first free pointers is stored.

    DOGMA_MemPool *mpFirstPool; // First Mem Pool.

    const size_t mnOverflowPoolSize; // Pools of this size will be allocated whenever allocation is made that will not fit in the main pool.
                                     // The overflow pool should be small and if things are setup right then we should need them very often.

    const size_t mnMaxSizeAllocation; // Maximum size this allocator will handle.

    uint32_t mnOffsetToStoreNext;       // Offset in free memory blocks to store the pointer to the next free block of the same size.
    uint32_t mnOffsetToStoreSize;       // Offset in free memory blocks to store the size of the free block (only used if mbStoreFreeBlockSize is true)
    uint32_t mnMinimumAllocationSize;   // minimum allocation size possible with this configuration
    uint32_t mbStoreFreeBlockSize;      // 1 = store sizeof free block within the block, There is no memory overhead for this.
    uint32_t mbTrackOutsideAllocations; // 1 = Keep track of all allocations that go directly to the heap.

    struct OutsideAllocationT
    {
      public:
        OutsideAllocationT *pNext;
        OutsideAllocationT *pPrev;
        // size_t (not uint32_t) so the header is a whole number of pointers and
        // the returned memory keeps the allocator's pointer-alignment guarantee
        size_t nSize;

        APT_INLINE void *GetReturnedPointer()
        {
            return &returnedMemory[0];
        }

        APT_INLINE static size_t GetStructOverHead()
        {
            return 2 * sizeof(OutsideAllocationT *) + sizeof(size_t);
        }

        APT_INLINE static OutsideAllocationT *GetStructPointerFromReturnedPointer(const void *pReturned)
        {
            return (OutsideAllocationT *)(((uint8_t *)pReturned) - GetStructOverHead());
        }

      private:
        // Gives us something to get the pointer from... Should not be messed with.
        uint8_t returnedMemory[1];
    };

    OutsideAllocationT *mpFirstOutSideAllocation;

  public:
    uint32_t mnBytesAllocatedInDogma;
    uint32_t mnBytesAllocatedOutsideDogma;
    uint32_t mnItemsAllocated; // Count of Allocated Items

#if defined(DOGMA_EXTRA_MEMORY_COUNTERS)
    uint32_t mnBytesInFreeLists;
    uint32_t mnItemsFreed; // Count of Allocated Items
    AllocationDataPerSize *mpaAllocDataPerSize;
#endif

#if defined(APT_VERIFY_NON_GC_ALLOC_SIZES)
    void *SizeVerifyAllocate(size_t nAllocatedSize);
    bool SizeVerifyDeallocate(void *pNowFree, size_t nAllocatedSize);
#endif

#if defined(APT_ALLOCATION_TRACKING)
    void *TrackedAllocate(size_t nAllocatedSize, const char *szAllocFile, uint32_t nLineNumber);
    bool TrackedDeallocate(void *pNowFree, size_t nAllocatedSize);
    void RegisterTrackedAllocations(void (*pRegFunc)(TrackedAllocation *pTracker));

    size_t mnTrackThisSize;
    static const size_t TRACK_SIZE_ALL  = (size_t)-1;
    static const size_t TRACK_SIZE_NONE = 0;

    TrackedAllocation *mpTrackers;
    uint32_t mnTrackers;
#endif

#if defined(XBOX_FASTCAP)
    void WriteDynamicMemoryUsageFile(void);
#endif

  private:
    // Here are a few operators not allowed on this object, they are put in private scope to slap people who try. (They are not implemented)
    DOGMA_PoolManager &operator=(const DOGMA_PoolManager &r);
};
