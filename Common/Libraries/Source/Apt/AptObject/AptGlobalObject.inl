#include "_Apt.h"
#include "AptObject/AptGlobalObject.h"

void AptGlobal::Set(const AptNativeString *const pKey, AptValue *const pValue)
{
    mNativeHash.Set(pKey, pValue);
}

AptValue *AptGlobal::Lookup(const AptNativeString *const pKey) const
{
    return (mNativeHash.Lookup(pKey));
}

/** Handles built-in _global members; only setInterval/clearInterval for now. */
AptValue *AptGlobal::Lookup(int idx) const
{
    switch (idx)
    {
    case AptGlobal::setIntervalIdx:
    {
        return gpCBsetInterval;
    }
    case AptGlobal::clearIntervalIdx:
    {
        return gpCBclearInterval;
    }
    default:
    {
        break;
    }
    }

    return NULL;
}
