#include "_Apt.h"
#include "AptTarget.h"
#include "AptAnimationTarget.h"
#include "AptCIH.h"
#include "AptCharacterInst.h"

#include "MainInline.h"
#include "Display/AptDisplayListState.h"
#include "AptObject/AptFilter.h"

#if defined(APT_DEBUGGER_ENABLE)
#include "AptDebugger/AptDebugger.h"
#endif

void AptMovie::resolve(unsigned char *pBase, AptConstFile *aConstantFile, intptr_t *pnCurrentConstantIndex)
{
    APT_ASSERT(!phLabels);
    phLabels = new AptNativeHash(APTMOVIE_LABELHASHSIZE);

    APT_RESOLVE(aFrames);
    for (int i = 0; i < (int)nFrames; i++)
    {
        APT_RESOLVE(aFrames[i].apControls);
        for (int j = 0; j < aFrames[i].nControls; j++)
        {
            APT_RESOLVE(aFrames[i].apControls[j]);
            switch (aFrames[i].apControls[j]->eType)
            {
            case (AptControlType_DoAction):
            {
                APT_RESOLVE(aFrames[i].apControls[j]->action.actions.aActionStream);
                AptActionInterpreter::resolveStream(aFrames[i].apControls[j]->action.actions.aActionStream, pBase, aConstantFile, pnCurrentConstantIndex);
            }

            break;
            case AptControlType_DoInitAction:
            {
                APT_RESOLVE(aFrames[i].apControls[j]->initAction.actions.aActionStream);
                AptActionInterpreter::resolveStream(aFrames[i].apControls[j]->initAction.actions.aActionStream, pBase, aConstantFile, pnCurrentConstantIndex);
            }

            break;
            case AptControlType_PlaceObject2:
            {
                APT_RESOLVE(aFrames[i].apControls[j]->placeObject2.szName);
                APT_RESOLVE(aFrames[i].apControls[j]->placeObject2.pActions);
                AptEventActionSet *pActions = aFrames[i].apControls[j]->placeObject2.pActions;
                if (pActions)
                {
                    APT_RESOLVE(pActions->aEventActions);
                    for (int k = 0; k < pActions->nEventActions; k++)
                    {
                        APT_RESOLVE(pActions->aEventActions[k].actions.aActionStream);
                        AptActionInterpreter::resolveStream(pActions->aEventActions[k].actions.aActionStream, pBase, aConstantFile, pnCurrentConstantIndex);
                    }
                }
            }
            break;
            case AptControlType_PlaceObject3:
            {
                APT_RESOLVE(aFrames[i].apControls[j]->placeObject3.szName);
                APT_RESOLVE(aFrames[i].apControls[j]->placeObject3.pActions);
                AptEventActionSet *pActions = aFrames[i].apControls[j]->placeObject3.pActions;
                if (pActions)
                {
                    APT_RESOLVE(pActions->aEventActions);
                    for (int k = 0; k < pActions->nEventActions; k++)
                    {
                        APT_RESOLVE(pActions->aEventActions[k].actions.aActionStream);
                        AptActionInterpreter::resolveStream(pActions->aEventActions[k].actions.aActionStream, pBase, aConstantFile, pnCurrentConstantIndex);
                    }
                }
                APT_RESOLVE(aFrames[i].apControls[j]->placeObject3.ppFilters);
                for (int32_t k = 0; k < int32_t(aFrames[i].apControls[j]->placeObject3.nNumFilters); ++k)
                {
                    APT_RESOLVE(aFrames[i].apControls[j]->placeObject3.ppFilters[k]);
                    AptFilter::Resolve(aFrames[i].apControls[j]->placeObject3.ppFilters[k], pBase);
                }
            }
            break;
            case AptControlType_FrameLabel:

            {
                APT_RESOLVE(aFrames[i].apControls[j]->frameLabel.szLabel);
                // Add this label to the hash of frame labels
                AptNativeString strLabel(aFrames[i].apControls[j]->frameLabel.szLabel);
                phLabels->Set(&strLabel, AptInteger::Create(i));
            }

            break;
            default:
            {
            }
            }
        }
    }
}

void AptMovie::unresolve(unsigned char *pBase, intptr_t *pnCurrentConstantIndex)
{
    for (int i = 0; i < (int)nFrames; i++)
    {
        for (int j = 0; j < aFrames[i].nControls; j++)
        {
            switch (aFrames[i].apControls[j]->eType)
            {
            case (AptControlType_DoAction):
            {
                AptActionInterpreter::unresolveStream(aFrames[i].apControls[j]->action.actions.aActionStream, pBase, pnCurrentConstantIndex);
                APT_UNRESOLVE(aFrames[i].apControls[j]->action.actions.aActionStream);
            }

            break;
            case AptControlType_DoInitAction:
            {
                AptActionInterpreter::unresolveStream(aFrames[i].apControls[j]->initAction.actions.aActionStream, pBase, pnCurrentConstantIndex);
                APT_UNRESOLVE(aFrames[i].apControls[j]->initAction.actions.aActionStream);
                // instead of making this as -1 we had made it -ve so here at the time of unresolving the file we can just again negate it and
                // get the original value back so we will not fail on verifyunresolve_apt in testsuite.
                // 0.17.00
                if (aFrames[i].apControls[j]->initAction.nSpriteID < 0)
                {
                    aFrames[i].apControls[j]->initAction.nSpriteID = -aFrames[i].apControls[j]->initAction.nSpriteID;
                }
            }

            break;
            case AptControlType_PlaceObject2:
            {
                AptEventActionSet *pActions = aFrames[i].apControls[j]->placeObject2.pActions;
                if (pActions)
                {
                    for (int k = 0; k < pActions->nEventActions; k++)
                    {
                        AptActionInterpreter::unresolveStream(pActions->aEventActions[k].actions.aActionStream, pBase, pnCurrentConstantIndex);
                        APT_UNRESOLVE(pActions->aEventActions[k].actions.aActionStream);
                    }
                    APT_UNRESOLVE(pActions->aEventActions);
                }
                APT_UNRESOLVE(aFrames[i].apControls[j]->placeObject2.szName);
                APT_UNRESOLVE(aFrames[i].apControls[j]->placeObject2.pActions);
            }
            break;
            case AptControlType_PlaceObject3:
            {
                AptEventActionSet *pActions = aFrames[i].apControls[j]->placeObject3.pActions;
                if (pActions)
                {
                    for (int k = 0; k < pActions->nEventActions; k++)
                    {
                        AptActionInterpreter::unresolveStream(pActions->aEventActions[k].actions.aActionStream, pBase, pnCurrentConstantIndex);
                        APT_UNRESOLVE(pActions->aEventActions[k].actions.aActionStream);
                    }
                    APT_UNRESOLVE(pActions->aEventActions);
                }
                APT_UNRESOLVE(aFrames[i].apControls[j]->placeObject3.szName);
                APT_UNRESOLVE(aFrames[i].apControls[j]->placeObject3.pActions);

                for (int32_t k = 0; k < int32_t(aFrames[i].apControls[j]->placeObject3.nNumFilters); ++k)
                {
                    AptFilter::Unresolve(aFrames[i].apControls[j]->placeObject3.ppFilters[k], pBase);
                    APT_UNRESOLVE(aFrames[i].apControls[j]->placeObject3.ppFilters[k]);
                }
                APT_UNRESOLVE(aFrames[i].apControls[j]->placeObject3.ppFilters);
            }
            break;
            case AptControlType_FrameLabel:

            {
                APT_UNRESOLVE(aFrames[i].apControls[j]->frameLabel.szLabel);
            }

            break;
            default:
            {
            }
            }
            APT_UNRESOLVE(aFrames[i].apControls[j]);
        }
        APT_UNRESOLVE(aFrames[i].apControls);
    }

    APT_UNRESOLVE(aFrames);

    if (phLabels != NULL)
    {
        phLabels->DestroyGCPointers();
        delete phLabels;
        phLabels = NULL;
    }
}

void AptMovie::DoTemporaryFrameControls(AptPseudoDisplayList *pPseudoDisplayList, int nFrame) const
{
    if (AptGetLib()->mInitParms.bUseNewClassInitOrder)
    {
        APT_ASSERT(nFrame < (int)nFrames);
    }
    else
    {
        APT_ASSERT(nFrame >= 0 && nFrame < (int)nFrames);
    }

    if (nFrame < 0)
        return;

    AptCIH *pSprInst = pPseudoDisplayList->GetParentSprite();

    for (int i = 0; i < aFrames[nFrame].nControls; i++)
    {
        AptControl *pControl = aFrames[nFrame].apControls[i];
        APT_ASSERT(nFrame == pSprInst->GetSpriteInstBase()->mnFrame);
        switch (pControl->eType)
        {
        case AptControlType_PlaceObject2:
        case AptControlType_PlaceObject3:
        {
            AptCharacter *pCharacter = NULL;
            AptPseudoCIHT *pPrev, *pItem;
            AptControlPlaceObject2 *pPlaceObject2 = &pControl->placeObject2;
            AptControlPlaceObject3 *pPlaceObject3 = &pControl->placeObject3;

            int nDepth       = 0;
            int nCharacterID = -1;

            if (pControl->eType == AptControlType_PlaceObject2)
            {
                nDepth       = pControl->placeObject2.nDepth;
                nCharacterID = pControl->placeObject2.nCharacterID;
            }
            else
            {
                APT_ASSERT(pControl->eType == AptControlType_PlaceObject3);
                nDepth       = pControl->placeObject3.nDepth;
                nCharacterID = pControl->placeObject3.nCharacterID;
            }

            pPseudoDisplayList->FindInst(nDepth, &pPrev, &pItem);
            if (nCharacterID != -1)
            {
                pCharacter = pSprInst->GetCharacterInst()->GetCharacterConst()->pParentAnim->animation.apCharacters[nCharacterID];
            }

            if (pControl->eType == AptControlType_PlaceObject2 && (pItem != NULL && nCharacterID == -1))
            {
                // If there nCharacterID is -1 and one of the four flags are true, we only update the desired information in pItem.
                APT_ASSERT(pItem->pControlInfo2 != NULL);

                pItem->pControlInfo2->matrix   = pPlaceObject2->eFlags & AptPlaceObjectFlag_Matrix ? &pPlaceObject2->matrix : pItem->pControlInfo2->matrix;
                pItem->pControlInfo2->ncxform  = pPlaceObject2->eFlags & AptPlaceObjectFlag_CXForm ? &pPlaceObject2->ncxform : pItem->pControlInfo2->ncxform;
                pItem->pControlInfo2->pActions = pPlaceObject2->eFlags & AptPlaceObjectFlag_Actions ? pPlaceObject2->pActions : pItem->pControlInfo2->pActions;
                pItem->pControlInfo2->fRatio   = pPlaceObject2->eFlags & AptPlaceObjectFlag_Ratio ? pPlaceObject2->fRatio : pItem->pControlInfo2->fRatio;
                pItem->pControlInfo2->eFlags   = (AptPlaceObjectFlags)(pItem->pControlInfo2->eFlags | pPlaceObject2->eFlags);
                // Commented out following line as it was incorrectly updating the sprite
                // In this loop we only intend to update the modified information for Move, CXForm, Actions and Ratio
                // pItem->pControl = pControl;
            }
            else if (pControl->eType == AptControlType_PlaceObject3 && (pItem != NULL && nCharacterID == -1))
            {
                // If there nCharacterID is -1 and one of the four flags are true, we only update the desired information in pItem.
                APT_ASSERT(pItem->pControlInfo3 != NULL);

                pItem->pControlInfo3->matrix   = pPlaceObject3->eFlags & AptPlaceObjectFlag_Matrix ? &pPlaceObject3->matrix : pItem->pControlInfo3->matrix;
                pItem->pControlInfo3->ncxform  = pPlaceObject3->eFlags & AptPlaceObjectFlag_CXForm ? &pPlaceObject3->ncxform : pItem->pControlInfo3->ncxform;
                pItem->pControlInfo3->pActions = pPlaceObject3->eFlags & AptPlaceObjectFlag_Actions ? pPlaceObject3->pActions : pItem->pControlInfo3->pActions;
                pItem->pControlInfo3->fRatio   = pPlaceObject3->eFlags & AptPlaceObjectFlag_Ratio ? pPlaceObject3->fRatio : pItem->pControlInfo3->fRatio;
                if (pPlaceObject3->nBlendMode != -1)
                {
                    pItem->pControlInfo3->nBlendMode = pPlaceObject3->nBlendMode;
                }
                if (pPlaceObject3->nNumFilters && pPlaceObject3->ppFilters)
                {
                    pItem->pControlInfo3->nNumFilters = pPlaceObject3->nNumFilters;
                    pItem->pControlInfo3->ppFilters   = pPlaceObject3->ppFilters;
                }

                pItem->pControlInfo3->eFlags = (AptPlaceObjectFlags)(pItem->pControlInfo3->eFlags | pPlaceObject3->eFlags);
            }
            else
            {
                // APT_ASSERT(pCharacter != NULL);
                pPseudoDisplayList->Insert(new AptPseudoCIHT(pControl, nFrame, nDepth, pCharacter)); //, pPrev, pItem);
            }
            break;
        }
        case AptControlType_RemoveObject2:
        {
            pPseudoDisplayList->Insert(new AptPseudoCIHT(pControl, nFrame, pControl->removeObject2.nDepth, NULL));
            break;
        }

        case AptControlType_StartSound:
        {
#if defined(APT_USE_SOUND_OBJECT)
            // pfnStartSound callback not called in proper order
            if (pSprInst != NULL && nFrame == 0)
            {
                AptGetUserFuncs().pfnStartSound(pSprInst->GetCharacterInst()->GetCharacterConst()->pParentAnim->animation.apCharacters[pControl->startSound.nID]->sound.zID, NULL);
            }
#endif
            break;
        }
        case AptControlType_BackgroundColour:
        case AptControlType_DoAction:
        case AptControlType_DoInitAction:
        case AptControlType_FrameLabel:
        case AptControlType_StartSoundStream:
            break;
        default:
        {
            APT_ASSERT(NOT_REACHED);
        }
        }
    }
}
void AptMovie::doFrameControls(AptDisplayList *pDisplayList, AptCIH *pInst, int nFrame) const
{
    APT_ASSERT(nFrame >= 0 && nFrame < (int)nFrames);

    // This has been moved from the normal for loop to a separate for loop as
    // we need to do all the initclip first and then go ahead and execute all other tags.
    // BUG fix -550 #initclip not running soon enough
    for (int i = 0; i < aFrames[nFrame].nControls; i++)
    {
        AptControl *pControl = aFrames[nFrame].apControls[i];
        if (pControl->eType == AptControlType_DoInitAction)
        {
            if (pControl->initAction.nSpriteID >= 0)
            {
                // APT_DEBUGPRINT(APT_DEBUG_MSG_DEFAULT_LVL, "DoInitAction:%d\n", pControl->initAction.nSpriteID);

                // ### this is done so DoInitActions will only happen once. nSpriteID was never used, so rather than adding a new value to the structre,
                //  this variable was used to ensure initclip actions only happen once.

                // void * pSavedValue   = gAptActionInterpreter.PrepareForExecution("doFrameControls");
                APT_DEFINE_ACTION_SETUP(pInst, NULL, "AptDoFrameControls", AptActionType_Initclip);
                void *pSavedValue = gAptActionInterpreter.PrepareForExecution(&oActionSetup);

#if (APT_DEBUG_LEVEL >= APT_DEBUG_NORMAL)
                int nStackSizePre = gAptActionInterpreter.stack.GetSize();
#endif
                AptCharacterInst *pCharacterInst = pInst ? pInst->GetRootAnimation()->GetAnimationInst() : NULL;
                // During initialization, component classes are registered to the global space, not to the calling movie clip
                gAptActionInterpreter.bRunningInitActions = AptActionInterpreter::ENABLE_AGGRESIVE_ZOMBIE_CLEANUP;

                gAptActionInterpreter.runStream(pControl->initAction.actions.aActionStream, pInst, -1, pCharacterInst);

                gAptActionInterpreter.bRunningInitActions = false;

                // instead of making this as -1 we will make it -ve so at the time of unresolving the file we can just again negate it and
                // get the original value back so we will not fail on verifyunresolve_apt in testsuite.
                // we are checking for nSpriteID >=0 above in if condition so this will still work.
                // 0.17.00
                pControl->initAction.nSpriteID = -pControl->initAction.nSpriteID;

#if (APT_DEBUG_LEVEL >= APT_DEBUG_NORMAL)
                int nStackSizePost = gAptActionInterpreter.stack.GetSize();
                APT_ASSERT(nStackSizePre == nStackSizePost);
#endif

                // gAptActionInterpreter.CleanupAfterExecution("doFrameControls", pSavedValue);
                gAptActionInterpreter.CleanupAfterExecution(pSavedValue, &oActionSetup); // oActionSetup is defined by the macro
            }
        }
    }

    for (int i = 0; i < aFrames[nFrame].nControls; i++)
    {
        AptControl *pControl = aFrames[nFrame].apControls[i];
        switch (pControl->eType)
        {
        // APT_CASE(AptControlType_BackgroundColour)
        case AptControlType_BackgroundColour:
        {
            // This global var is reset when a new animation is loaded.
            // The following code is to ensure that the background is only set once
            if (AptGetLib()->mbBackgroundColorSet == false)
            {
                AptGetUserFuncs().pfnSetBackgroundColour(pControl->backgroundColour.nColour);
                AptGetLib()->mbBackgroundColorSet = true;
            }
        }

        break;
        case AptControlType_PlaceObject2:
        case AptControlType_PlaceObject3:
        {
            AptCharacterAnimation *pTmpAnim = &pInst->GetCharacterInst()->GetCharacterConst()->pParentAnim->animation;

            int32_t nCharacterID = -1;
            if (pControl->eType == AptControlType_PlaceObject2)
            {
                nCharacterID = pControl->placeObject2.nCharacterID;
            }
            else
            {
                APT_ASSERT(pControl->eType == AptControlType_PlaceObject3);
                nCharacterID = pControl->placeObject3.nCharacterID;
            }

            pTmpAnim->ExecuteInitActions(pInst, nCharacterID);
#if defined(APT_DECOUPLED_RENDERING)
            if (nCharacterID != -1)
            {
                AptCharacter *pCharacter = pTmpAnim->apCharacters[nCharacterID];
                if (pCharacter->m_pAnimFile.pData == NULL)
                {
                    int nImpID = pTmpAnim->IsImport(nCharacterID);
                    if (nImpID != -1)
                    {
                        APT_ASSERT(pTmpAnim->aImports[nImpID].nID == nCharacterID);
                        pCharacter->m_pAnimFile = pTmpAnim->aImports[nImpID].file;
                    }
                    else
                    {
                        APT_ASSERT(pCharacter->pParentAnim == pInst->GetCharacterInst()->GetCharacterConst()->pParentAnim);
                        pCharacter->m_pAnimFile = pInst->GetCharacterInst()->GetCharacterConst()->m_pAnimFile;
                    }
                }
                // APT_ASSERT(pCharacter->m_pAnimFile.pData != NULL);
            }
#endif
            // It has to be called even if nCharacterID is < 0. This is triggering an issue sometimes, but it has to be treated inside placeObject
            if (pControl->eType == AptControlType_PlaceObject2)
            {
                pDisplayList->placeObject(&pControl->placeObject2, pInst);
            }
            else
            {
                APT_ASSERT(pControl->eType == AptControlType_PlaceObject3);
                pDisplayList->placeObject(&pControl->placeObject3, pInst);
            }
        }

        break;
        case AptControlType_DoAction:

        {
        }

        break;
        case AptControlType_DoInitAction:

        {
            // no need to do this any more in here as it is already done in a separate loop above.
        }

        break;
        case AptControlType_RemoveObject2:

        {
            pDisplayList->removeObject(&pControl->removeObject2);
        }

        break;
        case AptControlType_FrameLabel:

        {
        }

        break;
        case AptControlType_StartSound:

        {
#if defined(APT_USE_SOUND_OBJECT)
            APT_ASSERT(pInst);
            AptGetUserFuncs().pfnStartSound(pInst->GetCharacterInst()->GetCharacterConst()->pParentAnim->animation.apCharacters[pControl->startSound.nID]->sound.zID, NULL);
#endif
        }

        break;
        case AptControlType_StartSoundStream:

        {
#if defined(APT_USE_SOUND_OBJECT)
            APT_DEBUGPRINT(APT_DEBUG_MSG_WARNING_LVL, "warning: soundstream level is always 0\n");
            // AptGetUserFuncs().pfnStartSoundStream(0, pControl->startSoundStream.nID);
#endif
        }

        break;
        default:
        {
            APT_ASSERT(NOT_REACHED);
        }
        }
    }
}

void AptMovie::runFrameActions(AptCIH *pInst, int nFrame) const
{
    for (int i = 0; i < aFrames[nFrame].nControls; i++)
    {
        AptControl *pControl = aFrames[nFrame].apControls[i];
        if (pControl->eType == AptControlType_DoAction)
        {
            // void * pSavedValue   = gAptActionInterpreter.PrepareForExecution("tickIntervalTimers");
            APT_DEFINE_ACTION_SETUP(pInst, NULL, "AptFrameActions", AptActionType_FrameActions);
            void *pSavedValue                = gAptActionInterpreter.PrepareForExecution(&oActionSetup);
            AptCharacterInst *pCharacterInst = pInst ? pInst->GetRootAnimation()->GetAnimationInst() : NULL;
            gAptActionInterpreter.runStream(pControl->action.actions.aActionStream, pInst, -1, pCharacterInst);
            // gAptActionInterpreter.CleanupAfterExecution("runFrameActions", pSavedValue);
            gAptActionInterpreter.CleanupAfterExecution(pSavedValue, &oActionSetup); // oActionSetup is defined by the macro
        }
    }
}

void AptMovie::queueFrameActions(AptCIH *pInst, int nFrame) const
{
    for (int i = 0; i < aFrames[nFrame].nControls; i++)
    {
        AptControl *pControl = aFrames[nFrame].apControls[i];
        if (pControl->eType == AptControlType_DoAction)
        {
            GetTargetSim()->GetAnimationTarget()->AddActionBack(&pControl->action.actions, pInst,
                                                                    ACTION_TYPE_CALL_PARAM(AptActionType_FrameActions)
                                                                        gNullInput);
        }
    }
}

int AptMovie::labelToFrame(const AptNativeString *pLabel) const
{
    if (!pLabel)
    {
        APT_DEBUGPRINT(APT_DEBUG_MSG_WARNING_LVL, "WARNING: AptMovie::labelToFrame: movie not yet loaded\n");
        return -1;
    }
    APT_ASSERT(phLabels);
    AptValue *pFrame = phLabels->Lookup(pLabel);
    if (pFrame)
        return pFrame->toInteger();
    else
        return -1;
}
