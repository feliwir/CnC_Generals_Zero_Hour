/*** Include files ********************************************************************************/
#include "_Apt.h"
#include "AptCIH.h"
#include "AptCharacterInst.h"
#include "AptRenderItem.h"
#include "AptCharacter/AptCharacter.h"
#include "MainInline.h"

#include "AptRenderList.h" // for add function
/*** Defines **************************************************************************************/

/*** Macros ***************************************************************************************/

/*** Type Definitions *****************************************************************************/

/*** Function Prototypes **************************************************************************/

void AptDecoupleTreeTraversal(const AptCIH *pCur, int32_t nCurrRenderTick, AptRenderingContext *pRenderingContext, AptMaskRenderOperation eMaskOperation, int nMaskDepth, AptAnimLevelE eAnimLevels, bool bUseDepthCompare);
AptAssetEffect PushEffectRenderElement(const AptCIH *pCIH, const AptRenderItem *pRenderItem);
void PopEffectRenderElement(const AptRenderItem *pRenderItem, AptAssetEffect pEffect);

/*** Variables ************************************************************************************/

// Private variables

// Public variables

/*** Private Functions ****************************************************************************/

AptAssetEffect PushEffectRenderElement(const AptCIH *pCIH, const AptRenderItem *pRenderItem)
{
    AptCIH *pNonConstCIH = (AptCIH *)pCIH;
    if (pCIH->GetHasUIEffects())
    {
        AptEffectData oEffectData;
        oEffectData.mnBlendMode = (uint32_t)pCIH->GetCharacterInst()->GetBlendMode();
        AptNativeHash *pHash    = pNonConstCIH->GetNativeHashVirtual();
        if (pHash)
        {
            // many times hashtable is empty/null so make sure to check it first.
            oEffectData.pFiltersArray = pHash->Lookup(StringPool::GetString(SC_filters));
        }

        APT_ASSERT(AptGetUserFuncs().pfnCreateEffect);
        if (AptGetUserFuncs().pfnCreateEffect != NULL)
        {
            AptAssetEffect pEffect = AptGetUserFuncs().pfnCreateEffect(oEffectData, (AptValue *)pCIH);

            if (pEffect)
            {
                AptRenderInfo info;
                info.mnLevel    = AptRenderItem::suCurrentRenderAnimLevel;
                info.mTransform = *AptMath::_ClipStackGetTop();
                // this is to denote that we are pushing a UIEffect, we don't want to add extra member to AptRenderInfo just to denote that
                // we are pushing a UIEffect, if really needed we can add a separate member for that or share enum bitfields.
                info.meMaskOper  = (AptMaskRenderOperation)AptEffectOperation_Apply;
                info.mnMaskDepth = 0;
                info.mpEffect    = pEffect;
                AptRenderList::Add(info, pRenderItem, NULL);
            }
            return pEffect;
        }
        else
        {
            return NULL;
        }
    }
    else
        return NULL;
}

void PopEffectRenderElement(const AptRenderItem *pRenderItem, AptAssetEffect pEffect)
{
    if (pEffect != NULL)
    {
        AptRenderInfo info;
        info.mnLevel    = AptRenderItem::suCurrentRenderAnimLevel;
        info.mTransform = *AptMath::_ClipStackGetTop();
        // this is to denote that we are pushing a UIEffect, we don't want to add extra member to AptRenderInfo just to denote that
        // we are pushing a UIEffect, if really needed we can add a separate member for that or share enum bitfields.
        info.meMaskOper  = (AptMaskRenderOperation)AptEffectOperation_Unapply;
        info.mnMaskDepth = 0;
        info.mpEffect    = pEffect;
        AptRenderList::Add(info, pRenderItem, NULL);
    }
}

/*** Public Functions *****************************************************************************/

void AptDecoupleTreeTraversalClipper(const AptCIH *pRenderItemBegin, int32_t nCurrRenderTick, AptRenderingContext *pRenderingContext, AptMaskRenderOperation eMaskOperation, int nMaskDepth, AptAnimLevelE eAnimLevels, bool bUseDepthCompare, const AptCIH *pRenderItemEnd);
const AptCIH *AptDecoupleClippedTreeTraversal(const AptCIH *pCur, int32_t nCurrRenderTick, AptRenderingContext *pRenderingContext, AptMaskRenderOperation eMaskOperation, int nMaskDepth, AptAnimLevelE eAnimLevels, bool bUseDepthCompare);

static void AptDecoupleMaskedTreeTraversal2(const AptCIH *pCur, int32_t nCurrRenderTick, AptRenderingContext *pRenderingContext, AptMaskRenderOperation eMaskOperation, int nMaskDepth, AptAnimLevelE eAnimLevels, bool bUseDepthCompare)
{
    // This Does not do the bUseDepthCompare because we are still honoring setmask.
    const AptCIH *pChild = pCur->GetFirstChild();
    if (pChild && pChild->GetCharacterInst()->GetCharacterType() != AptCharacterType_CustomControl)
    {
        const AptRenderItem *pRenderItem = pCur->GetCharacterInst()->mpRenderItem;
        ;
        pRenderItem->PushRenderData(pRenderingContext, eMaskOperation, nMaskDepth);
        AptAssetEffect pEffect = PushEffectRenderElement(pCur, pRenderItem);
        AptDecoupleMaskedTreeTraversal2(pChild, nCurrRenderTick, pRenderingContext, eMaskOperation, nMaskDepth, eAnimLevels, false);
        PopEffectRenderElement(pRenderItem, pEffect);
        pRenderItem->PopRenderData(pRenderingContext, eMaskOperation, nMaskDepth);
    }
    else
    {
        const AptRenderItem *pRenderItem = pCur->GetCharacterInst()->mpRenderItem;
        pRenderItem->Render(pRenderingContext, eMaskOperation, nMaskDepth, pCur);
    }

    AptCIH *pNext = pCur->GetDisplayListNext();

    while (pNext)
    {
        pChild = pNext->GetFirstChild();

        if (pChild && pNext->GetCharacterInst()->GetCharacterType() != AptCharacterType_CustomControl)
        {
            const AptRenderItem *pRenderItem = pNext->GetCharacterInst()->mpRenderItem;
            ;
            pRenderItem->PushRenderData(pRenderingContext, eMaskOperation, nMaskDepth);
            AptAssetEffect pEffect = PushEffectRenderElement(pNext, pRenderItem);
            AptDecoupleMaskedTreeTraversal2(pChild, nCurrRenderTick, pRenderingContext, eMaskOperation, nMaskDepth, eAnimLevels, false);
            PopEffectRenderElement(pRenderItem, pEffect);
            pRenderItem->PopRenderData(pRenderingContext, eMaskOperation, nMaskDepth);
        }
        else
        {
            const AptRenderItem *pRenderItem = pNext->GetCharacterInst()->mpRenderItem;
            pRenderItem->Render(pRenderingContext, eMaskOperation, nMaskDepth, pNext);
        }
        pNext = pNext->GetDisplayListNext();
    }
}

static void AptDecoupleMaskedTreeTraversal(const AptCIH *pCur, int32_t nCurrRenderTick, AptRenderingContext *pRenderingContext, AptMaskRenderOperation eMaskOperation, int nMaskDepth, AptAnimLevelE eAnimLevels, bool bUseDepthCompare)
{
    // This Does not do the bUseDepthCompare because we are still honoring setmask.
    const AptCIH *pChild = pCur->GetFirstChild();
    if (pChild && pChild->GetCharacterInst()->GetCharacterType() != AptCharacterType_CustomControl)
    {
        const AptRenderItem *pRenderItem = pCur->GetCharacterInst()->mpRenderItem;
        ;
        pRenderItem->PushRenderData(pRenderingContext, eMaskOperation, nMaskDepth);
        AptAssetEffect pEffect = PushEffectRenderElement(pCur, pRenderItem);
        AptDecoupleMaskedTreeTraversal(pChild, nCurrRenderTick, pRenderingContext, eMaskOperation, nMaskDepth, eAnimLevels, false);
        PopEffectRenderElement(pRenderItem, pEffect);
        pRenderItem->PopRenderData(pRenderingContext, eMaskOperation, nMaskDepth);

        AptCIH *pNext = pChild->GetDisplayListNext();

        while (pNext)
        {
            pChild = pNext->GetFirstChild();

            if (pChild && pNext->GetCharacterInst()->GetCharacterType() != AptCharacterType_CustomControl)
            {
                const AptRenderItem *pRenderItem = pNext->GetCharacterInst()->mpRenderItem;
                ;
                pRenderItem->PushRenderData(pRenderingContext, eMaskOperation, nMaskDepth);
                AptAssetEffect pEffect = PushEffectRenderElement(pNext, pRenderItem);
                AptDecoupleMaskedTreeTraversal2(pChild, nCurrRenderTick, pRenderingContext, eMaskOperation, nMaskDepth, eAnimLevels, false);
                PopEffectRenderElement(pRenderItem, pEffect);
                pRenderItem->PopRenderData(pRenderingContext, eMaskOperation, nMaskDepth);
            }
            else
            {
                const AptRenderItem *pRenderItem = pNext->GetCharacterInst()->mpRenderItem;
                pRenderItem->Render(pRenderingContext, eMaskOperation, nMaskDepth, pNext);
            }
            pNext = pNext->GetDisplayListNext();
        }
    }
    else
    {
        const AptRenderItem *pRenderItem = pCur->GetCharacterInst()->mpRenderItem;
        pRenderItem->Render(pRenderingContext, eMaskOperation, nMaskDepth, pCur);
    }
}

void AptDecoupleTreeTraversal(const AptCIH *pCur, int32_t nCurrRenderTick, AptRenderingContext *pRenderingContext, AptMaskRenderOperation eMaskOperation, int nMaskDepth, AptAnimLevelE eAnimLevels, bool bUseDepthCompare)
{
    const AptCIH *pChild               = NULL;
    const AptCIH *pMaskRenderItemBegin = NULL; // Pointer to clipper object
    const AptCIH *pMaskRenderItemEnd   = NULL; // Pointer to clipper object
    const AptCIH *pMask                = NULL;
    const AptCIH *pNext                = pCur;

    APT_PREFETCH(0, pNext);

    while (pNext)
    {
        const AptCIH *nextNext = pNext->GetDisplayListNext();

        APT_PREFETCH(0, nextNext);

        if (bUseDepthCompare)
        {
            if (!(((int)(1 << pNext->GetDepth())) & eAnimLevels))
            {
                pNext = nextNext;
                continue;
            }
            AptRenderItem::suCurrentRenderAnimLevel = pNext->GetDepth();
        }

        if (pMaskRenderItemBegin != NULL && pMaskRenderItemBegin->GetCharacterInst()->mpRenderItem->GetClipDepth() < pNext->GetDepth()) // If we are clipping and this object is not clipped, push out the clipped stuff and continue to render as normal
        {
            nMaskDepth--;
            AptDecoupleTreeTraversalClipper(pMaskRenderItemBegin, nCurrRenderTick, pRenderingContext, AptMaskRenderOperation_Subtract, nMaskDepth, eAnimLevels, bUseDepthCompare, pMaskRenderItemEnd);
            pMaskRenderItemBegin = NULL;
            pMaskRenderItemEnd   = NULL;
        }
        if (pMaskRenderItemBegin == NULL && pNext->GetCharacterInst()->mpRenderItem->GetClipDepth() >= 0) // This pRenderItem is a clipper
        {
            pMaskRenderItemEnd   = AptDecoupleClippedTreeTraversal(pNext, nCurrRenderTick, pRenderingContext, AptMaskRenderOperation_Add, nMaskDepth, eAnimLevels, bUseDepthCompare);
            pMaskRenderItemBegin = pNext;
            pNext                = pMaskRenderItemEnd;
            nMaskDepth++;
        }
        else if (pNext->GetCharacterInst()->GetIsVisible())
        {
            // I'm checking AptCharacterInst::GetIsVisible() here because it's faster than AptCIH::IsVisible(). It's faster
            // because it doesn't iterate up the tree checking the visibility of all the CIH's parents as well. I'm trusting here
            // that it's safe to use it, because this is iterating down the tree and we should already have pruned if one of this
            // node's parents was hidden. Just in case I'm completely wrong, though, here's an assert. (svogelpohl)
            APT_ASSERT(pNext->IsVisible());

            if (pNext->HasMask())
            {
                pMask = pNext->GetMask();
                APT_ASSERT(pMask);
                const AptRenderItem *pMaskRenderItem = pMask->GetCharacterInst()->mpRenderItem;
                pMaskRenderItem->PushRenderDataAbsolute(pRenderingContext);
                AptAssetEffect pEffect = PushEffectRenderElement(pMask, pMaskRenderItem);
                AptDecoupleMaskedTreeTraversal(pMask, nCurrRenderTick, pRenderingContext, AptMaskRenderOperation_Add, nMaskDepth, eAnimLevels, bUseDepthCompare);
                PopEffectRenderElement(pMaskRenderItem, pEffect);
                pMaskRenderItem->PopRenderData(pRenderingContext, AptMaskRenderOperation_Add, nMaskDepth);
                nMaskDepth++;
            }

            if (!pNext->IsMask())
            {
                pChild = pNext->GetFirstChild();

                if (pChild && pNext->GetCharacterInst()->GetRenderItem()->GetCharacterType() != AptCharacterType_CustomControl)
                {
                    const AptRenderItem *pRenderItem = pNext->GetCharacterInst()->mpRenderItem;
                    pRenderItem->PushRenderData(pRenderingContext, eMaskOperation, nMaskDepth);
                    AptAssetEffect pEffect = PushEffectRenderElement(pNext, pRenderItem);
                    pRenderItem->Render(pRenderingContext, eMaskOperation, nMaskDepth, pNext);
                    AptDecoupleTreeTraversal(pChild, nCurrRenderTick, pRenderingContext, eMaskOperation, nMaskDepth, eAnimLevels, false);
                    PopEffectRenderElement(pRenderItem, pEffect);
                    pRenderItem->PopRenderData(pRenderingContext, eMaskOperation, nMaskDepth);
                }
                else
                {
                    const AptRenderItem *pRenderItem = pNext->GetCharacterInst()->mpRenderItem;
                    pRenderItem->Render(pRenderingContext, eMaskOperation, nMaskDepth, pNext);
                }
            }

            if (pMask != NULL)
            {
                nMaskDepth--;
                const AptRenderItem *pMaskRenderItem = pMask->GetCharacterInst()->mpRenderItem;
                pMaskRenderItem->PushRenderDataAbsolute(pRenderingContext);
                AptAssetEffect pEffect = PushEffectRenderElement(pMask, pMaskRenderItem);
                AptDecoupleMaskedTreeTraversal(pMask, nCurrRenderTick, pRenderingContext, AptMaskRenderOperation_Subtract, nMaskDepth, eAnimLevels, bUseDepthCompare);
                PopEffectRenderElement(pMaskRenderItem, pEffect);
                pMaskRenderItem->PopRenderData(pRenderingContext, AptMaskRenderOperation_Subtract, nMaskDepth);
                pMask = NULL;
            }
        }

        pNext = nextNext;
    }

    if (pMaskRenderItemBegin != NULL) // If we are clipping and this object is not clipped, push out the clipped stuff and continue to render as normal
    {
        nMaskDepth--;
        AptDecoupleTreeTraversalClipper(pMaskRenderItemBegin, nCurrRenderTick, pRenderingContext, AptMaskRenderOperation_Subtract, nMaskDepth, eAnimLevels, bUseDepthCompare, pMaskRenderItemEnd);
    }
}

void AptDecoupleTreeTraversalClipper(const AptCIH *pRenderItemBegin, int32_t nCurrRenderTick, AptRenderingContext *pRenderingContext, AptMaskRenderOperation eMaskOperation, int nMaskDepth, AptAnimLevelE eAnimLevels, bool bUseDepthCompare, const AptCIH *pRenderItemEnd)
{
    // const AptCIH * pChild = pRenderItemBegin->GetFirstChild();
    // if(pChild && pChild->GetCharacterInst()->GetCharacterType() != AptCharacterType_CustomControl)
    //{
    //     const AptRenderItem * pRenderItem = pRenderItemBegin->GetCharacterInst()->mpRenderItem;;
    //     pRenderItem->PushRenderData(pRenderingContext, eMaskOperation, nMaskDepth);
    //     AptDecoupleTreeTraversalClipper(pChild, nCurrRenderTick, pRenderingContext, eMaskOperation, nMaskDepth, eAnimLevels, false, pRenderItemEnd);
    //     pRenderItem->PopRenderData(pRenderingContext, eMaskOperation, nMaskDepth);
    // }
    // else
    //{
    //     const AptRenderItem * pRenderItem = pRenderItemBegin->GetCharacterInst()->mpRenderItem;
    //     pRenderItem->Render(pRenderingContext, eMaskOperation, nMaskDepth, pRenderItemBegin);
    // }

    // if(pRenderItemBegin == pRenderItemEnd)
    //{
    //     return;
    // }

    // AptCIH * pNext = pRenderItemBegin->GetDisplayListNext();

    const AptCIH *pNext = pRenderItemBegin;

    while (pNext)
    {
        const AptCIH *pChild = pNext->GetFirstChild();

        if (pChild && pNext->GetCharacterInst()->GetCharacterType() != AptCharacterType_CustomControl)
        {
            const AptRenderItem *pRenderItem = pNext->GetCharacterInst()->mpRenderItem;
            ;
            pRenderItem->PushRenderData(pRenderingContext, eMaskOperation, nMaskDepth);
            AptAssetEffect pEffect = PushEffectRenderElement(pNext, pRenderItem);
            AptDecoupleTreeTraversalClipper(pChild, nCurrRenderTick, pRenderingContext, eMaskOperation, nMaskDepth, eAnimLevels, false, pRenderItemEnd);
            PopEffectRenderElement(pRenderItem, pEffect);
            pRenderItem->PopRenderData(pRenderingContext, eMaskOperation, nMaskDepth);
        }
        else
        {
            const AptRenderItem *pRenderItem = pNext->GetCharacterInst()->mpRenderItem;
            pRenderItem->Render(pRenderingContext, eMaskOperation, nMaskDepth, pNext);
        }

        if (pNext == pRenderItemEnd)
        {
            break;
        }
        pNext = pNext->GetDisplayListNext();
    }
}

const AptCIH *AptDecoupleClippedTreeTraversal(const AptCIH *pCur, int32_t nCurrRenderTick, AptRenderingContext *pRenderingContext, AptMaskRenderOperation eMaskOperation, int nMaskDepth, AptAnimLevelE eAnimLevels, bool bUseDepthCompare)
{
    const AptCIH *pChild = pCur->GetFirstChild();
    if (pChild && pChild->GetCharacterInst()->GetCharacterType() != AptCharacterType_CustomControl)
    {
        const AptRenderItem *pRenderItem = pCur->GetCharacterInst()->mpRenderItem;
        ;
        pRenderItem->PushRenderData(pRenderingContext, eMaskOperation, nMaskDepth);
        AptAssetEffect pEffect = PushEffectRenderElement(pCur, pRenderItem);
        AptDecoupleTreeTraversalClipper(pChild, nCurrRenderTick, pRenderingContext, eMaskOperation, nMaskDepth, eAnimLevels, false, NULL);
        PopEffectRenderElement(pRenderItem, pEffect);
        pRenderItem->PopRenderData(pRenderingContext, eMaskOperation, nMaskDepth);
    }
    else
    {
        const AptRenderItem *pRenderItem = pCur->GetCharacterInst()->mpRenderItem;
        pRenderItem->Render(pRenderingContext, eMaskOperation, nMaskDepth, pCur);
    }

    AptCIH *pNext             = pCur->GetDisplayListNext();
    const AptCIH *pTmpSibling = pCur;

    while (pNext && pCur->GetCharacterInst()->mpRenderItem->GetClipDepth() < pNext->GetDepth())
    {
        pTmpSibling = pNext;
        pChild      = pNext->GetFirstChild();

        if (pChild && pNext->GetCharacterInst()->GetCharacterType() != AptCharacterType_CustomControl)
        {
            const AptRenderItem *pRenderItem = pNext->GetCharacterInst()->mpRenderItem;
            ;
            pRenderItem->PushRenderData(pRenderingContext, eMaskOperation, nMaskDepth);
            AptAssetEffect pEffect = PushEffectRenderElement(pNext, pRenderItem);
            AptDecoupleTreeTraversalClipper(pChild, nCurrRenderTick, pRenderingContext, eMaskOperation, nMaskDepth, eAnimLevels, false, NULL);
            PopEffectRenderElement(pRenderItem, pEffect);
            pRenderItem->PopRenderData(pRenderingContext, eMaskOperation, nMaskDepth);
        }
        else
        {
            const AptRenderItem *pRenderItem = pNext->GetCharacterInst()->mpRenderItem;
            pRenderItem->Render(pRenderingContext, eMaskOperation, nMaskDepth, pNext);
        }
        pNext = pNext->GetDisplayListNext();
    }
    return pTmpSibling;
}
