#include "_Apt.h"
#include "AptTarget.h"
#include "AptAnimationTarget.h"
#include "AptCIH.h"
#include "AptCharacterInst.h"
#include "AptGlobal.h"
#include "AptValue/AptValueVector.h"
#include "AptValue/AptBoolean.h"
#include "AptValue/AptInteger.h"
#include "AptValue/AptFloat.h"
#include "MainInline.h"

#include "AptValueGCAllocator.h"

#include <unordered_map>

void RemoveReferences(AptValue **pAllocatedValues, int32_t nItemsInArray);
#if defined(APT_XML_MEMORY_DUMP_SUPPORTED)
void PrintObjectMapXML(const char *psDumpName);
#endif // APT_XML_MEMORY_DUMP_SUPPORTED

// Tracks allocation-list slots for AptCIHs currently being bulk-removed, so that if removing
// references to one ends up freeing memory, the allocation list doesn't dangle.
using allocMap = std::unordered_map<AptValue *, void *>;
static allocMap g_pAllocatedValues;

/**
 * Initializes the garbage collector.
 */
void AptGC::Initialize()
{
}

/**
 * Callback for marking GC objects in the first phase of garbage collection.
 * @param pFromRef object this call originated from
 * @param pToRef object being marked
 * @param pReferenceName reference name, for debugging
 * @param bFlag flags indicating where pToRef is
 */
void AptGC::sReferenceRegistrationCb(const AptValue *pFromRef, AptValue *&pToRef, const char *pReferenceName, int32_t bFlag)
{
    if (!pToRef->getGCMark())
    {
        if (pToRef->IsGarbageCollected())
        {
            APT_ASSERT(pToRef->mValueBitfield.mbIsAllocated && "Reached deallocated item in Garbage Collection list!");
            pToRef->setGCMark(true);
            pToRef->RegisterReferences();
        }
    }
}

/**
 * Deletes any object not reachable from a root object.
 *
 * Calls RegisterReferences on all root objects, then AptRegisterGlobalReferences, which
 * recursively marks everything they touch. Then calls DestroyGCPointers on all unmarked objects
 * before deleting any of them, since DestroyGCPointers on one may touch others about to be
 * deleted. The AptValue release path is prevented from deleting GC'd objects while this runs, for
 * the same reason.
 *
 * Note reference counting and garbage collection can interact: some objects are solely
 * reference-counted, others solely GC'd. Clean up all references in DestroyGCPointers and don't
 * delete GC'd objects until all references are cleaned up.
 */
void AptGC::CleanUnreachable()
{
#if defined(APT_DEBUG)
    int nNumDeleted = 0;
#endif

    AptGetLib()->mpValuesToRelease->ReleaseValues();

    // Step 1: find all the roots and mark them recursively.
    {
        AptValue *pObject = GetGCPoolManager()->GetFirstAptValue();

        void (*pOldValue)(const AptValue *pFromRef, AptValue *&pToRef, const char *pReferenceName, int32_t bFlag) = AptValue::sReferenceRegistrationCb;
        AptValue::sReferenceRegistrationCb                                                                        = sReferenceRegistrationCb;

        while (pObject != NULL)
        {
#if (APT_DEBUG_LEVEL >= APT_DEBUG_MAXIMUM)
            APT_ASSERT(pObject->getRefCount() > 0);
#endif
            if (pObject->getGCRoot() != 0)
            {
                if (pObject->getGCMark() == false)
                {
                    pObject->setGCMark(true);
                    pObject->RegisterReferences();
                }
            }
            pObject = GetGCPoolManager()->GetNextAptValue(pObject);
        }

        // Mark from all the other traceable objects.
        AptRegisterGlobalReferences();

        AptValue::sReferenceRegistrationCb = pOldValue;
    }

    // Step 2: clean up references on objects about to be deleted. This can cause a number of
    // marked objects to be deleted immediately (RefCounts==0), so suspend deferred deletion.
    {
        AptValue *pObject = GetGCPoolManager()->GetFirstAptValue();

        bool bOldVal                         = AptValue::sbSuspendRefcountDeletions;
        AptValue::sbSuspendRefcountDeletions = true;

        while (pObject != NULL)
        {
            if (pObject->getGCMark() == false)
            {
                pObject->PreDestroy();
                pObject->DestroyGCPointers();
            }

            // No need to cache the next value: this one isn't being deleted yet.
            pObject = GetGCPoolManager()->GetNextAptValue(pObject);
        }

        AptValue::sbSuspendRefcountDeletions = bOldVal;
    }

    // Step 3: sweep all the unmarked objects.
    {
        AptValue *pObject = GetGCPoolManager()->GetFirstAptValue();
        while (pObject != NULL)
        {
            if (pObject->getGCMark() == false)
            {
#if (APT_DEBUG_LEVEL >= APT_DEBUG_MAXIMUM)
                APT_ASSERT(pObject->getRefCount() == 0);
#endif

#if defined(APT_DEBUG)
                ++nNumDeleted;
#endif
                AptValue *pNext = GetGCPoolManager()->GetNextAptValue(pObject);
                pObject->DeleteThis();
                pObject = pNext;
            }
            else
            {
#if (APT_DEBUG_LEVEL >= APT_DEBUG_MAXIMUM)
                APT_ASSERT(pObject->getRefCount() > 0);
#endif
                // Clear the mark for the next GC run.
                pObject->setGCMark(false);

                pObject = GetGCPoolManager()->GetNextAptValue(pObject);
            }
        }
    }

    AptGetLib()->mpValuesToRelease->ReleaseValues();

#if defined(APT_DEBUG)
    if (nNumDeleted)
    {
        APT_DEBUGPRINT(APT_DEBUG_MSG_DEBUG_LVL, "Apt-GC---------------------- Sweeping of %d objects\n", nNumDeleted);
    }
#endif

    // Release the base type pools too.
    AptInteger::ClearPool();
    AptFloat::ClearPool();
    StringPool::ClearTemporaryPool();
}

/**
 * Deletes all Apt objects. Still calls DestroyGCPointers first, to clean up objects that are only
 * reference-counted (not GC'd).
 */
void AptGC::CleanAll()
{
    AptValue *pObject;
    int nNumDeleted = 0;

    AptGetLib()->mpValuesToRelease->ReleaseValues();

    {
        pObject = GetGCPoolManager()->GetFirstAptValue();

        bool bOldVal                         = AptValue::sbSuspendRefcountDeletions;
        AptValue::sbSuspendRefcountDeletions = true;

        while (pObject != NULL)
        {
            pObject->PreDestroy();
            pObject->DestroyGCPointers();
            pObject = GetGCPoolManager()->GetNextAptValue(pObject);
            nNumDeleted++;
        }

        AptValue::sbSuspendRefcountDeletions = bOldVal;
    }

    AptGetLib()->mpValuesToRelease->ReleaseValues();

    pObject = GetGCPoolManager()->GetFirstAptValue();

    while (pObject != NULL)
    {
#if (APT_DEBUG_LEVEL >= APT_DEBUG_NORMAL)
        pObject->SetDestroyedGC(); // Needed on debug builds to avoid asserts.
#endif

        AptValue *pNext = GetGCPoolManager()->GetNextAptValue(pObject);
        pObject->DeleteThis();
        pObject = pNext;
    }

    AptGetLib()->mpValuesToRelease->ReleaseValues();

#if defined(APT_DEBUG)
    if (nNumDeleted)
    {
        APT_DEBUGPRINT(APT_DEBUG_MSG_DEBUG_LVL, "Apt-GC-CleanAll------------- Cleaned %d objects\n", nNumDeleted);
    }
#endif

    AptInteger::ClearPool();
    AptFloat::ClearPool();
    StringPool::ClearTemporaryPool();
}

#if defined(APT_XML_MEMORY_DUMP_SUPPORTED)

bool gbDumpOnLoadAnimation = false;

static void XMLReferenceRegistrationCb(const AptValue *pFromRef, AptValue *&pToRef, const char *pReferenceName, int32_t bFlag)
{
    APT_DEBUGPRINT(APT_DEBUG_MSG_DEFAULT_LVL, "<REFV2 FROM=\"%u\" TO=\"%u\" NAME=\"%s\"/>\n",
                   pFromRef ? pFromRef->GetUniqueID() : 0xffffffff, pToRef->GetUniqueID(), pReferenceName);
}
#endif // defined(APT_XML_MEMORY_DUMP_SUPPORTED)

static AptValue *gpRefernceValue        = NULL;
static AptValue *gpRefernceValueReplace = NULL;
static int32_t pnRefCount               = 0;

/**
 * Callback for AptValue::sReferenceRegistrationCb: replaces every reference to gpRefernceValue
 * with gpRefernceValueReplace (or the undefined value, if none given).
 */
static void ReferenceReplaceCb(const AptValue *pFromRef, AptValue *&pToRef, const char *pReferenceName, int32_t bFlag)
{
    if (pToRef != gpRefernceValue)
        return;

    APT_ASSERT(bFlag != APT_REFREG_IS_DISPLAYLIST);
#if !defined(APT_DEBUG)
    if (bFlag == APT_REFREG_IS_DISPLAYLIST)
    {
        APT_DEBUGPRINT(APT_DEBUG_MSG_WARNING_LVL, "THIS SHOULD NOT HAPPEN, PLEASE CONTACT APT TEAM");
        return;
    }
#endif

    // We're removing the reference: dec the object so we know when we've got them all, but not
    // until we're done with it.
    AptValue *pHoldCIH = pToRef;

    if (pToRef->isCIH(true))
    {
        if (gpRefernceValueReplace == NULL)
        {
            if (pFromRef == NULL || !pFromRef->isScriptFunction())
            {
                pToRef = (bFlag == APT_REFREG_IS_APTCIH) ? gpUndefinedCIH : (AptValue *)gpUndefinedValue;
            }
            else if (pFromRef != NULL && pFromRef->isScriptFunction())
            {
                if (pFromRef->c_scriptfunction()->mpParentAnim == pToRef)
                {
                    pToRef->c_cih(true)->DecZombieCount();
                }
                pToRef = gpUndefinedCIH;
            }
        }
        else
        {
            if (pFromRef == NULL || !pFromRef->isScriptFunction())
            {
                if (bFlag == APT_REFREG_IS_APTCIH && !gpRefernceValueReplace->isCIH())
                {
                    pToRef = gpUndefinedCIH;
                }
                else
                {
                    pToRef = (AptValue *)gpRefernceValueReplace;
                    APT_INC(gpRefernceValueReplace);
                    pnRefCount--;
                }
            }
            else if (pFromRef != NULL && pFromRef->isScriptFunction())
            {
                if ((pFromRef->c_scriptfunction()->mpParentAnim == pToRef) && pToRef->c_cih(true)->GetCIHState() != AptCIH::AptCIHState_Zombie)
                {
                    pToRef->c_cih(true)->DecZombieCount();
                    pToRef = (!gpRefernceValueReplace->isCIH()) ? gpUndefinedCIH : (AptValue *)gpRefernceValueReplace;
                    if (pToRef == gpRefernceValueReplace)
                    {
                        APT_INC(gpRefernceValueReplace);
                    }
                }
            }
        }
    }
    else
    {
        if (gpRefernceValueReplace == NULL)
        {
            pToRef = (AptValue *)gpUndefinedValue;
        }
        else
        {
            pToRef = (AptValue *)gpRefernceValueReplace;
            APT_INC(gpRefernceValueReplace);
        }
    }
    APT_ASSERT(pnRefCount >= 0);
    if (pToRef != pHoldCIH)
    {
        APT_DEC(pHoldCIH);
    }
    else
    {
        // If nothing was replaced, make sure there was a good reason.
        APT_ASSERT(pToRef->c_cih(true)->GetCIHState() == AptCIH::AptCIHState_Zombie);
    }
}

/**
 * Replaces all references to pValue with pNewObj.
 * @param pAllocatedValues linear array of all GC values, or NULL to walk the whole GC pool
 * @param nItemsInArray number of items in pAllocatedValues
 */
void AptGC::ReplaceReferences(AptValue *pValue, AptValue *pNewObj, AptValue **pAllocatedValues, int32_t nItemsInArray)
{
    gpRefernceValue        = pValue;
    gpRefernceValueReplace = pNewObj;
    pnRefCount             = pValue->getRefCount();
    uint32_t nTempRefCount = pnRefCount;

    AptValue *pObject                                                                                         = GetGCPoolManager()->GetFirstAptValue();
    void (*pOldValue)(const AptValue *pFromRef, AptValue *&pToRef, const char *pReferenceName, int32_t bFlag) = AptValue::sReferenceRegistrationCb;
    AptValue::sReferenceRegistrationCb                                                                        = ReferenceReplaceCb;

    // Try the animation-pool-data pools first, then fall back to walking every object.
    GetTargetSim()->GetAnimationTarget()->RemoveCIHReferences();
    AptRegisterGlobalReferences();

    if (pValue->getRefCount() > 1)
    {
        if (pAllocatedValues == NULL)
        {
#if defined(APT_DEBUG) // Run through everything in debug, to catch ref-count issues.
            while (pObject != NULL)
#else
            while (pObject != NULL && pnRefCount > 0)
#endif
            {
                pObject->RegisterReferences();
                pObject = GetGCPoolManager()->GetNextAptValue(pObject);
                if (pValue->getRefCount() == 1)
                {
                    break;
                }
            }
        }
        else
        {
            for (int32_t nArrayIndex = 0; nArrayIndex < (int32_t)nItemsInArray; nArrayIndex++)
            {
                pObject = pAllocatedValues[nArrayIndex];
                if (pObject != NULL && pObject->mValueBitfield.mbIsAllocated)
                {
                    pObject->RegisterReferences();
                    if (pValue->getRefCount() == 1 || pnRefCount == 0)
                    {
                        break;
                    }
                }
                if (pObject == pValue)
                {
                    pAllocatedValues[nArrayIndex] = NULL;
                }
            }
        }
    }

    if (pNewObj != NULL)
    {
        pNewObj->setRefCount(pNewObj->getRefCount() + (nTempRefCount - pnRefCount)); // Match the replaced object's ref count.
    }

    gpRefernceValue        = NULL;
    gpRefernceValueReplace = NULL;
    pnRefCount             = 0;

    AptValue::sReferenceRegistrationCb = pOldValue;
}

/**
 * Like ReferenceReplaceCb, but only touches AptCIHs that are GC-marked and on the animation
 * target's delayed release list -- a bulk-delete fast path for AptAnimationTarget::CleanRemList().
 */
static void RemoveReferenceRegistrationCb(const AptValue *pFromRef, AptValue *&pToRef, const char *pReferenceName, int32_t bFlag)
{
    if (pToRef->getGCMark() == false)
    {
        return;
    }

    APT_ASSERT(bFlag != APT_REFREG_IS_DISPLAYLIST);
#if !defined(APT_DEBUG)
    if (bFlag == APT_REFREG_IS_DISPLAYLIST)
    {
        APT_DEBUGPRINT(APT_DEBUG_MSG_WARNING_LVL, "THIS SHOULD NOT HAPPEN, PLEASE CONTACT APT TEAM");
        return;
    }
#endif

    AptValue *pHoldCIH = pToRef;

    if (pToRef->isCIH(true))
    {
        if (gpRefernceValueReplace == NULL)
        {
            if (pFromRef == NULL || !pFromRef->isScriptFunction())
            {
                pToRef = (bFlag == APT_REFREG_IS_APTCIH) ? gpUndefinedCIH : (AptValue *)gpUndefinedValue;
            }
            else if (pFromRef != NULL && pFromRef->isScriptFunction())
            {
                if (pFromRef->c_scriptfunction()->mpParentAnim == pToRef)
                {
                    pToRef->c_cih(true)->DecZombieCount();
                }
                pToRef = gpUndefinedCIH;
            }
        }
        else
        {
            if (pFromRef == NULL || !pFromRef->isScriptFunction())
            {
                if (bFlag == APT_REFREG_IS_APTCIH && !gpRefernceValueReplace->isCIH())
                {
                    pToRef = gpUndefinedCIH;
                }
                else
                {
                    pToRef = (AptValue *)gpRefernceValueReplace;
                    APT_INC(gpRefernceValueReplace);
                    pnRefCount--;
                }
            }
            else if (pFromRef != NULL && pFromRef->isScriptFunction())
            {
                if ((pFromRef->c_scriptfunction()->mpParentAnim == pToRef) && pToRef->c_cih(true)->GetCIHState() != AptCIH::AptCIHState_Zombie)
                {
                    pToRef->c_cih(true)->DecZombieCount();
                    pToRef = (!gpRefernceValueReplace->isCIH()) ? gpUndefinedCIH : (AptValue *)gpRefernceValueReplace;
                    if (pToRef == gpRefernceValueReplace)
                    {
                        APT_INC(gpRefernceValueReplace);
                    }
                }
            }
        }
    }
    else
    {
        if (gpRefernceValueReplace == NULL)
        {
            pToRef = (AptValue *)gpUndefinedValue;
        }
        else
        {
            pToRef = (AptValue *)gpRefernceValueReplace;
            APT_INC(gpRefernceValueReplace);
        }
    }
    APT_ASSERT(pnRefCount >= 0);
    if (pToRef != pHoldCIH)
    {
        if (pHoldCIH->getRefCount() == 1)
        {
            // Find pToRef's slot in sapDelayedReleaseList so it can be removed from the list.
            AptCIH **listPtrForToRef                = NULL;
            AptAnimationCihList &delayedReleaseList = AptAnimationTarget::GetDelayedReleaseList();
            int32_t delayedReleaseListSize          = AptAnimationTarget::GetDelayedReleaseListSize();
            for (int32_t idxReleaseList = 0; idxReleaseList < delayedReleaseListSize; idxReleaseList++)
            {
                if (pHoldCIH == delayedReleaseList[idxReleaseList])
                {
                    listPtrForToRef = &delayedReleaseList[idxReleaseList];
                    break;
                }
            }

            APT_ASSERT(listPtrForToRef != NULL);

            *listPtrForToRef = NULL;

            allocMap::iterator allocIter = g_pAllocatedValues.find(pHoldCIH);
            if (allocIter != g_pAllocatedValues.end())
            {
                *((uintptr_t *)allocIter->second) = 0;
            }
        }

        APT_DEC(pHoldCIH);
    }
    else
    {
        APT_ASSERT(pToRef->c_cih(true)->GetCIHState() == AptCIH::AptCIHState_Zombie);
    }
}

/**
 * Removes all references to items on the animation target's delayed release list. Items in the
 * list must already have their GC mark set to true.
 *
 * Called from AptAnimationTarget::CleanRemList() to replace all references to AptCIHs being
 * deleted. All GC object pointers are gathered into a linear array first, then RegisterReferences
 * is called on each, recursively invoking RemoveReferenceRegistrationCb via the
 * APT_REGISTER_REFERENCE_XXX calls -- which only replaces GC-marked AptCIHs, ignoring everything
 * else. This is much faster than calling ReplaceReferences per AptCIH, which would walk every GC
 * object for each one.
 * @param pAllocatedValues all objects that could hold a reference
 * @param nItemsInArray number of items in pAllocatedValues
 */
void RemoveReferences(AptValue **pAllocatedValues, int32_t nItemsInArray)
{
    for (int32_t i = 0; i < nItemsInArray; ++i)
    {
        g_pAllocatedValues[pAllocatedValues[i]] = &pAllocatedValues[i];
    }

    gpRefernceValue        = NULL;
    gpRefernceValueReplace = NULL;

    void (*oldCallback)(const AptValue *pFromRef, AptValue *&pToRef, const char *pReferenceName, int32_t bFlag) = AptValue::sReferenceRegistrationCb;
    AptValue::sReferenceRegistrationCb                                                                          = RemoveReferenceRegistrationCb;

    GetTargetSim()->GetAnimationTarget()->RemoveCIHReferences();
    AptRegisterGlobalReferences();

    for (int32_t nArrayIndex = 0; nArrayIndex < nItemsInArray; nArrayIndex++)
    {
        AptValue *pObject = pAllocatedValues[nArrayIndex];
        if (pObject != NULL && pObject->mValueBitfield.mbIsAllocated)
        {
            pObject->RegisterReferences();
        }
    }

    g_pAllocatedValues.clear();

    AptValue::sReferenceRegistrationCb = oldCallback;
}

#if defined(APT_XML_MEMORY_DUMP_SUPPORTED)
/**
 * Prints the whole memory graph as XML, for debugging.
 */
void PrintObjectMapXML(const char *psDumpName)
{
    AptValue *pObject;
    static int gnXMLDumpIndex = 0;

    APT_DEBUGPRINT(APT_DEBUG_MSG_DEFAULT_LVL, "<DUMP NAME=\"%s\" INDEX=\"%d\" FORMAT=\"2\">\n", psDumpName, gnXMLDumpIndex++);

    void (*pOldValue)(const AptValue *pFromRef, AptValue *&pToRef, const char *pReferenceName, int32_t bFlag) = AptValue::sReferenceRegistrationCb;
    AptValue::sReferenceRegistrationCb                                                                        = XMLReferenceRegistrationCb;

    pObject = GetGCPoolManager()->GetFirstAptValue();

    while (pObject != NULL)
    {
        if (pObject->isScriptFunction() || pObject->isCIH())
        {
            AptCharacterAnimationInst *pAnim = NULL;
            AptNativeString sDesc;
            if (pObject->isCIH())
            {
                AptCIH *pCIHObject = pObject->c_cih();
                AptCIH *pCIHRoot   = pCIHObject->GetRootAnimation();
                if (pCIHRoot->IsAnimationInst())
                {
                    pAnim = pCIHRoot->GetAnimationInst();
                }
                sDesc = pCIHObject->GetInstanceName();
            }
            else
            {
                AptScriptFunctionBase *pSF = pObject->c_scriptfunction();
                const char *pName          = pSF->GetName();
                pAnim                      = pSF->mpParentAnim->GetAnimationInst();
                sDesc                      = pName ? pName : "";
            }
            APT_DEBUGPRINT(APT_DEBUG_MSG_DEFAULT_LVL, "<NODE ID=\"%u\" PTR=\"0x%X\" TYPE=\"%s\" REFCOUNT=\"%d\" GCROOT=\"%s\" FILENAME=\"%s.swf\" DISCRIPTION=\"%s\"/>\n",
                           pObject->GetUniqueID(),
                           pObject,
                           AptGetTypeOfAptValue(pObject),
                           pObject->getRefCount(),
                           pObject->getGCRoot() ? "TRUE" : "FALSE",
                           pAnim ? pAnim->mpFile.Get()->GetName().c_str() : "",
                           sDesc.c_str());
        }
        else
        {
            APT_DEBUGPRINT(APT_DEBUG_MSG_DEFAULT_LVL, "<NODE ID=\"%u\" PTR=\"0x%X\" TYPE=\"%s\" REFCOUNT=\"%d\" GCROOT=\"%s\"/>\n",
                           pObject->GetUniqueID(),
                           pObject,
                           AptGetTypeOfAptValue(pObject),
                           pObject->getRefCount(),
                           pObject->getGCRoot() ? "TRUE" : "FALSE");
        }
        pObject = GetGCPoolManager()->GetNextAptValue(pObject);
    }

    pObject = GetGCPoolManager()->GetFirstAptValue();

    while (pObject != NULL)
    {
        pObject->RegisterReferences();
        pObject = GetGCPoolManager()->GetNextAptValue(pObject);
    }

    AptRegisterGlobalReferences();

    AptValue::sReferenceRegistrationCb = pOldValue;

    APT_DEBUGPRINT(APT_DEBUG_MSG_DEFAULT_LVL, "</DUMP>\n");
}
#endif
