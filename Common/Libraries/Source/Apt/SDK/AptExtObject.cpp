/**
 * @addtogroup g_referenceguide        Reference Guides
 */
//@{
/**
 * @brief AptSDK: Apt Extension SDK
 */
//@}

/*** Include files ********************************************************************************/

#include "_Apt.h"
#include "MainInline.h"
#include "AptExtObject.h"

#include APT_INC_THREAD_H
#include APT_INC_THREAD_MUTEX_H

/*** Defines **************************************************************************************/

/*** Macros ***************************************************************************************/

/*** Type Definitions *****************************************************************************/

/*** Function Prototypes **************************************************************************/

/*** Variables ************************************************************************************/

// Private variables

// Public variables

extern void AptUpdateLock();
extern void AptUpdateUnlock();

/*** Private Functions ****************************************************************************/

void *AptExtObject::operator new(size_t size)
{
    AptExtObject *retValue = 0;
    AptUpdateLock();
    retValue               = (AptExtObject *)APT_GC_NEW(size);
    retValue->mnObjectSize = static_cast<uint32_t>(size); // Fill this in here. It should be safe.
    AptUpdateUnlock();
    return retValue;
}

void AptExtObject::SetFunction(const char *pKey, AptNativeFunction *pFunction)
{
    AptUpdateLock();
    AptNativeString pTmpStr(pKey);
    mpNativeHash->Set(&pTmpStr, (AptValue *)pFunction);
    AptUpdateUnlock();
}

/*** Public Functions *****************************************************************************/

AptExtObject::AptExtObject(const int32_t iNumMembers)
    : AptValueGC(AptVFT_Extension)
{
    // lint -esym(1402, AptExtObject::mnObjectSize)
    // lint -esym(1401, AptExtObject::mnObjectSize)
    APT_ASSERT(mnObjectSize != 0x00000000);
    APT_ASSERT(mnObjectSize != 0xcdcdcdcd); // Dogma initializes allocated memory to cd's.
                                            // This assert typically will fire if AptExtObject is using
                                            // multiple inheritance (bad) or if an AptExtObject Based
                                            // class defines it's own new delete operators (also bad).
                                            // Feel free to contact the Apt team for clarification on this.

    AptUpdateLock();
    mpNativeHash = new AptNativeHash(iNumMembers);

    // don't let Ext objects be placed on the deferred vector. Not sure what thread they are deleted in.
    SetAllowDelayedDeletion(false);
    AptUpdateUnlock();
}

AptExtObject::~AptExtObject()
{
    AptUpdateLock();
    delete mpNativeHash;
    AptUpdateUnlock();
}

AptValue *AptExtObject::objectMemberLookup(AptValue *const pContext, const AptNativeString *const pName) const
{
    AptUpdateLock();
    AptValue *pVal = (mpNativeHash->Lookup(pName));
    AptUpdateUnlock();
    return pVal;
}

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
        // Right now This function is not defined up the inheritance tree.
        // So we aren't calling it. if at some point in time the function is implementes at the
        // base class level we need to uncomment this line.
        // AptValueGC::RegisterReferences();

        mpNativeHash->RegisterReferences(this);
    }
    AptUpdateUnlock();
}

void AptExtObject::DestroyGCPointers()
{
    AptUpdateLock();
    // Go up the Chain.
    AptValueGC::DestroyGCPointers();

    // Now now what we need to do.
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

AptValue *AptExtObject::GetParam(const int32_t iParam)
{
    APT_ASSERT(iParam >= 0);
    AptUpdateAutoLock lock;
    // Undefined is a valid parameter
    return (gAptActionInterpreter.stackAt(iParam));
}

AptValue *AptExtObject::GetUndefinedValue(void)
{
    return (gpUndefinedValue);
}

AptValue *AptExtObject::CreateNewAptObject(void)
{
    AptUpdateAutoLock lock;
    return (new AptObject(AptVFT_Object));
}

AptValue *AptExtObject::CreateNewAptObject(const int32_t iHashSize)
{
    AptUpdateAutoLock lock;
    return (new AptObject(AptVFT_Object, iHashSize));
}

AptNativeFunction *AptExtObject::CreateNewAptFunction(AptExtFunctionPtr pAptExtFnc)
{
    AptUpdateAutoLock lock;
    return (new AptNativeFunction((AptNativeFunctionPointer)pAptExtFnc));
}

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

bool AptExtObject::SetVariable(AptValue *pContext, const AptNativeString *const pName, AptValue *const pValue)
{
    AptUpdateAutoLock lock;
    return (gAptActionInterpreter.setVariable(pContext, NULL, pName, pValue, true, false, true));
}
