#include "AptValue/AptBoolean.h"
#include "_AptInternalDefines.h"
#include "MainInline.h"

#if !defined(APT_ENABLE_INLINE)
#include "AptValue/AptBoolean.inl"
#endif

AptBoolean *AptBoolean::spBooleanFalse = NULL;
AptBoolean *AptBoolean::spBooleanTrue  = NULL;


void *AptBoolean::operator new(size_t size)
{
    return APT_NONGC_NEW(size);
}

void AptBoolean::operator delete(void *p, size_t size)
{
    APT_NONGC_DELETE(p, size);
}

void *AptBoolean::operator new[](size_t size)
{
    return AptNonGCAllocSaveSize(APT_ALLOC_PARAMS(size));
}

void AptBoolean::operator delete[](void *p)
{
    AptValueNoGC::VerifyAptValueNoGC();
    AptNonGCFreeSavedSize(p);
}

/** Sets up the global true/false AptBoolean singletons. */
void AptBoolean::Initialize()
{
    if (spBooleanFalse == NULL || spBooleanTrue == NULL)
    {
        spBooleanFalse = new AptBoolean(false);
        spBooleanTrue  = new AptBoolean(true);
    }
}

/**
 * Factory function returning the shared AptBoolean singleton for @p value.
 * @return the corresponding AptBoolean
 */
AptBoolean *AptBoolean::Create(const bool value)
{
    return value ? spBooleanTrue : spBooleanFalse;
}

/** Deletes the global true/false AptBoolean singletons. */
void AptBoolean::Shutdown()
{
    if (spBooleanFalse != NULL)
    {
        spBooleanFalse->ForceDelete();
        spBooleanFalse = NULL;
    }
    if (spBooleanTrue != NULL)
    {
        spBooleanTrue->ForceDelete();
        spBooleanTrue = NULL;
    }
}
