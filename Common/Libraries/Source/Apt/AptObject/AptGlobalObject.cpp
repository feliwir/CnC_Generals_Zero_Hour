#include "_Apt.h"
#include "AptObject/AptGlobalObject.h"
#include "AptObject/AptGlobalExtensionObject.h"
#include "MainInline.h"
#include "AptObjects.h"

#if !defined(APT_ENABLE_INLINE)
#include "AptObject/AptGlobalObject.inl"
#endif



/**
 * @param pContext not used
 * @param pName member name
 * @return the member if found; otherwise NULL
 */
AptValue *AptGlobal::objectMemberLookup(AptValue *const pContext, const AptNativeString *const pName) const
{
    APT_ASSERT(this == AptGetLib()->mpGlobalGlobalObject);

    AptValue *pValue = NULL;

    // look up built-in global members before looking in hashtable
    Objects *pObjects = ObjectIndex::in_word_set(pName->c_str(), pName->Size());
    if (pObjects)
    {
        pValue = Lookup(pObjects->nIndex);
    }

    if (!pValue)
    {
        pValue = AptGetLib()->mpGlobalExtensionObject->Lookup(pName);

        if (pValue && (pValue->isUndefined() == false))
        {
            return (pValue);
        }
        pValue = AptGetLib()->mpGlobalGlobalObject->Lookup(pName);
    }
    return (pValue);
}

bool AptGlobal::objectMemberSet(AptValue *const pContext, const AptNativeString *const pName, AptValue *const pValue)
{
    APT_ASSERT(this == AptGetLib()->mpGlobalGlobalObject);

    if (AptGetLib()->mpGlobalExtensionObject->Lookup(pName))
    {
        // The member exists in the extension object; don't override it, just look it up.
        return (true);
    }

    Set(pName, pValue);
    return (true);
}

/**
 * AptObjects.h (gperf-generated) can only be included from one .cpp, so both AptGlobal and
 * AptValue::findChild go through this to look up a gperf index by name.
 * @return the gperf index if name is in objects.gperf, otherwise -1.
 */
int AptGlobal::GetGperfIndex(const AptNativeString *const name) const
{
    int idx           = -1;
    Objects *pObjects = ObjectIndex::in_word_set(name->c_str(), name->Size());
    if (pObjects)
    {
        idx = pObjects->nIndex;
    }
    return idx;
}
