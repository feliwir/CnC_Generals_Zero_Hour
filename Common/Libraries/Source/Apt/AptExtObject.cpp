#include "_Apt.h"
#include "MainInline.h"
#include "AptExtObject.h"

#include APT_INC_THREAD_H
#include APT_INC_THREAD_MUTEX_H


extern void AptUpdateLock();
extern void AptUpdateUnlock();

void *AptExtObject::operator new(size_t size)
{
    AptExtObject *retValue = 0;
    AptUpdateLock();
    retValue               = (AptExtObject *)APT_GC_NEW(size);
    retValue->mnObjectSize = static_cast<uint32_t>(size); // Safe to fill in here.
    AptUpdateUnlock();
    return retValue;
}

/**
 * Registers a user AptNativeFunction by name in the extension object's hash table; used by the
 * APTEXT_FUNCTION_INIT macro.
 */
void AptExtObject::SetFunction(const char *pKey, AptNativeFunction *pFunction)
{
    AptUpdateLock();
    AptNativeString pTmpStr(pKey);
    mpNativeHash->Set(&pTmpStr, (AptValue *)pFunction);
    AptUpdateUnlock();
}

/**
 * @param iNumMembers initial hash table size; the table can grow, but should ideally be sized to
 * the number of C++ functions the object will have.
 */
AptExtObject::AptExtObject(const int32_t iNumMembers)
    : AptValueGC(AptVFT_Extension)
{
    // mnObjectSize is deliberately not initialized here -- operator new already did it.
    APT_ASSERT(mnObjectSize != 0x00000000);
    APT_ASSERT(mnObjectSize != 0xcdcdcdcd); // Dogma fills freshly allocated memory with 0xcd; this
                                            // firing usually means multiple inheritance, or a
                                            // subclass defining its own new/delete (both unsupported).

    AptUpdateLock();
    mpNativeHash = new AptNativeHash(iNumMembers);

    // Extension objects shouldn't be placed on the deferred vector -- unclear what thread deletes them.
    SetAllowDelayedDeletion(false);
    AptUpdateUnlock();
}

AptExtObject::~AptExtObject()
{
    AptUpdateLock();
    delete mpNativeHash;
    AptUpdateUnlock();
}

/**
 * @param pContext ignored
 * @param pName variable name to look up
 * @return the variable, or NULL if not found
 */
AptValue *AptExtObject::objectMemberLookup(AptValue *const pContext, const AptNativeString *const pName) const
{
    AptUpdateLock();
    AptValue *pVal = (mpNativeHash->Lookup(pName));
    AptUpdateUnlock();
    return pVal;
}

/**
 * Sets a variable in the hash table if it doesn't already exist.
 * @return always true: even when the variable isn't set, that's the intended behavior.
 */
bool AptExtObject::objectMemberSet(AptValue *const pContext, const AptNativeString *const pName, AptValue *const pValue)
{
    AptUpdateLock();
    mpNativeHash->SetIfNotExists(pName, pValue);
    AptUpdateUnlock();
    return true;
}

AptNativeHash *AptExtObject::GetNativeHashVirtual()
{
    return (mpNativeHash);
}

bool AptExtObject::ContainsNativeHashVirtual() const
{
    return (true);
}

void AptExtObject::RegisterReferences()
{
    AptUpdateLock();
    if (!APT_REFERENCES_REGISTERED(this))
    {
        // Not defined further up the inheritance tree right now, so not calling the parent version.
        mpNativeHash->RegisterReferences(this);
    }
    AptUpdateUnlock();
}

void AptExtObject::DestroyGCPointers()
{
    AptUpdateLock();
    AptValueGC::DestroyGCPointers();
    mpNativeHash->DestroyGCPointers();
    AptUpdateUnlock();
}

AptValue *AptExtObject::Lookup(const AptNativeString *const pKey) const
{
    AptUpdateLock();
    AptValue *pVal = mpNativeHash->Lookup(pKey);
    AptUpdateUnlock();
    return pVal;
}

void AptExtObject::Set(const AptNativeString *const pKey, AptValue *const pValue)
{
    AptUpdateLock();
    mpNativeHash->SetIfNotExists(pKey, pValue);
    AptUpdateUnlock();
}

/**
 * Apt C++ extension functions receive pThis (usually the AptExtObject containing the function)
 * and nParams. GetParam retrieves the parameter at iParam from the interpreter's stack; an
 * out-of-range index yields an invalid pointer.
 */
AptValue *AptExtObject::GetParam(const int32_t iParam)
{
    APT_ASSERT(iParam >= 0);
    AptUpdateAutoLock lock;
    return (gAptActionInterpreter.stackAt(iParam));
}

AptValue *AptExtObject::GetUndefinedValue(void)
{
    return (gpUndefinedValue);
}

/** @return a new empty AptObject with the default hash table size. */
AptValue *AptExtObject::CreateNewAptObject(void)
{
    AptUpdateAutoLock lock;
    return (new AptObject(AptVFT_Object));
}

/** @param iHashSize initial hash table size for the new object. */
AptValue *AptExtObject::CreateNewAptObject(const int32_t iHashSize)
{
    AptUpdateAutoLock lock;
    return (new AptObject(AptVFT_Object, iHashSize));
}

/** Used by the APTEXT_FUNCTION_INIT macro to wrap a C++ function as an AptNativeFunction. */
AptNativeFunction *AptExtObject::CreateNewAptFunction(AptExtFunctionPtr pAptExtFnc)
{
    AptUpdateAutoLock lock;
    return (new AptNativeFunction((AptNativeFunctionPointer)pAptExtFnc));
}

/**
 * @param pContext scope to look the variable up in
 * @return the variable, or NULL if not found or undefined
 */
AptValue *AptExtObject::GetVariable(AptValue *pContext, const AptNativeString *const pVariable)
{
    AptUpdateAutoLock lock;
    AptValue *pValue = gAptActionInterpreter.getVariable(pContext, NULL, pVariable, true, false, true);
    if (pValue && pValue->getIsDefined())
    {
        return pValue;
    }
    return NULL;
}

/**
 * @param pContext scope to set the variable in
 * @return true if the variable was set
 */
bool AptExtObject::SetVariable(AptValue *pContext, const AptNativeString *const pName, AptValue *const pValue)
{
    AptUpdateAutoLock lock;
    return (gAptActionInterpreter.setVariable(pContext, NULL, pName, pValue, true, false, true));
}
