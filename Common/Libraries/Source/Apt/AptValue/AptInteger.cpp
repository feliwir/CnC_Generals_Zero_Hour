#include "_Apt.h"
#include "AptValue/AptInteger.h"
#include "AptValue/AptValueVector.h"
#include "AptGlobal.h"
#include "MainInline.h"

AptInteger *AptInteger::spFirstFree = NULL;


void *AptInteger::operator new(size_t size)
{
    return APT_NONGC_NEW(size);
}

void AptInteger::operator delete(void *p, size_t size)
{
    APT_NONGC_DELETE(p, size);
}

void *AptInteger::operator new[](size_t size)
{
    return AptNonGCAllocSaveSize(APT_ALLOC_PARAMS(size));
}

void AptInteger::operator delete[](void *p)
{
    AptValueNoGC::VerifyAptValueNoGC();
    AptNonGCFreeSavedSize(p);
}

/** Recycles a pooled AptInteger if one is free, otherwise allocates a new one. */
AptInteger *AptInteger::Create(const int nValue)
{
#if APT_AUTOLOCK_VALUES
    AptUpdateAutoLock lock;
#else
    AptAssertIsSimulationThread();
#endif

#if APT_POOL_INTEGERS
    if (spFirstFree)
    {
        AptInteger *pNewInt = spFirstFree; // Re-read inside the lock.
        if (pNewInt)
        {
            APT_ASSERT(pNewInt->getVtblIndex() == AptVFT_Integer);
            APT_ASSERT(pNewInt->getRefCount() == 0);

            // May not have been deleted from the deferred vector yet; set the release-at-end flag.
            pNewInt->SetReleaseAtEnd();

            // Push onto the release vector so it gets recycled if possible.
            AptGetLib()->mpValuesToRelease->PushValue(pNewInt);

            spFirstFree = pNewInt->mpNextFree;

            pNewInt->mnValue = nValue;

            return (pNewInt);
        }
    }
#endif

    return (new AptInteger(nValue));
}

void AptInteger::Destroy()
{
#if APT_AUTOLOCK_VALUES
    AptUpdateAutoLock lock;
#endif

#if APT_POOL_INTEGERS
    {
        // Add to the free pool.
        mpNextFree  = spFirstFree;
        spFirstFree = this;
    }
#else
    {
        delete this;
    }
#endif
}

void AptInteger::ClearPool()
{
#if APT_POOL_INTEGERS
    {
        while (spFirstFree != NULL)
        {
            AptInteger *pNext = spFirstFree->mpNextFree;
            spFirstFree->DestroyGCPointers();
            delete spFirstFree; // The only place a real destruction happens.
            spFirstFree = pNext;
        }
    }
#endif
}
