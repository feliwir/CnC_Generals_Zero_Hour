#include "_Apt.h"
#include "AptValue/AptFloat.h"
#include "AptValue/AptValueVector.h"
#include "AptGlobal.h"
#include "MainInline.h"

AptFloat *AptFloat::spFirstFree = NULL;


/** Class-specific operator new; allocates via the non-garbage-collecting pool. */
void *AptFloat::operator new(size_t size)
{
    return APT_NONGC_NEW(size);
}

/** Class-specific operator delete. */
void AptFloat::operator delete(void *p, size_t size)
{
    APT_NONGC_DELETE(p, size);
}

/** Class-specific operator new[]. */
void *AptFloat::operator new[](size_t size)
{
    return AptNonGCAllocSaveSize(APT_ALLOC_PARAMS(size));
}

/** Class-specific operator delete[]. */
void AptFloat::operator delete[](void *p)
{
    AptValueNoGC::VerifyAptValueNoGC();
    AptNonGCFreeSavedSize(p);
}

/** Creates an AptFloat, reusing a pooled instance if the float pool is enabled. */
AptFloat *AptFloat::Create(const float fValue)
{
#if APT_AUTOLOCK_VALUES
    AptUpdateAutoLock lock;
#else
    AptAssertIsSimulationThread();
#endif

#if APT_POOL_FLOATS
    if (spFirstFree != NULL)
    {
        AptFloat *pNewFloat = spFirstFree; // Re-read inside the lock.
        if (pNewFloat)
        {
            //  The type should be float
            APT_ASSERT(pNewFloat->getVtblIndex() == AptVFT_Float);
            //  The refCount should be equal to zero
            APT_ASSERT(pNewFloat->getRefCount() == 0);
            // May not have been deleted from the deferred vector. Set Release At End flag.
            pNewFloat->SetReleaseAtEnd();

            //  Push the value on the release vector
            //  So it will be recycled if possible
            AptGetLib()->mpValuesToRelease->PushValue(pNewFloat);

            spFirstFree = pNewFloat->mpNextFree;

            pNewFloat->mfValue = fValue;

            return (pNewFloat);
        }
    }
#endif

    return (new AptFloat(fValue));
}

/** Either returns this instance to the pool or deletes it, depending on how we are configured. */
void AptFloat::Destroy()
{
#if APT_AUTOLOCK_VALUES
    AptUpdateAutoLock lock;
#endif

#if APT_POOL_FLOATS
    {
        //  Add to the free pool
        mpNextFree  = spFirstFree;
        spFirstFree = this;
    }
#else
    {
        delete this;
    }
#endif
}

/** Clears the float pool, if enabled. */
void AptFloat::ClearPool()
{
#if APT_POOL_FLOATS
    {
        while (spFirstFree != NULL)
        {
            AptFloat *pNext = spFirstFree->mpNextFree;
            spFirstFree->DestroyGCPointers();
            delete spFirstFree; //  The only place where we do a real destruction
            spFirstFree = pNext;
        }
    }
#endif
}
