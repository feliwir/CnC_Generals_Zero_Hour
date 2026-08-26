/**
 * The Apt Value GC Allocator uses the dogma allocator architecture to trace AptValue-derived
 * objects for garbage collection and debug/informational purposes, letting the list of allocated
 * AptValueGC objects be walked at any time without maintaining a separate linked list (which cost
 * 8 bytes per object). This couples the allocator somewhat tightly to AptValue, so changes to
 * AptValue's layout can break it.
 *
 * The pool manager walks each pool from its base, reading the first 4 bytes of each block. A bit
 * reserved in AptValue (never touched by AptValue itself) tells the allocator whether that block
 * is allocated or free. If allocated, AptGetSizeOfAptValue() gives the object's size (and so the
 * location of the next block); if free, the free block stores its own size. Either way, the next
 * block's start is known, so every allocated value can be visited in turn (e.g. by the garbage
 * collector). Walking blocks this way is also cache-friendly, since it moves linearly through
 * memory rather than following pointers randomly.
 */

#pragma once
/*** Include files ********************************************************************************/
#include "AptDefine.h"

#include "DogmaAllocator.h"
#include "Apt.h"
#include "AptValue/AptValue.h"

/*** Defines **************************************************************************************/

/*** Macros ***************************************************************************************/

/*** Type Definitions *****************************************************************************/
class AptValueGC; // forward Declaration

/*** Functions ************************************************************************************/

/**
 * Extension to the DOGMA_PoolManager which is designed for and aware of AptValueGC The benefit of making this class aware of both objects is that we can give it enough knowledge to be able to walk the pools of memory for garbage collection purposes. Since we reserve a bit in the free memory that is also reserved in AptValue we can start at the begining of each pool and see if the pointer is allocated and what size it is (to get to the next pointer).
 */
class AptValueGC_PoolManager : public DOGMA_PoolManager
{
  public:
    static void *operator new(size_t size)
    {
        return AptGetUserFuncs().pfnMemAlloc(APT_ALLOC_PARAMS(size));
    }
    static void operator delete(void *p, size_t size)
    {
        AptGetUserFuncs().pfnMemFreeSize(p, size);
    }
    static void *operator new[](size_t size)
    {
        return AptGetUserFuncs().pfnMemAlloc(APT_ALLOC_PARAMS(size));
    }
    static void operator delete[](void *p)
    {
        AptGetUserFuncs().pfnMemFree(p);
    }

    AptValueGC_PoolManager(uint32_t mainPoolSizeBytes, uint32_t overflowPoolSizeBytes) : DOGMA_PoolManager(mainPoolSizeBytes,
                                                                                                           overflowPoolSizeBytes,
                                                                                                           snMinAllocation, snMaxAllocation, snOffsetToStoreNext,
                                                                                                           true, snOffsetToStoreSize,
                                                                                                           true /* Be sure to track outside allocations */)
    {
    }

    AptValueGC *AllocateAptValueGC(size_t nAllocatedSize);
    void DeallocateAptValueGC(AptValueGC *pNowFree, size_t nAllocatedSize);

#if defined(APT_ALLOCATION_TRACKING)
    AptValueGC *TrackedAllocateAptValueGC(size_t nAllocatedSize, const char *szFile, uint32_t nLine);
    void TrackedDeallocateAptValueGC(AptValueGC *pNowFree, size_t nAllocatedSize);
#endif

    AptValue *GetNextAptValue(const AptValue *pPrevious);
    AptValue *GetFirstAptValue();
    void VerifyList();

    AptValue **GetAllAllocatedAptValues();
    void ReleaseAllocatedAptValuesArray(AptValue **pAptValuesArray, int32_t nItemsInArray);

    static void StaticInitialize();

  protected:
    // Variables setup by StaticInitialize
    static uint8_t snOffsetToStoreNext;
    static uint8_t snOffsetToStoreSize;
    static uint8_t snMinAllocation;
    static uint32_t snMaxAllocation;

  private:
    // Here are a few operators not allowed on this object, they are put in private scope to slap people who try. (They are not implemented)
    AptValueGC_PoolManager &operator=(const AptValueGC_PoolManager &r);
};

/**
 * Utility object used to make AptValueGC_PoolManager more readable, giving anonymous pool memory an
 * interface. Relies on a bit reserved in the "size" field of the free memory block matching up with
 * the bit reserved for this purpose in AptValue.
 */
using AptValueGC_MemItem = struct _AptValueGC_MemItem
{
    union
    {

        struct
        {
            struct _AptValueGC_MemItem *pNextItem;
            uintptr_t bIsAllocated : 1;
#if APT_PLATFORM_PTR_SIZE == 4
            uintptr_t nSize : 31;
#elif APT_PLATFORM_PTR_SIZE == 8
            uintptr_t nSize : 63;
#else
#error "Don't know what to do here"
#endif
            struct _AptValueGC_MemItem *pPrevItem;
        } Type1;

        struct
        {
            uintptr_t bIsAllocated : 1;
#if APT_PLATFORM_PTR_SIZE == 4
            uintptr_t nSize : 31;
#elif APT_PLATFORM_PTR_SIZE == 8
            uintptr_t nSize : 63;
#else
#error "Don't know what to do here"
#endif
            struct _AptValueGC_MemItem *pNextItem;
            struct _AptValueGC_MemItem *pPrevItem;
        } Type2;
    };

    /**
     * Looks at the bit agreed (shared with AptValue) that tells us if the memory block we are
     * looking at is allocated.
     * Insert your long description here.
     * @param nSizeOffset how many bytes into the block should we look?
     * @return bool              - true = it is allocated and occupied by an aptvalue.
     */
    APT_INLINE bool IsAllocated(uint32_t nSizeOffset)
    {
        if (nSizeOffset == sizeof(void *))
        {
            return Type1.bIsAllocated ? true : false;
        }
        else if (nSizeOffset == 0)
        {
            return Type2.bIsAllocated ? true : false;
        }
        else
        {
            APT_FAIL("bad nSizeOffset");
            return false;
        }
    }

    /**
     * modify the AptValue approved "Allocated" bit to the given new value.
     * @param nSizeOffset Offset into the block to look for our bit.
     * @param newValue value to put there. true = allocated
     */
    void SetIsAllocated(uint32_t nSizeOffset, bool newValue)
    {
        if (nSizeOffset == sizeof(void *))
        {
            Type1.bIsAllocated = (uint32_t)(newValue ? 1 : 0);
        }
        else if (nSizeOffset == 0)
        {
            Type2.bIsAllocated = (uint32_t)(newValue ? 1 : 0);
        }
        else
        {
            APT_FAIL("bad nSizeOffset");
        }
    }

    /**
     * return the size of a free block.
     * [idiot disclaimer] The pointer must be to an free block of memory
     * @param nSizeOffset Output     uint32_t GetSize  -
     * @return uint32_t GetSize  -
     */
    APT_INLINE uintptr_t GetSize(uint32_t nSizeOffset)
    {
        APT_ASSERT(!IsAllocated(nSizeOffset));

        if (nSizeOffset == sizeof(void *))
        {
// Big endian systems put the size at the other end of the word.
#if defined(APT_SYSTEM_BIG_ENDIAN)
            return Type1.nSize;
#else
            return Type1.nSize << 1;
#endif
        }
        else if (nSizeOffset == 0)
        {
// Big endian systems put the size at the other end of the word.
#if defined(APT_SYSTEM_BIG_ENDIAN)
            return Type2.nSize;
#else
            return Type2.nSize << 1;
#endif
        }
        else
        {
            APT_FAIL("bad nSizeOffset");
            return 0;
        }
    }
};
