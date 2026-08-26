/*** Include files ********************************************************************************/

#include "SDK/AptExtObjectRegistry.h"
#include "AptObject/AptGlobalObject.h"
#include "AptObject/AptGlobalExtensionObject.h"
#include "_Apt.h"
#include "MainInline.h"

/*** Defines **************************************************************************************/

/*** Macros ***************************************************************************************/

/*** Type Definitions *****************************************************************************/

/*** Function Prototypes **************************************************************************/

/*** Variables ************************************************************************************/

// Private variables
// Public variables

/*** Public Functions *****************************************************************************/

void AptExtObjectRegistry::Register(AptExtObject *pExtObject)
{
    AptNativeString sObjName(pExtObject->GetName());

    // If the name contains '.' then we must build
    // up the namespace as a tree of Objects. This
    // is what Flash does when you declare a class
    // in ActionScript with a fully-qualified name.
    // For each scope (denoted by '.') in sObjName, create a nested object under _global.
    AptObject *pScope = AptGetLib()->mpGlobalGlobalObject;
    int32_t i         = _ForEachScope(sObjName, &pScope, _CreateScope);

    // conveniently, i is 1 + the last index of '.' within sObjName
    if (i > 0)
    {
        // We are registering a class within a namespace.
        // Skip the namespace letters and parse out
        // just the class name.
        sObjName = sObjName.Mid(i);
    }
    else
    {
        // We are registering at the global scope,
        // not within a namespace.
        pScope = AptGetLib()->mpGlobalExtensionObject;
    }

    // if this assert fires, the object has already been registered
    APT_ASSERT(NULL == pScope->Lookup(&sObjName));
    pScope->Set(&sObjName, pExtObject);
}

void AptExtObjectRegistry::UnRegister(const char *szName)
{
    APT_ASSERT(szName != NULL);

    AptNativeString sObjName(szName);

    // find the parent scope of the given class
    AptObject *pScope = AptGetLib()->mpGlobalGlobalObject;
    int32_t i         = _ForEachScope(sObjName, &pScope, _GetNextScope);

    // conveniently, i is 1 + the last index of '.' within sObjName
    if (i > 0)
    {
        // We are unregistering a class within a namespace.
        // Skip the namespace letters and parse out
        // just the class name.
        sObjName = sObjName.Mid(i);
    }
    else
    {
        // We are registering at the global scope,
        // not within a namespace.
        pScope = AptGetLib()->mpGlobalExtensionObject;
    }

    // remove the AptExtObject from the scope
    AptValue *pExtObject = pScope->Lookup(&sObjName);
    APT_ASSERT(NULL != pExtObject);

    // Note sim thread was locked in AptUnRegisterExtension
    _APT_INC(pExtObject);
    pScope->GetNativeHashVirtual()->Unset(&sObjName);
    _APT_DEC(pExtObject);

    if (i > 0)
    {
        // tear down the namespace scopes that we set up when the object was registered
        AptNativeString sFullName(szName);
        _UnsetEmptyChild(AptGetLib()->mpGlobalGlobalObject, sFullName, 0, sFullName.Find('.'));
    }
}

/*** Private Functions ****************************************************************************/

int32_t AptExtObjectRegistry::_ForEachScope(const AptNativeString &sName, AptObject **ppScope, ScopeFuncPtr pActionFunction)
{
    int32_t i = 0;
    for (int32_t j = sName.Find('.'); (j >= 0) && *ppScope; j = sName.Find('.', j + 1))
    {
        // Get the next part of the namespace and
        // create or delete an Object in it.
        AptNativeString sSubStr = sName.Mid(i, j - i);
        AptObject *pNewScope    = pActionFunction(sSubStr, *ppScope);
        *ppScope                = pNewScope;
        i                       = j + 1;
    }
    return *ppScope ? i : 0;
}

AptObject *AptExtObjectRegistry::_CreateScope(const AptNativeString &sName, AptObject *pScope)
{
    APT_ASSERT(sName.Size() > 0);
    AptValue *pNewScope = pScope->Lookup(&sName);
    if (!pNewScope)
    {
        pNewScope = new AptObject(AptVFT_Object);
        pScope->Set(&sName, pNewScope);
    }
    else
    {
        // If namespace collides with an existing class or movieclip name, then
        // that's OK, but it sure better be an Object.
        APT_ASSERT(pNewScope->isObject());
    }
    return (pNewScope && pNewScope->isObject()) ? static_cast<AptObject *>(pNewScope) : NULL;
}

AptObject *AptExtObjectRegistry::_GetNextScope(const AptNativeString &sName, AptObject *pScope)
{
    AptValue *pNextScope = pScope ? pScope->Lookup(&sName) : NULL;
    return (pNextScope && pNextScope->isObject()) ? static_cast<AptObject *>(pNextScope) : NULL;
}

bool AptExtObjectRegistry::_UnsetEmptyChild(AptObject *pScope, const AptNativeString &sFullName, int32_t i, int32_t j)
{
    // check that the substring start index isn't out of bounds
    if (pScope && (i < static_cast<int32_t>(sFullName.GetLength())))
    {
        // check that the substring end index isn't out of bounds either
        if (j < 0)
            j = sFullName.GetLength();

        // get the substring that is the child scope,
        // and lookup that child scope.
        AptNativeString sChildName = sFullName.Mid(i, j - i);
        AptValue *pChild           = pScope->Lookup(&sChildName);
        if (pChild)
        {
            APT_ASSERT(pChild->isObject());
            if (pChild->isObject())
            {
                do
                {
                    // If the child scope has no children itself, then
                    // it can be deleted (we just need to call Unset).
                    // This is how the empty namespace hierarchy gets deleted.
                    if (!pChild->GetNativeHashVirtual()->GetFirstItem())
                    {
                        pScope->GetNativeHashVirtual()->Unset(&sChildName);
                        return true;
                    }

                    // Otherwise, if the child scope has children, then recurse and try
                    // to delete empty sub-scopes.
                } while (_UnsetEmptyChild(static_cast<AptObject *>(pChild), sFullName, j + 1, sFullName.Find('.', j + 1)));
            }
        }
    }
    return false;
}
