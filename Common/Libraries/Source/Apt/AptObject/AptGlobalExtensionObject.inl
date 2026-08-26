#pragma once

/*** Include files ********************************************************************************/

#include "_Apt.h"
#include "AptObject/AptGlobalExtensionObject.h"

/** Sets pKey to pValue on the global extension object's native hash. */
void AptGlobalExtensionObject::Set(const AptNativeString *const pKey, AptValue *const pValue)
{
    mNativeHash.Set(pKey, pValue);
}

/** Removes pKey from the global extension object's native hash. */
void AptGlobalExtensionObject::UnSet(const AptNativeString *const pKey)
{
    mNativeHash.Unset(pKey);
}

/** Looks up pKey in the global extension object's native hash. */
AptValue *AptGlobalExtensionObject::Lookup(const AptNativeString *const pKey)
{
    return (mNativeHash.Lookup(pKey));
}
