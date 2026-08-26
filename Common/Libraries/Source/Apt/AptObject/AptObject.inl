#include "AptObject/AptObject.h"
#include "AptNativeHash.h"

void AptValueWithHash::Set(const AptNativeString *const pKey, AptValue *const pValue)
{
    mNativeHash.Set(pKey, pValue);
}

AptValue *AptValueWithHash::Lookup(const AptNativeString *const pKey) const
{
    return (mNativeHash.Lookup(pKey));
}

void AptObject::Set__Proto__(AptValue *const pValue)
{
    mNativeHash.Set__Proto__(pValue);
}

void AptObject::SetPrototype(AptValue *const pValue)
{
    mNativeHash.SetPrototype(pValue);
}
