/*** Include files ********************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include "_Apt.h" // Include after stdio and stdlib.
#include "Display/AptDisplayListState.h"
#include "AptFastStack.h"
#include "AptStd/AptMath.h"
#include "AptValue/AptValueVector.h"
#include "AptExtObject.h"
#include "AptFrameStack.h"
#include "MainInline.h"
#include "DogmaAllocator.h"
#include "AptValueGCAllocator.h"

/*** Defines **************************************************************************************/

/*** Variables*************************************************************************************/
uint8_t AptValueGC_PoolManager::snOffsetToStoreNext;
uint8_t AptValueGC_PoolManager::snOffsetToStoreSize;
uint8_t AptValueGC_PoolManager::snMinAllocation;
uint32_t AptValueGC_PoolManager::snMaxAllocation;

/*** Type Definitions *****************************************************************************/

/*** Functions ************************************************************************************/

AptValueGC *AptValueGC_PoolManager::AllocateAptValueGC(size_t nAllocatedSize)
{
    AptValueGC *pValue        = (AptValueGC *)DOGMA_PoolManager::Allocate(APT_ALLOC_PARAMS(nAllocatedSize));
    AptValueGC_MemItem *pItem = (AptValueGC_MemItem *)pValue;

#if APT_DEBUG_LEVEL >= APT_DEBUG_NORMAL
    // Make sure the shared bit is in the right location by toggling it and reading it back.
    pItem->SetIsAllocated(snOffsetToStoreSize, false);         // Clear as free item
    APT_ASSERT(pValue->mValueBitfield.mbIsAllocated == false); // Read as AptValue
#endif

    pItem->SetIsAllocated(snOffsetToStoreSize, true);         // Set Allocated as Free Item
    APT_ASSERT(pValue->mValueBitfield.mbIsAllocated == true); // Verify as AptValue

    return pValue;
}

void AptValueGC_PoolManager::DeallocateAptValueGC(AptValueGC *pNowFree, size_t nAllocatedSize)
{
#if defined(APT_DEBUG)
    AptValueGC_MemItem *pItem = (AptValueGC_MemItem *)pNowFree;

    // Verify that the Size being deallocated is actually the size of the AptValue object.
    APT_ASSERT(AptGetSizeOfAptValue(pNowFree) == nAllocatedSize && "MemFree was passed the wrong size for this object!");
    APT_ASSERT(pItem->IsAllocated(snOffsetToStoreSize) && "MemFree Was called on a value that was already deallocted!");
#endif // APT_DEBUG

    DOGMA_PoolManager::Deallocate(pNowFree, nAllocatedSize);

    // Code used to do some post processing, but that assumed the value wasn't re-allocated already!
    // This is obviously an error. Besides we were clearing the allocated flag, but since the
    // allocated flag is in the freed size, the bit should automatically be cleared then the size
    // is inserted.
}

#if defined(APT_ALLOCATION_TRACKING)

AptValueGC *AptValueGC_PoolManager::TrackedAllocateAptValueGC(size_t nAllocatedSize, const char *szFile, uint32_t nLine)
{
    AptValueGC *pValue        = (AptValueGC *)DOGMA_PoolManager::TrackedAllocate(nAllocatedSize, szFile, nLine);
    AptValueGC_MemItem *pItem = (AptValueGC_MemItem *)pValue;

#if APT_DEBUG_LEVEL >= APT_DEBUG_NORMAL
    // Make sure the shared bit is in the right location by toggling it and reading it back.
    pItem->SetIsAllocated(snOffsetToStoreSize, false);         // Clear as free item
    APT_ASSERT(pValue->mValueBitfield.mbIsAllocated == false); // Read as AptValue
#endif

    pItem->SetIsAllocated(snOffsetToStoreSize, true);         // Set Allocated as Free Item
    APT_ASSERT(pValue->mValueBitfield.mbIsAllocated == true); // Verify as AptValue

    return pValue;
}

void AptValueGC_PoolManager::TrackedDeallocateAptValueGC(AptValueGC *pNowFree, size_t nAllocatedSize)
{
    AptValueGC_MemItem *pItem = (AptValueGC_MemItem *)pNowFree;

    // Verify that the Size being deallocated is actually the size of the AptValue object.
    APT_ASSERT(AptGetSizeOfAptValue(pNowFree) == nAllocatedSize && "MemFree was passed the wrong size for this object!");
    APT_ASSERT(pItem->IsAllocated(snOffsetToStoreSize) && "MemFree Was called on a value that was already deallocted!");

    if (DOGMA_PoolManager::TrackedDeallocate(pNowFree, nAllocatedSize))
    {
        // Must do this after deallocation, because deallocation can over write it.
        pItem->SetIsAllocated(snOffsetToStoreSize, false);

#if APT_DEBUG_LEVEL >= APT_DEBUG_NORMAL
        // Verify that the Pool Manager set the size properly and that the AptValueGC_MemItem can read it out.
        uintptr_t nItemSize = pItem->GetSize(snOffsetToStoreSize);
        APT_ASSERT(nItemSize == nAllocatedSize);
#endif
    }
}

#endif // #if defined(APT_ALLOCATION_TRACKING)

AptValue *AptValueGC_PoolManager::GetNextAptValue(const AptValue *pPrevious)
{
    DOGMA_MemPool *pPool = mpFirstPool;

    // Find the pool we are in.
    do
    {
        if (pPool->PtrIsInThisPool(pPrevious))
            break;
    } while ((pPool = pPool->GetNextPool()) != NULL);

    // if pPool is null already, then the passed pointer was not allocated internally
    if (pPool == NULL)
    {
        return (AptValue *)GetNextOutsideAllocation(pPrevious);
    }
    else
    {
        // Start looking for the next Allocated Block (But First Advance Past this one.
        AptValueGC_MemItem *pItem = (AptValueGC_MemItem *)pPrevious;
        uintptr_t nItemSize;

        if (pItem->IsAllocated(snOffsetToStoreSize))
            nItemSize = AptGetSizeOfAptValue((AptValue *)pItem);
        else
            nItemSize = pItem->GetSize(snOffsetToStoreSize);
        // nItemSize = ToNextValidSize(nItemSize);
        nItemSize = static_cast<uint32_t>(ToNextValidSize(nItemSize));

        uintptr_t temp = ((uintptr_t)pItem) + nItemSize;
        pItem          = (AptValueGC_MemItem *)temp;

        if (!pPool->PtrIsInThisPool(pItem))
        {
            pPool = pPool->GetNextPool();
            if (pPool)
            {
                pItem = (AptValueGC_MemItem *)pPool->GetFirstItem();
            }
        }

        while (pPool != NULL)
        {
            // Start looking for the next item.
            while (pPool->PtrIsInThisPool(pItem))
            {

                uintptr_t nItemSize;
                if (pItem->IsAllocated(snOffsetToStoreSize))
                {
                    return (AptValue *)pItem;
                }
                else
                {
                    nItemSize = pItem->GetSize(snOffsetToStoreSize);
                }
                // nItemSize = ToNextValidSize(nItemSize);
                nItemSize      = static_cast<uint32_t>(ToNextValidSize(nItemSize));
                uintptr_t temp = ((uintptr_t)pItem) + nItemSize;
                pItem          = (AptValueGC_MemItem *)temp;
            }

            pPool = pPool->GetNextPool();
            if (pPool)
            {
                pItem = (AptValueGC_MemItem *)pPool->GetFirstItem();
            }
        }
    }

    // OK The passed pointer WAS an internal allocation, but it was the last one. Start passing back outside allocations
    return (AptValue *)GetFirstOutsideAllocation();
}

AptValue *AptValueGC_PoolManager::GetFirstAptValue()
{

    DOGMA_MemPool *pPool = mpFirstPool;

    do
    {

        AptValueGC_MemItem *pItem = (AptValueGC_MemItem *)pPool->GetFirstItem();

        while (pPool->PtrIsInThisPool(pItem))
        {
            uint32_t nItemSize;
            if (pItem->IsAllocated(snOffsetToStoreSize))
            {
                return (AptValue *)pItem;
            }
            else
            {
                nItemSize = pItem->GetSize(snOffsetToStoreSize);
            }

            uintptr_t temp = ((uintptr_t)pItem) + nItemSize;
            pItem          = (AptValueGC_MemItem *)temp;
        }

    } while ((pPool = pPool->GetNextPool()) != NULL);

    return (AptValue *)GetFirstOutsideAllocation();
}

AptValue **AptValueGC_PoolManager::GetAllAllocatedAptValues()
{
#if APT_USE_TEMPORARY_ALLOCATORS
    AptValue **pAptValuesArray = (AptValue **)AptGetUserFuncs().pfnTempAlloc(APT_ALLOC_PARAMS(mnItemsAllocated * sizeof(AptValue *)));
#else
    AptValue **pAptValuesArray = (AptValue **)AptGetUserFuncs().pfnMemAlloc(APT_ALLOC_PARAMS(mnItemsAllocated * sizeof(AptValue *)));
#endif
    APT_ASSERT(pAptValuesArray != NULL)

    // lint --e(668) PcLint warning Warning 668: Possibly passing a null pointer to function 'memset(void *, int, unsigned int)', arg. no. 1 [Reference: file C:\MyWork\Perforce\mullet\sw10\code\mainline\ion\Apt\2.06.03-ion_dev\source\Apt\AptValueGCAllocator.cpp: line 354]
    memset((void *)pAptValuesArray, 0, mnItemsAllocated * sizeof(AptValue *));
    // now walk thru all the pools and collect all the values in the array.

    int32_t nArrayIndex = 0;

    DOGMA_MemPool *pPool = mpFirstPool;
    do
    {

        AptValueGC_MemItem *pItem = (AptValueGC_MemItem *)pPool->GetFirstItem();

        while (pPool->PtrIsInThisPool(pItem))
        {
            uint32_t nItemSize;
            if (pItem->IsAllocated(snOffsetToStoreSize))
            {
                pAptValuesArray[nArrayIndex++] = (AptValue *)pItem;
                nItemSize                      = AptGetSizeOfAptValue((AptValue *)pItem);
            }
            else
            {
                nItemSize = pItem->GetSize(snOffsetToStoreSize);
            }
            // nItemSize = ToNextValidSize(nItemSize);
            nItemSize      = static_cast<uint32_t>(ToNextValidSize(nItemSize));
            uintptr_t temp = ((uintptr_t)pItem) + nItemSize;
            pItem          = (AptValueGC_MemItem *)temp;
        }

    } while ((pPool = pPool->GetNextPool()) != NULL);

    AptValue *pOutsideValues = (AptValue *)GetFirstOutsideAllocation();
    if (pOutsideValues != NULL)
    {
        while (pOutsideValues != NULL)
        {
            pAptValuesArray[nArrayIndex++] = (AptValue *)pOutsideValues;
            pOutsideValues                 = (AptValue *)GetNextOutsideAllocation(pOutsideValues);
        }
    }

    APT_ASSERT(nArrayIndex == (int32_t)mnItemsAllocated);

    return pAptValuesArray;
}

void AptValueGC_PoolManager::ReleaseAllocatedAptValuesArray(AptValue **pAptValuesArray, int32_t nItemsInArray)
{
    if (pAptValuesArray)
    {
#if APT_USE_TEMPORARY_ALLOCATORS
        AptGetUserFuncs().pfnTempFreeSize(pAptValuesArray, nItemsInArray * sizeof(AptValue *));
#else
        AptGetUserFuncs().pfnMemFreeSize(pAptValuesArray, nItemsInArray * sizeof(AptValue *));
#endif
    }
}

void AptValueGC_PoolManager::VerifyList()
{
    uint32_t nItemsFound = 0;

    AptValue *pValue = GetFirstAptValue();

    while (pValue != NULL)
    {
        nItemsFound++;
        pValue = GetNextAptValue(pValue);
    }

    APT_ASSERT(nItemsFound == mnItemsAllocated);
    return;
}

void AptValueGC_PoolManager::StaticInitialize()
{
    extern const uint8_t AptValueSizesByVType[APT_VALUESIZES_ARRAYSIZE];

    AptValue *pTemp = 0;

    // Gotta find out where that darn vtable is. (would use the offsetof macro, but it doesn't seem reliable across platforms...)
    if (/*lint -e(413) */ (uintptr_t)&pTemp->mnValueData - (uintptr_t)pTemp == 0)
    {
        // vtable is at the end, so shared dword is at the beginning
        snOffsetToStoreNext = sizeof(void *);
        snOffsetToStoreSize = 0;
    }
    else if (/*lint -e(413) */ (uintptr_t)&pTemp->mnValueData - (uintptr_t)pTemp == sizeof(void *))
    {
        // Vtable is at the beginning, if we want to share a dword with the apt value, we'll need to move up a little
        snOffsetToStoreNext = 0;
        snOffsetToStoreSize = sizeof(void *);
    }

    // Dynamically Set the Minimum and Maximum Size.
    uint32_t maxSizeSoFar = 0;
    uint32_t minSizeSoFar = 1000000;

    for (int i = (AptVFT_xxx + 1); i < AptVFT_NumVFTs; i++)
    {
        if (AptValueSizesByVType[i] > maxSizeSoFar)
        {
            maxSizeSoFar = AptValueSizesByVType[i];
        }

        if (AptValueSizesByVType[i] < minSizeSoFar)
        {
            minSizeSoFar = AptValueSizesByVType[i];
        }
    }

    snMaxAllocation = maxSizeSoFar;
    APT_ASSERT(minSizeSoFar < 0xFF);
    snMinAllocation = static_cast<uint8_t>(minSizeSoFar);
}
