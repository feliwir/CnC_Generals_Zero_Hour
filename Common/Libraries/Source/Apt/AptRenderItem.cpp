/*** Include files ********************************************************************************/

#include "_Apt.h"
#include "AptRenderItem.h"
#include "AptCharacterInst.h"
#include "AptCIH.h"
#include "TextFormat.h"
#include "MainInline.h"
#include "Display/AptRenderingContext.h"
#include "AptStd/AptMath.h"
#if !defined(APT_ENABLE_INLINE)
#include "AptRenderItem.inl"
#endif
#include "AptTarget.h"
#include "AptAnimationTarget.h"
#include "AptBCRenderTree.h"
#include "AptRenderList.h"
#include "AptRenderableImage.h"

/*** Defines **************************************************************************************/

/*** Macros ***************************************************************************************/

/*** Type Definitions *****************************************************************************/

/*** Function Prototypes **************************************************************************/
// needed in customcontrol, dynamic text render functions.
extern AptAssetEffect PushEffectRenderElement(const AptCIH *pCIH, const AptRenderItem *pRenderItem);
extern void PopEffectRenderElement(const AptRenderItem *pRenderItem, AptAssetEffect pEffect);

/*** Variables ************************************************************************************/

#if defined(APT_3D)
#endif

// Private variables
#if defined(APT_RI_LIST_VERIFY)
AptRenderItem *AptRenderItem::spHeadItem = NULL;
#endif

uint32_t AptRenderItem::sItemsAllocated = 0;
// Public variables

unsigned AptRenderItem::suCurrentRenderAnimLevel = 0;

//////////////////////////////////////////////////////////////////////////
// $TODO : Remove the AptRenderTree.h inclusion here, this was done for the PushMatricesAbsolute addition
//////////////////////////////////////////////////////////////////////////

/*** Private Functions ****************************************************************************/

static void PushMatrices(AptRenderingContext *pRenderingContext, const AptRenderItem *pItem)
{
    if (AptOptIsEnabled(APT_OPT_SPRTREE))
    {
        AptDisplayList::_drawCharacterInstOpti(pItem);
    }
    else
    {
        pRenderingContext->pushColourTransform();
        pRenderingContext->appendColourTransform(pItem->GetColorMatrixConst());
        pRenderingContext->pushVertexMatrix();
#if !defined(APT_3D)
        pRenderingContext->appendVertexMatrix(pItem->GetPositionMatrixConst());
#else
        AptMath::Mat44T matrix;
        AptMath::MatConvert(&matrix, pItem->GetPositionMatrixConst());
        AptMath::MatRotate3d(&matrix, pItem->GetXRotation(), pItem->GetYRotation(), pItem->GetZPosition(), pItem->GetZScale());
        pRenderingContext->appendVertexMatrix(&matrix);
#endif
    }
}

static void PushMatricesAbsolute(AptRenderingContext *pRenderingContext, const AptRenderItem *pItem)
{
    if (AptOptIsEnabled(APT_OPT_SPRTREE))
    {
        AptDisplayList::_drawCharacterInstAbsoluteOpti(pItem);
    }
    else
    {
        // $TODO - may be we can remove color transform matrix push/append operation as it is not needed for masks and buttons
        pRenderingContext->pushColourTransform();
        pRenderingContext->appendColourTransform(pItem->GetColorMatrixConst());

        pRenderingContext->pushVertexMatrix();
#if !defined(APT_3D)
        pRenderingContext->setVertexMatrix(&gIdentityMatrix);
        pRenderingContext->appendVertexMatrix(pItem->GetMaskPositionMatrixConst());
#else
        AptMath::Mat44T matrix;
        AptMath::MatMakeIdentity(&matrix);
        pRenderingContext->setVertexMatrix(&matrix);
        AptMath::MatConvert(&matrix, pItem->GetMaskPositionMatrixConst());
        pRenderingContext->appendVertexMatrix(&matrix);
#endif
    }
}

static void PopMatrices(AptRenderingContext *pRenderingContext, const AptRenderItem *pItem)
{
    if (AptOptIsEnabled(APT_OPT_SPRTREE))
    {
        AptMath::ClipStackPop();
    }
    else
    {
        pRenderingContext->popColourTransform();
        pRenderingContext->popVertexMatrix();
    }
}

AptRenderItem::AptRenderItem(AptCharacter *pCharacter, int32_t nCurrentTick) : mpCharacter(pCharacter),
                                                                               mpMaskPositionMatrix(NULL),
                                                                               mnDepth(-1),
                                                                               mnClipDepth(-1),
                                                                               mbIsVisible(true),
                                                                               mbIsMask(false),
                                                                               mbHasMask(false),
                                                                               mbIsDeletionMark(false),
                                                                               mbCreatedDynamic(false), /* This will be set later if true. */
                                                                               mbPendingHiddenByTree(0),
                                                                               mbIsHiddenByTree(0),
                                                                               mbRootNonVisibleItem(0),
                                                                               meCharacterType(0),
                                                                               mpMask(NULL),
                                                                               mnReferenceCount(0),
                                                                               mnCreatedOnTick(nCurrentTick)
{
    // $TODO remove setting of identity matrix from here and put it above
    // we will have to define copy constructor for AptMatrix.
    mpPositionMatrix = NULL;
    mpColorMatrix    = NULL;
#if defined(APT_3D)
    mp3DRotation = NULL;
#endif

    if (pCharacter)
    {
        pCharacter->AddCharacterReference();
    }

#if defined(APT_RI_LIST_VERIFY)
    mpNextAllocatedItem = spHeadItem;
    if (spHeadItem)
    {
        spHeadItem->mpPrevAllocatedItem = this;
    }
    mpPrevAllocatedItem = NULL;
    spHeadItem          = this;
#endif

    sItemsAllocated++;
}

AptRenderItem::AptRenderItem(AptRenderItem *pObj, int32_t nCurrentTick, bool bCreateAsNewRevisionOfObj) : mpCharacter(pObj->mpCharacter),
                                                                                                          mpMaskPositionMatrix(NULL),
                                                                                                          mnDepth(-1),
                                                                                                          mnClipDepth(-1),
                                                                                                          mbIsVisible(true),
                                                                                                          mbIsMask(false),
                                                                                                          mbHasMask(false),
                                                                                                          mbIsDeletionMark(false),
                                                                                                          mbCreatedDynamic(pObj->mbCreatedDynamic),
                                                                                                          mbPendingHiddenByTree(0),
                                                                                                          mbIsHiddenByTree(0),
                                                                                                          mbRootNonVisibleItem(0),
                                                                                                          meCharacterType(0),
                                                                                                          mpMask(NULL),
                                                                                                          mnReferenceCount(0),
                                                                                                          mnCreatedOnTick(nCurrentTick)
{
    // $TODO remove setting of identity matrix from here and put it above
    // we will have to define copy constructor for AptMatrix.
    mpPositionMatrix = NULL;
    mpColorMatrix    = NULL;

    if (pObj->mpPositionMatrix != NULL)
    {
        if (mpPositionMatrix == NULL)
            mpPositionMatrix = new AptMatrix();
        mpPositionMatrix->AptMatrixCopy(pObj->mpPositionMatrix);
    }
    else
    {
        if (mpPositionMatrix != NULL)
            mpPositionMatrix->AptMatrixCopy(&gIdentityMatrix);
    }

    if (pObj->mpColorMatrix != NULL)
    {
        if (mpColorMatrix == NULL)
            mpColorMatrix = new AptCXForm(pObj->mpColorMatrix);
        // mpColorMatrix->AptCXFormCopy(pObj->mpColorMatrix);
    }
    else
    {
        if (mpColorMatrix != NULL)
            mpColorMatrix->AptCXFormCopy(&gIdentityCXForm);
    }
#if defined(APT_3D)
    mp3DRotation = NULL;
    if (pObj->mp3DRotation != NULL)
    {
        mp3DRotation = new AptRI3DHelper();
        mp3DRotation->AptRI3DHelperCopy(pObj->mp3DRotation);
    }
#endif
    if (bCreateAsNewRevisionOfObj)
    {
        // Copy over the rest of the items.
        mnDepth     = pObj->mnDepth;
        mnClipDepth = pObj->mnClipDepth;
        mbIsVisible = pObj->mbIsVisible;
        mbIsMask    = pObj->mbIsMask;
        mbHasMask   = pObj->mbHasMask;
        SetMaskMatrix(pObj->mpMaskPositionMatrix);
        mbIsDeletionMark = pObj->mbIsDeletionMark; // $TODO: pObj->mbIsDeletionMark should never be true... assert here
        // mnReferenceCount= 0; // Each Item gets it's own reference count.

        if (mpMask != NULL)
        {
            APT_RI_INC(mpMask, this, "mpMask");
        }

        // If creating a new revision of a "pending force Writable" object set the force writable.
        if (pObj->mbPendingHiddenByTree)
        {
            mbIsHiddenByTree = 1;
        }

        // Propagate this to the next revision
        mbRootNonVisibleItem = pObj->mbRootNonVisibleItem;

        APT_ASSERT(pObj->mbIsHiddenByTree == 0);
    }

    if (mpCharacter != NULL)
    {
        mpCharacter->AddCharacterReference();
    }

#if defined(APT_RI_LIST_VERIFY)
    // $TODO - can use atomic read modify write for spHead.
    mpNextAllocatedItem = spHeadItem;
    if (spHeadItem)
    {
        spHeadItem->mpPrevAllocatedItem = this;
    }
    mpPrevAllocatedItem = NULL;
    spHeadItem          = this;
#endif

    sItemsAllocated++;
}

#if defined(APT_RI_LIST_VERIFY)
static bool gbRenderItemShuttingDown = false;
#endif // APT_RI_LIST_VERIFY

AptRenderItem::~AptRenderItem()
{
    if (mpCharacter != NULL)
    {
        mpCharacter->ReleaseCharacterReference();
    }

    mpCharacter = NULL;

    if (mpMask != NULL)
    {
        APT_RI_DEC(mpMask, this, "mpMask");
    }
    if (mpMaskPositionMatrix != NULL)
    {
        delete mpMaskPositionMatrix;
        mpMaskPositionMatrix = NULL;
    }

    if (mpPositionMatrix != NULL)
    {
        delete mpPositionMatrix;
        mpPositionMatrix = NULL;
    }

    if (mpColorMatrix != NULL)
    {
        delete mpColorMatrix;
        mpColorMatrix = NULL;
    }
#if defined(APT_3D)
    if (mp3DRotation != NULL)
    {
        delete mp3DRotation;
        mp3DRotation = NULL;
    }
#endif

#if defined(APT_RI_LIST_VERIFY)
    // $TODO - can use atomic read modify write for spHead.

    if (mpNextAllocatedItem)
    {
        mpNextAllocatedItem->mpPrevAllocatedItem = mpPrevAllocatedItem;
    }
    if (mpPrevAllocatedItem)
    {
        mpPrevAllocatedItem->mpNextAllocatedItem = mpNextAllocatedItem;
    }
    else if (spHeadItem == this)
    {
        spHeadItem = mpNextAllocatedItem;
    }
#endif

    sItemsAllocated--;
}

/*** Public Functions *****************************************************************************/
#if defined(APT_3D)
//// Getter and Setter methods for the 3D variables
float AptRenderItem::GetZPosition() const
{
    if (mp3DRotation == NULL)
        return 0.f;
    return mp3DRotation->GetZPosition();
}

float AptRenderItem::GetZScale() const
{
    if (mp3DRotation == NULL)
        return 1.f;
    return mp3DRotation->GetZScale();
}

float AptRenderItem::GetYRotation() const
{
    if (mp3DRotation == NULL)
        return 0.f;
    return mp3DRotation->GetYRotation();
}

float AptRenderItem::GetXRotation() const
{
    if (mp3DRotation == NULL)
        return 0.f;
    return mp3DRotation->GetXRotation();
}

void AptRenderItem::SetZPosition(float zPosition)
{
    if (mp3DRotation == NULL)
        mp3DRotation = new AptRI3DHelper();
    mp3DRotation->SetZPosition(zPosition);
}

void AptRenderItem::SetZScale(float zScale)
{
    if (mp3DRotation == NULL)
        mp3DRotation = new AptRI3DHelper();
    mp3DRotation->SetZScale(zScale);
}

void AptRenderItem::SetXRotation(float xRot)
{
    if (mp3DRotation == NULL)
        mp3DRotation = new AptRI3DHelper();
    mp3DRotation->SetXRotation(xRot);
}

void AptRenderItem::SetYRotation(float yRot)
{
    if (mp3DRotation == NULL)
        mp3DRotation = new AptRI3DHelper();
    mp3DRotation->SetYRotation(yRot);
}
#endif

AptMatrix *AptRenderItem::GetPositionMatrixWritable()
{
    if (mpPositionMatrix == NULL)
    {
        mpPositionMatrix = new AptMatrix();
        mpPositionMatrix->AptMatrixCopy(&gIdentityMatrix);
        return mpPositionMatrix;
    }
    return mpPositionMatrix;
}

AptCXForm *AptRenderItem::GetColorMatrixWritable()
{
    if (mpColorMatrix == NULL)
    {
        mpColorMatrix = new AptCXForm(&gIdentityCXForm);
        // mpColorMatrix->AptCXFormCopy(&gIdentityCXForm);
        return mpColorMatrix;
    }
    return mpColorMatrix;
}

void AptRenderItem::SetIsVisible(bool newState)
{
    if (newState == (mbIsVisible ? 1 : 0))
    {
        return;
    }

    if (newState)
    {
        // We are becoming visible
        // if we were a root, make our children renderable again.
        if (mbRootNonVisibleItem)
        {
            mbRootNonVisibleItem = 0; // We are visible now, so no longer a root non-visible item
            if (!GetIsMask())
                PropagateTreeIsVisible(true); // Propagate the visible flag down.
        }
    }
    else
    {
        // We are losing visibility
        // If we are already invisible because of a parent, then there is nothing to do.
        if (GetIsHiddenByTree() == false /* If the flag is not set */)
        { /* then we need to set it */

            mbRootNonVisibleItem = 1; // In this case we are now a root non-visible item.
            SetIsHiddenByTree(false); // We are now hidden by a tree
            if (!GetIsMask())
                PropagateTreeIsVisible(false); // Propagate the visible flag down.
        }
    }

    mbIsVisible = newState ? 1 : 0;
}

void AptRenderItem::PropagateTreeIsVisible(bool bTreeIsNowVisible) const
{
}
#if defined(APT_RI_LIST_VERIFY)

void AptRenderItem::VerifyItemsBeforeRender(int32_t nCurrentRenderTick)
{
}

static bool sDoVerifyItem = false;
void AptRenderItem::VerifyItemsAfterRender(int32_t nCurrentRenderTick)
{
    if (!sDoVerifyItem)
    {
        return;
    }

    const AptRenderItem *pTemp = spHeadItem;

    static bool sDoPrintTrees = false;
    // $TODO - this can be platform specific
    FILE *fdot = NULL;
    if (sDoPrintTrees)
    {
        char szName[512];
#if !defined(APT_PLATFORM_XENON)
        sprintf(szName, "C:\\packages\\GraphViz\\dump-%8.8x.dot", nCurrentRenderTick);
#else
        sprintf(szName, "d:\\dump-%8.8x.dot", nCurrentRenderTick);
#endif
        fdot = fopen(szName, "w");
        if (!fdot)
        {
            APT_DEBUGPRINT(APT_DEBUG_MSG_WARNING_LVL, "\nCould not Open file!!!!\n");
            return;
        }

        fprintf(fdot, "digraph states {\n");
    }

    bool errorsFound = false;
    while (pTemp != NULL)
    {
        bool ThisObjBad = false;
        if (pTemp->mpNextRevision != NULL)
        {
            const AptRenderItem *pNextRev = pTemp->mpNextRevision;
            AptGetUserFuncs().pfnDebugPrint("!@!@%%%: Stale Render Item Not removed by Render call Current Tick:%x: %x-t%x -> NxRev:%x-t%x\n",
                                    nCurrentRenderTick, pTemp, pTemp->mnCreatedOnTick, pNextRev, pNextRev->mnCreatedOnTick);
            errorsFound = true;
            ThisObjBad  = true;
        }

        if (sDoPrintTrees)
        {
            const char *pFilename = NULL;
            if (pTemp->mpCharacter && pTemp->mpCharacter->m_pAnimFile)
                pFilename = pTemp->mpCharacter->m_pAnimFile->GetName().c_str();

            if (ThisObjBad)
            {
                fprintf(fdot, "i%p [label=\"%p\\nType:%d\\nTick%8.8X\\nVisible:%s\\nmbPendingHiddenByTree:%s\\nmbIsHiddenByTree:%s\\nmbRootNonVisibleItem:%s\\nDepth:%d\\nClip:%d\\nRefCount:%d\\nFilename:%s\" peripheries=3,color=red];",
                        pTemp, pTemp, pTemp->GetCharacterType(), pTemp->mnCreatedOnTick, pTemp->mbIsVisible ? "true" : "false", pTemp->mbPendingHiddenByTree ? "true" : "false", pTemp->mbIsHiddenByTree ? "true" : "false", pTemp->mbRootNonVisibleItem ? "true" : "false", pTemp->mnDepth, pTemp->mnClipDepth, pTemp->mnReferenceCount, pFilename);
            }
            else
            {
                fprintf(fdot, "i%p [label=\"%p\\nType:%d\\nTick%8.8X\\nVisible:%s\\nmbPendingHiddenByTree:%s\\nmbIsHiddenByTree:%s\\nmbRootNonVisibleItem:%s\\nDepth:%d\\nClip:%d\\nRefCount:%d\\nFilename:%s\"];",
                        pTemp, pTemp, pTemp->GetCharacterType(), pTemp->mnCreatedOnTick, pTemp->mbIsVisible ? "true" : "false", pTemp->mbPendingHiddenByTree ? "true" : "false", pTemp->mbIsHiddenByTree ? "true" : "false", pTemp->mbRootNonVisibleItem ? "true" : "false", pTemp->mnDepth, pTemp->mnClipDepth, pTemp->mnReferenceCount, pFilename);
            }

            if (pTemp->mpNextRevision != NULL)
                fprintf(fdot, "i%p -> i%p [label=\"%s\" color=black];", pTemp, pTemp->mpNextRevision, "mpNextRevision");
            if (pTemp->mpFirstChild != NULL)
                fprintf(fdot, "i%p -> i%p [label=\"%s\" color=red];", pTemp, pTemp->mpFirstChild, "mpFirstChild");
            if (pTemp->mpNextSibling != NULL)
                fprintf(fdot, "i%p -> i%p [label=\"%s\" color=blue];", pTemp, pTemp->mpNextSibling, "mpNextSibling");
            if (pTemp->mpMask != NULL)
                fprintf(fdot, "i%p -> i%p [label=\"%s\" color=green];", pTemp, pTemp->mpMask, "mpMask");
        }
        pTemp = pTemp->mpNextAllocatedItem;
    }

    if (sDoPrintTrees)
    {
        fprintf(fdot, "}");
        fclose(fdot);
    }
}
#endif // end of #if defined(APT_RI_LIST_VERIFY)

AptRenderItem *AptRenderItem::Manager_CreateItem(AptCharacter *pCharacter, int32_t nCurrentUpdateTick)
{
    // Need to create a Render Item based on the passed pCharacter.
    AptRenderItem *pInst = NULL;

    if (pCharacter == NULL) // create a renderItemlevel
    {
        pInst = new AptRenderItemLevel(NULL, nCurrentUpdateTick);
        return pInst;
    }

    switch (pCharacter->eType)
    {
    case AptCharacterType_Sprite:
        pInst = new AptRenderItemSprite(pCharacter, nCurrentUpdateTick);
        break;
    case AptCharacterType_Text:
        pInst = new AptRenderItemDynamicText(pCharacter, nCurrentUpdateTick);
        break;
    case AptCharacterType_StaticText:
        pInst = new AptRenderItemStaticText(pCharacter, nCurrentUpdateTick);
        break;
    case AptCharacterType_Shape:
        pInst = new AptRenderItemShape(pCharacter, nCurrentUpdateTick);
        break;
    case AptCharacterType_Morph:
        pInst = new AptRenderItemMorph(pCharacter, nCurrentUpdateTick);
        break;
    case AptCharacterType_Animation:
        pInst = new AptRenderItemAnimation(pCharacter, nCurrentUpdateTick);
        break;
    case AptCharacterType_Image:
        pInst = new AptRenderItemLoadedTexture(pCharacter, nCurrentUpdateTick);
        break;

    case AptCharacterType_Button:
    {
#if defined APT_USE_BUTTONS
        pInst = new AptRenderItemButton(pCharacter, nCurrentUpdateTick);
#endif
        break;
    }

    default:
    {
        APT_FAIL("Uncertain what kind of RenderItem to create");
        break;
    }
    }
    return pInst;
}

bool AptRenderItem::CleanUp()
{
#if defined(APT_RI_LIST_VERIFY)
    if (spHeadItem != NULL)
    {
        printf("AptRenderItem::CleanUp freeing items");
    }

    gbRenderItemShuttingDown = true;

    AptRenderItem *pTempNext = spHeadItem;
    while (pTempNext)
    {
        delete pTempNext;
        pTempNext = spHeadItem;
    }
    gbRenderItemShuttingDown = false;
#endif // APT_RI_LIST_VERIFY
    return true;
}

AptRenderItem *AptRenderItem::Manager_CloneNewItem(int32_t nCurrentUpdateTick)
{
    // Should not clone a deleted object.
    APT_ASSERT(!mbIsDeletionMark);

    AptRenderItem *pTmp = Clone(nCurrentUpdateTick, false);
    return pTmp;
}

void AptRenderItem::SetCharacter(AptCharacter *pCharacter)
{
    APT_ASSERT(AptGetLib()->mnCurrUpdateTick == mnCreatedOnTick);
    if (pCharacter != mpCharacter)
    {
        if (pCharacter)
            pCharacter->AddCharacterReference();
        if (mpCharacter)
            mpCharacter->ReleaseCharacterReference();
        mpCharacter = pCharacter;
    }
}

void AptRenderItem::SetIsMask(bool bIsMask, AptMatrix *pMatrix)
{
    if (bIsMask == (mbIsMask ? true : false) && mpMaskPositionMatrix == pMatrix)
    {
        return;
    }

    if (bIsMask == false)
    {
        // Mask is no longer a mask
        if (mpMaskPositionMatrix != NULL)
        {
            delete mpMaskPositionMatrix;
        }
        mpMaskPositionMatrix = NULL;
    }
    else
    {
        SetMaskMatrix(pMatrix);
    }

    // Visible state will be set by SetHasMask

    mbIsMask = bIsMask ? 1 : 0;
}

void AptRenderItem::Manager_SetMask(const AptRenderItem *pNew)
{
    APT_ASSERT(pNew != this);
    // APT_ASSERT(pNew == NULL || !pNew->mbIsDeletionMark);

    if (pNew != mpMask)
    {
        if (pNew != NULL)
            APT_RI_INC(pNew, this, "mpMask");
        if (mpMask != NULL)
            APT_RI_DEC(mpMask, this, "mpMask");

        // Didn't modify the state, and anyone who would modify the state would have to get a writable pointer.
        // Thus the const is casted off here.
        mpMask = (AptRenderItem *)pNew;
    }
}

void AptRenderItem::Manager_SetDeletionMark(bool bNewValue)
{
    // APT_ASSERT(mbIsDeletionMark == false);

    if (bNewValue)
    {
        if (mpMask)
        {
            APT_RI_DEC(mpMask, this, "mpMask");
        }
    }

    mbIsDeletionMark = bNewValue;
}

void AptRenderItem::Manager_UpdateMask(AptRenderItem *pNew) const
{
    APT_ASSERT(pNew != this);

    // Don't return deleted items. they are here as placeholders.
    if (pNew != NULL && pNew->mbIsDeletionMark)
    {
        pNew = NULL;
    }

    if (pNew != mpMask)
    {
        if (pNew != NULL)
        {
            APT_RI_INC(pNew, this, "mpMask");
        }
        if (mpMask)
        {
            APT_RI_DEC(mpMask, this, "mpMask");
        }
        mpMask = pNew;
        APT_ASSERT((mbHasMask == 0 || (mbHasMask == 1 && mpMask != NULL)) && "How can I say I have a mask, but not really have one?");
    }
}

void AptRenderItem::PushRenderData(AptRenderingContext *pRenderingContext, AptMaskRenderOperation eMaskOperation, int32_t nMaskDepth) const
{
    // Base class has no children.
    APT_ASSERT(false && "Attempting to render children of an object with none!");
}

void AptRenderItem::PushRenderDataAbsolute(AptRenderingContext *pRenderingContext) const
{
    // Base class has no children.
    APT_ASSERT(false && "Attempting to render children of an object with none!");
}

void AptRenderItem::PopRenderData(AptRenderingContext *pRenderingContext, AptMaskRenderOperation eMaskOperation, int32_t nMaskDepth) const
{
    // Base class has no children.
    APT_ASSERT(false && "Attempting to render children of an object with none!");
}

void AptRenderItem::Render(AptRenderingContext *pRenderingContext, AptMaskRenderOperation eMaskOperation, int32_t nMaskDepth, const AptCIH *pCIH) const
{
}

void AptRenderItem::SetHasMask(bool newState, AptRenderItem *pMask)
{
    mbHasMask = newState ? 1 : 0;
    if (mpMask != pMask)
    {
        if (pMask != NULL)
        {
            APT_RI_INC(pMask, this, "mpMask");

            if (GetIsVisible() == false || GetIsHiddenByTree() == true)
            {
                // Masked item is NOT visible
                pMask->mbRootNonVisibleItem = 0;      // Mask is not root non visible
                pMask->SetIsHiddenByTree(true);       // Mask is hidden by Masked item
                pMask->PropagateTreeIsVisible(false); // Update Mask branch not visible
            }
            else
            {
                // Masked item is visible
                pMask->mbRootNonVisibleItem = 0;     // Mask is not root non vis
                pMask->SetIsHiddenByTree(false);     // Mask is not hidden by Masked item
                pMask->PropagateTreeIsVisible(true); // Update Mask branch visible
            }
        }
        if (mpMask != NULL)
        {
            APT_RI_DEC(mpMask, this, "mpMask");
        }
        mpMask = pMask;
    }
    if (mpMask == NULL)
    {
        mbHasMask = 0;
    }
    APT_ASSERT((mbHasMask == 0 || (mbHasMask == 1 && mpMask != NULL)) && "How can I say I have a mask, but not really have one?");
}

void AptRenderItem::SetMaskMatrix(AptMatrix *pMatrix)
{
    if (mpMaskPositionMatrix == NULL && pMatrix != NULL)
    {
        mpMaskPositionMatrix = new AptMatrix();
        mpMaskPositionMatrix->AptMatrixCopy(pMatrix);
    }
    else if (mpMaskPositionMatrix != NULL && pMatrix != NULL)
    {
        mpMaskPositionMatrix->AptMatrixCopy(pMatrix);
    }
    else if (mpMaskPositionMatrix != NULL && pMatrix == NULL)
    {
        delete mpMaskPositionMatrix;
        mpMaskPositionMatrix = NULL;
    }
}

void AptRenderItem::CopyRenderDataFrom(const AptRenderItem *pModel)
{
    if (pModel->mpPositionMatrix != NULL)
    {
        if (mpPositionMatrix == NULL)
            mpPositionMatrix = new AptMatrix();
        mpPositionMatrix->AptMatrixCopy(pModel->mpPositionMatrix);
    }
    else
    {
        if (mpPositionMatrix != NULL)
            mpPositionMatrix->AptMatrixCopy(&gIdentityMatrix);
    }

    if (pModel->mpColorMatrix != NULL)
    {
        if (mpColorMatrix == NULL)
            mpColorMatrix = new AptCXForm(pModel->mpColorMatrix);
        // mpColorMatrix->AptCXFormCopy(pModel->mpColorMatrix);
    }
    else
    {
        if (mpColorMatrix != NULL)
            mpColorMatrix->AptCXFormCopy(&gIdentityCXForm);
    }
    mnClipDepth = pModel->mnClipDepth;
    mbIsVisible = pModel->mbIsVisible;
}

AptRenderItemLevel::AptRenderItemLevel(AptCharacter *pCharacter, int32_t nCurrentUpdateTick) : AptRenderItem(pCharacter, nCurrentUpdateTick)
{
    SetCharacterType(AptCharacterType_Level);
}

AptRenderItemLevel::AptRenderItemLevel(class AptRenderItemLevel *pObj, int32_t nCurrentUpdateTick, bool bCreateAsNewRevisionOfObj) : AptRenderItem(pObj, nCurrentUpdateTick, bCreateAsNewRevisionOfObj)
{
    SetCharacterType(AptCharacterType_Level);
}

AptRenderItemSprite::AptRenderItemSprite(AptCharacter *pCharacter, int32_t nCurrentUpdateTick) : AptRenderItem(pCharacter, nCurrentUpdateTick), mnBlendMode(0)
{
    SetCharacterType(AptCharacterType_Sprite);
}

void AptRenderItemSprite::PushRenderData(AptRenderingContext *pRenderingContext, AptMaskRenderOperation eMaskOperation, int32_t nMaskDepth) const
{
#if defined(APT_RENDER_FLAGS)
    APT_ASSERT(AptGetUserFuncs().pfnPushRenderFlags != NULL && "pfnPushRenderFlags callback not set");
    if (!msRenderFlags.IsEmpty() && AptGetUserFuncs().pfnPushRenderFlags)
        AptGetUserFuncs().pfnPushRenderFlags(msRenderFlags.c_str());
#endif
    PushMatrices(pRenderingContext, this);
}

void AptRenderItemSprite::PushRenderDataAbsolute(AptRenderingContext *pRenderingContext) const
{
#if defined(APT_RENDER_FLAGS)
    APT_ASSERT(AptGetUserFuncs().pfnPushRenderFlags != NULL && "pfnPushRenderFlags callback not set");
    if (!msRenderFlags.IsEmpty() && AptGetUserFuncs().pfnPushRenderFlags)
        AptGetUserFuncs().pfnPushRenderFlags(msRenderFlags.c_str());
#endif
    PushMatricesAbsolute(pRenderingContext, this);
}

void AptRenderItemSprite::PopRenderData(AptRenderingContext *pRenderingContext, AptMaskRenderOperation eMaskOperation, int32_t nMaskDepth) const
{
    PopMatrices(pRenderingContext, this);
#if defined(APT_RENDER_FLAGS)
    APT_ASSERT(AptGetUserFuncs().pfnPopRenderFlags != NULL && "pfnPopRenderFlags callback not set");
    if (!msRenderFlags.IsEmpty() && AptGetUserFuncs().pfnPopRenderFlags)
        AptGetUserFuncs().pfnPopRenderFlags(msRenderFlags.c_str());
#endif
}

AptRenderItemSprite::AptRenderItemSprite(class AptRenderItemSprite *pObj, int32_t nCurrentUpdateTick, bool bCreateAsNewRevisionOfObj) : AptRenderItem(pObj, nCurrentUpdateTick, bCreateAsNewRevisionOfObj)
#if defined(APT_RENDER_FLAGS)
                                                                                                                                        ,
                                                                                                                                        msRenderFlags(pObj->msRenderFlags)
#endif
{
    mnBlendMode = pObj->mnBlendMode;
    SetCharacterType(AptCharacterType_Sprite);
    if (GetCreatedDynamic())
    {
        mpCharacter = AptCharacterHelper::GetAptMovieCharacter();
    }
}

AptRenderItemAnimation::AptRenderItemAnimation(AptCharacter *pCharacter, int32_t nCurrentUpdateTick) : AptRenderItemSprite(pCharacter, nCurrentUpdateTick)
{
    SetCharacterType(AptCharacterType_Animation);
}

AptRenderItemAnimation::AptRenderItemAnimation(class AptRenderItemAnimation *pObj, int32_t nCurrentUpdateTick, bool bCreateAsNewRevisionOfObj) : AptRenderItemSprite(pObj, nCurrentUpdateTick, bCreateAsNewRevisionOfObj)
{
    SetCharacterType(AptCharacterType_Animation);
}

AptRenderItemCustomControl::AptRenderItemCustomControl(AptCharacter *pCharacter, int32_t nCurrentUpdateTick)
    : AptRenderItemSprite(pCharacter, nCurrentUpdateTick)
#if defined(APT_CUSTOM_CONTROL_USE_ZID)
      ,
      mpCustomControlRender(new AptRenderableCustomControl), mStoredZid(NULL)
#endif
{
    SetCharacterType(AptCharacterType_CustomControl);
}

AptRenderItemCustomControl::AptRenderItemCustomControl(class AptRenderItemCustomControl *pObj, int32_t nCurrentUpdateTick, bool bCreateAsNewRevisionOfObj)
    : AptRenderItemSprite(pObj, nCurrentUpdateTick, bCreateAsNewRevisionOfObj), m_szType(pObj->m_szType), m_szTarget(pObj->m_szTarget), m_szProperties(pObj->m_szProperties)
#if defined(APT_CUSTOM_CONTROL_USE_ZID)
      ,
      mpCustomControlRender(new AptRenderableCustomControl), mStoredZid(NULL)
#endif
{
    SetCharacterType(AptCharacterType_CustomControl);
}

AptRenderItemCustomControl::AptRenderItemCustomControl(class AptRenderItemSprite *pObj, int32_t nCurrentUpdateTick, bool bCreateAsNewRevisionOfObj)
    : AptRenderItemSprite(pObj, nCurrentUpdateTick, bCreateAsNewRevisionOfObj)
#if defined(APT_CUSTOM_CONTROL_USE_ZID)
      ,
      mpCustomControlRender(new AptRenderableCustomControl), mStoredZid(NULL)
#endif
{
    SetCharacterType(AptCharacterType_CustomControl);
}

void AptRenderItemCustomControl::PushRenderData(AptRenderingContext *pRenderingContext, AptMaskRenderOperation eMaskOperation, int32_t nMaskDepth) const
{
#if defined(APT_RENDER_FLAGS)
    APT_ASSERT(AptGetUserFuncs().pfnPushRenderFlags != NULL && "pfnPushRenderFlags callback not set");
    if (!msRenderFlags.IsEmpty() && AptGetUserFuncs().pfnPushRenderFlags)
        AptGetUserFuncs().pfnPushRenderFlags(msRenderFlags.c_str());
#endif
}

void AptRenderItemCustomControl::PushRenderDataAbsolute(AptRenderingContext *pRenderingContext) const
{
#if defined(APT_RENDER_FLAGS)
    APT_ASSERT(AptGetUserFuncs().pfnPushRenderFlags != NULL && "pfnPushRenderFlags callback not set");
    if (!msRenderFlags.IsEmpty() && AptGetUserFuncs().pfnPushRenderFlags)
        AptGetUserFuncs().pfnPushRenderFlags(msRenderFlags.c_str());
#endif
}

void AptRenderItemCustomControl::PopRenderData(AptRenderingContext *pRenderingContext, AptMaskRenderOperation eMaskOperation, int32_t nMaskDepth) const
{
#if defined(APT_RENDER_FLAGS)
    APT_ASSERT(AptGetUserFuncs().pfnPopRenderFlags != NULL && "pfnPopRenderFlags callback not set");
    if (!msRenderFlags.IsEmpty() && AptGetUserFuncs().pfnPopRenderFlags)
        AptGetUserFuncs().pfnPopRenderFlags(msRenderFlags.c_str());
#endif
}

AptRenderItemCustomControl::~AptRenderItemCustomControl()
{
#if defined(APT_CUSTOM_CONTROL_USE_ZID)
    mpCustomControlRender = SafeRelease(mpCustomControlRender);
    mpCustomControlRender = NULL;
    mStoredZid            = NULL;
#endif
}

AptRenderItemButton::AptRenderItemButton(AptCharacter *pCharacter, int32_t nCurrentUpdateTick) : AptRenderItemSprite(pCharacter, nCurrentUpdateTick)
{
    SetCharacterType(AptCharacterType_Button);
}

AptRenderItemButton::AptRenderItemButton(class AptRenderItemButton *pObj, int32_t nCurrentUpdateTick, bool bCreateAsNewRevisionOfObj) : AptRenderItemSprite(pObj, nCurrentUpdateTick, bCreateAsNewRevisionOfObj)
{
    SetCharacterType(AptCharacterType_Button);
}

AptRenderItemDynamicText::AptRenderItemDynamicText(AptCharacter *pCharacter, int32_t nCurrentUpdateTick) : AptRenderItem(pCharacter, nCurrentUpdateTick),
                                                                                                           mTextValue(),
                                                                                                           mVarValue(),
                                                                                                           mpRenderString(0),
                                                                                                           mnColour(pCharacter->text.nColour),
                                                                                                           mnScroll(1),
                                                                                                           mnBackColor(0x00ffffff),
                                                                                                           mbBackground(0),
                                                                                                           meAlignment(pCharacter->text.eAlignment),
                                                                                                           mnBorderColor(0x00000000),
                                                                                                           mbBorder(0),
                                                                                                           mbMouseWheelEnabled(AptGetLib()->mbDefaultMouseWheel),
                                                                                                           meBoxAlignment(AptStringAlignment_None),
                                                                                                           mbWordWrap(0),
                                                                                                           mbMultiline(0),
                                                                                                           mrBounds(),
                                                                                                           mfFontSize(pCharacter->text.fFontHeight),
                                                                                                           mnFontID(pCharacter->text.nFontID),
                                                                                                           mpMyTextFormat(NULL),
                                                                                                           meFlags(APT_TEXTFIELD_DIRTY),
                                                                                                           mnBlendMode(0),
                                                                                                           mnLeading(TextFormat::UNDEFINED_LEADING_VALUE),
                                                                                                           mnTracking(TextFormat::UNDEFINED_TRACKING_VALUE)
{
    SetCharacterType(AptCharacterType_Text);
    // ### updated to use the new rBounds struct
    mrBounds.fBottom = pCharacter->text.rBounds.fBottom;
    mrBounds.fLeft   = pCharacter->text.rBounds.fLeft;
    mrBounds.fRight  = pCharacter->text.rBounds.fRight;
    mrBounds.fTop    = pCharacter->text.rBounds.fTop;
}

AptRenderItemDynamicText::AptRenderItemDynamicText(class AptRenderItemDynamicText *pObj, int32_t nCurrentUpdateTick, bool bCreateAsNewRevisionOfObj) : AptRenderItem(pObj, nCurrentUpdateTick, bCreateAsNewRevisionOfObj),
                                                                                                                                                       mTextValue(pObj->mTextValue),
                                                                                                                                                       mVarValue(pObj->mVarValue),
                                                                                                                                                       mpRenderString(0),
                                                                                                                                                       mnColour(pObj->mnColour),
                                                                                                                                                       mnScroll(pObj->mnScroll),
                                                                                                                                                       mnBackColor(pObj->mnBackColor),
                                                                                                                                                       mbBackground(pObj->mbBackground),
                                                                                                                                                       meAlignment(pObj->meAlignment),
                                                                                                                                                       mnBorderColor(pObj->mnBorderColor),
                                                                                                                                                       mbBorder(pObj->mbBorder),
                                                                                                                                                       mbMouseWheelEnabled(pObj->mbMouseWheelEnabled),
                                                                                                                                                       meBoxAlignment(pObj->meBoxAlignment),
                                                                                                                                                       mbWordWrap(pObj->mbWordWrap),
                                                                                                                                                       mbMultiline(pObj->mbMultiline),
                                                                                                                                                       mrBounds(pObj->mrBounds),
                                                                                                                                                       mfFontSize(pObj->mfFontSize),
                                                                                                                                                       mnFontID(pObj->mnFontID),
                                                                                                                                                       mpMyTextFormat(NULL),
                                                                                                                                                       meFlags(APT_TEXTFIELD_DIRTY),
                                                                                                                                                       mnBlendMode(pObj->mnBlendMode),
                                                                                                                                                       mnLeading(pObj->mnLeading),
                                                                                                                                                       mnTracking(pObj->mnTracking)
{
    SetCharacterType(AptCharacterType_Text);

    if (pObj->mpMyTextFormat != NULL)
    {
        mpMyTextFormat = new TextFormat(pObj->mpMyTextFormat);
    }

    if (GetCreatedDynamic())
    {
        mpCharacter = AptCharacterHelper::GetAptTextCharacter();
    }
}

AptRenderItemDynamicText::~AptRenderItemDynamicText()
{
    // If there was a setTextFormat call done, delete the pMyTextFormat object
    if (mpMyTextFormat != NULL)
    {
        delete mpMyTextFormat;
        mpMyTextFormat = NULL;
    }

    if (mpRenderString)
        mpRenderString->Release();
    mpRenderString = NULL;
}

AptRenderItemStaticText::AptRenderItemStaticText(AptCharacter *pCharacter, int32_t nCurrentUpdateTick) : AptRenderItem(pCharacter, nCurrentUpdateTick)
{
    SetCharacterType(AptCharacterType_StaticText);
}

AptRenderItemStaticText::AptRenderItemStaticText(class AptRenderItemStaticText *pObj, int32_t nCurrentUpdateTick, bool bCreateAsNewRevisionOfObj) : AptRenderItem(pObj, nCurrentUpdateTick, bCreateAsNewRevisionOfObj)
{
    SetCharacterType(AptCharacterType_StaticText);
}

AptRenderItemMorph::AptRenderItemMorph(AptCharacter *pCharacter, int32_t nCurrentUpdateTick) : AptRenderItem(pCharacter, nCurrentUpdateTick)
{
    fRatio = 1.f;
    SetCharacterType(AptCharacterType_Morph);
}

AptRenderItemMorph::AptRenderItemMorph(class AptRenderItemMorph *pObj, int32_t nCurrentUpdateTick, bool bCreateAsNewRevisionOfObj) : AptRenderItem(pObj, nCurrentUpdateTick, bCreateAsNewRevisionOfObj)
{
    fRatio = 1.f;
    SetCharacterType(AptCharacterType_Morph);
}

AptRenderItemLoadedTexture::AptRenderItemLoadedTexture(AptCharacter *character, int currentUpdateTick)
    : AptRenderItem(character, currentUpdateTick), mLoadedTextureRenderable(NULL)
{
    SetCharacterType(AptCharacterType_Image);
#if defined(APT_DECOUPLED_RENDERING)
    mLoadedTextureRenderable = new AptRenderableImage(character->image.texture, character->m_pAnimFile);
#else
    mLoadedTextureRenderable = new AptRenderableImage(character->image.texture, AptFilePtr(0));
#endif
}

AptRenderItemLoadedTexture::AptRenderItemLoadedTexture(AptRenderItemLoadedTexture *obj, int currentUpdateTick, bool createAsNewRevisionOfObj)
    : AptRenderItem(obj, currentUpdateTick, createAsNewRevisionOfObj)
{
    SetCharacterType(AptCharacterType_Image);
#if defined(APT_DECOUPLED_RENDERING)
    mLoadedTextureRenderable = new AptRenderableImage(obj->GetCharacterConst()->image.texture, obj->GetCharacterConst()->m_pAnimFile);
#else
    mLoadedTextureRenderable = new AptRenderableImage(obj->GetCharacterConst()->image.texture, AptFilePtr(0));
#endif
}

AptRenderItemLoadedTexture::~AptRenderItemLoadedTexture()
{
    SafeRelease(mLoadedTextureRenderable);
    mLoadedTextureRenderable = NULL;
}

void AptRenderItemLoadedTexture::Render(AptRenderingContext *renderingContext, AptMaskRenderOperation maskOperation, int maskDepth, const AptCIH *cIH) const
{

    PushMatrices(renderingContext, this);

    AptRenderInfo info;
    info.mnLevel     = AptRenderItem::suCurrentRenderAnimLevel;
    info.mTransform  = *AptMath::_ClipStackGetTop();
    info.meMaskOper  = maskOperation;
    info.mnMaskDepth = maskDepth;
    AptRenderList::Add(info, this, mLoadedTextureRenderable);

    PopMatrices(renderingContext, this);
}

AptRenderItemShape::AptRenderItemShape(AptCharacter *pCharacter, int32_t nCurrentUpdateTick) : AptRenderItem(pCharacter, nCurrentUpdateTick)
{
    SetCharacterType(AptCharacterType_Shape);
}

AptRenderItemShape::AptRenderItemShape(class AptRenderItemShape *pObj, int32_t nCurrentUpdateTick, bool bCreateAsNewRevisionOfObj) : AptRenderItem(pObj, nCurrentUpdateTick, bCreateAsNewRevisionOfObj)
{
    SetCharacterType(AptCharacterType_Shape);
}

void AptRenderItemSprite::Render(AptRenderingContext *pRenderingContext, AptMaskRenderOperation eMaskOperation, int32_t nMaskDepth, const AptCIH *pCIH) const
{
    // TODO
}

void AptRenderItemCustomControl::Render(AptRenderingContext *pRenderingContext, AptMaskRenderOperation eMaskOperation, int32_t nMaskDepth, const AptCIH *pCIH) const
{

    const AptRenderItem *pChild = pCIH->GetFirstChild()->GetCharacterInst()->GetRenderItem();
    const AptCharacter *pShape  = pChild->GetCharacterConst();

    PushMatrices(pRenderingContext, this);
    PushMatrices(pRenderingContext, pChild);

    // create a push effect node for dynamic text
    AptAssetEffect pEffect = PushEffectRenderElement(pCIH, (const AptRenderItem *)this);

    AptRenderInfo info;
    info.mnLevel     = AptRenderItem::suCurrentRenderAnimLevel;
    info.mTransform  = *AptMath::_ClipStackGetTop();
    info.meMaskOper  = eMaskOperation;
    info.mnMaskDepth = nMaskDepth;

    if (!AptGetLib()->mInitParms.bCustomControlUseZid || !m_szType.IsEmpty())
        mpCustomControlRender->Set(m_szType.c_str(), m_szTarget.c_str(), m_szProperties.c_str());

    AptRenderList::Add(info, this, mpCustomControlRender, pShape->shape.pRenderUnit);

    // pop the effect node.
    PopEffectRenderElement((const AptRenderItem *)this, pEffect);

    PopMatrices(pRenderingContext, pChild);
    PopMatrices(pRenderingContext, this);
}

AptRenderItemCustomControl *AptRenderItemCustomControl::CopyFromSprite(class AptRenderItemSprite *pObj, int32_t nCurrentUpdateTick, bool bCreateAsNewRevisionOfObj)
{
    return new AptRenderItemCustomControl(pObj, nCurrentUpdateTick, bCreateAsNewRevisionOfObj);
}

void AptRenderItemAnimation::Render(AptRenderingContext *pRenderingContext, AptMaskRenderOperation eMaskOperation, int32_t nMaskDepth, const AptCIH *pCIH) const
{
    // TODO only sprite should be sufficient at this time
}

void AptRenderItemButton::Render(AptRenderingContext *pRenderingContext, AptMaskRenderOperation eMaskOperation, int32_t nMaskDepth, const AptCIH *pCIH) const
{
    AptMatrix cur = gIdentityMatrix;
    // AptGetLib()->mpRenderingContext->multMatrix(&cur, &this->matrix, &cur);
    const AptCIH *pParent = pCIH;
    while (pParent != NULL)
    {
        const AptMatrix *pMatrix = pParent->GetPositionMatrixConst();
        AptGetLib()->mpRenderingContext->multMatrix(&cur, pMatrix, &cur);
        pParent = pParent->GetDisplayListParent();
    }
    AptMatrix matrix;
#if !defined(APT_3D)
    pRenderingContext->getVertexMatrix(&matrix);
#else
    AptMath::Mat44T matrix2;
    pRenderingContext->getVertexMatrix(&matrix2);
    AptMath::MatConvert(&matrix, &matrix2);
#endif
}

void AptRenderItemDynamicText::Render(AptRenderingContext *pRenderingContext, AptMaskRenderOperation eMaskOperation, int32_t nMaskDepth, const AptCIH *pCIH) const
{
    AptAssetString zID = GetZID(GetRenderable());
    // this needs to happen before the function since the matrix for pTextInst can change after EnsureStringAllocated is called

    if (zID == NULL || zID == &AptCIH::sEmptyAssetString || ((GetStateFlags() & APT_TEXTFIELD_NONE) == 0))
    {
        AptCIH *pTmp = (AptCIH *)pCIH;
        pTmp->EnsureStringAllocated(pTmp->GetDisplayListParent());
        zID = GetZID(GetRenderable());
    }

    if (zID != NULL && zID != &AptCIH::sEmptyAssetString)
    {
        PushMatrices(pRenderingContext, this);
        // create a push effect node for dynamic text
        AptAssetEffect pEffect = PushEffectRenderElement(pCIH, (const AptRenderItem *)this);
        AptRenderInfo info;
        info.mnLevel     = AptRenderItem::suCurrentRenderAnimLevel;
        info.mTransform  = *AptMath::_ClipStackGetTop();
        info.meMaskOper  = eMaskOperation;
        info.mnMaskDepth = nMaskDepth;
        AptRenderList::Add(info, this, const_cast<AptRenderableString *>(GetRenderable()));
        // pop the effect node.
        PopEffectRenderElement((const AptRenderItem *)this, pEffect);
        PopMatrices(pRenderingContext, this);
    }
}

void AptRenderItemDynamicText::SetTextFormat(TextFormat *pNewFormat)
{
    delete mpMyTextFormat;
    mpMyTextFormat = (pNewFormat);
}

void AptRenderItemDynamicText::SetZID(AptAssetString newValue)
{
    if (!mpRenderString || (mpRenderString && mpRenderString->mString != newValue))
    {
        if (mpRenderString)
            mpRenderString->Release();
        mpRenderString = newValue ? new AptRenderableString(newValue) : 0;
    }
}

void AptRenderItemStaticText::Render(AptRenderingContext *pRenderingContext, AptMaskRenderOperation eMaskOperation, int32_t nMaskDepth, const AptCIH *pCIH) const
{
    const AptCharacter *pCharacter = GetCharacterConst();

    AptMatrix translate = gIdentityMatrix;
    float fXOffset = -99999999.f, fYOffset = -99999999.f, fAdvance = 0.f, fScale = 1.f;
    PushMatrices(pRenderingContext, this);
    pRenderingContext->pushVertexMatrix();
#if !defined(APT_3D)
    pRenderingContext->appendVertexMatrix(&pCharacter->statictext.matrix);
#else
    AptMath::Mat44T matrix;
    AptMath::MatMakeIdentity(&matrix);
    AptMath::MatConvert(&matrix, &pCharacter->statictext.matrix);
    pRenderingContext->appendVertexMatrix(&matrix);
#endif

    for (int i = 0; i < pCharacter->statictext.nFontRecords; i++)
    {
        pRenderingContext->pushColourTransform();
        AptCXForm tmpCXForm;
        tmpCXForm.AptFloatArrayCXFormCopy(&(pCharacter->statictext.aRecords[i].cxform));
        pRenderingContext->appendColourTransform(&tmpCXForm);

        AptCharacter *pFont = pCharacter->pParentAnim->animation.apCharacters[pCharacter->statictext.aRecords[i].nFontID];
        APT_ASSERT(pFont->eType == AptCharacterType_Font);
        APT_ASSERT(pFont->font.apGlyphs);

        if (FLOAT_NOT_EQUALS(fXOffset, pCharacter->statictext.aRecords[i].fXOffset) || FLOAT_NOT_EQUALS(fYOffset, pCharacter->statictext.aRecords[i].fYOffset))
        {
            fAdvance = 0.f;
            fScale   = 1.f;
        }

        fXOffset = pCharacter->statictext.aRecords[i].fXOffset;
        fYOffset = pCharacter->statictext.aRecords[i].fYOffset;
        fScale   = pCharacter->statictext.aRecords[i].fScale;

        for (int j = 0; j < pCharacter->statictext.aRecords[i].nGlyphs; j++)
        {
            translate.tx = fXOffset + fAdvance;
            translate.ty = fYOffset;
            translate.a  = fScale;
            translate.d  = fScale;

            AptCharacterGlyphEntry *pGlyphEntry = &pCharacter->statictext.aRecords[i].aGlyphs[j];
            APT_ASSERT(pGlyphEntry->nIndex < pFont->font.nGlyphs);
            if (AptOptIsEnabled(APT_OPT_SPRTREE))
                AptDisplayList::_drawCharacterInstOpti(&translate, &tmpCXForm);
            pRenderingContext->pushVertexMatrix();
#if !defined(APT_3D)
            pRenderingContext->appendVertexMatrix(&translate);
#else
            AptMath::Mat44T matrix;
            AptMath::MatMakeIdentity(&matrix);
            AptMath::MatConvert(&matrix, &translate);
            pRenderingContext->appendVertexMatrix(&matrix);
#endif
            pFont->font.apGlyphs[pGlyphEntry->nIndex]->render(pRenderingContext, eMaskOperation, nMaskDepth, this);
            pRenderingContext->popVertexMatrix();
            if (AptOptIsEnabled(APT_OPT_SPRTREE))
                AptMath::ClipStackPop();
            fAdvance += pGlyphEntry->nAdvance / 20.f;
        }

        pRenderingContext->popColourTransform();
    }
    pRenderingContext->popVertexMatrix();
    PopMatrices(pRenderingContext, this);
}

void AptRenderItemMorph::Render(AptRenderingContext *pRenderingContext, AptMaskRenderOperation eMaskOperation, int32_t nMaskDepth, const AptCIH *pCIH) const
{
    const AptCharacter *pCharacter = this->GetCharacterConst();
    pRenderingContext->pushColourTransform();
    // pRenderingContext->curCXForm.scale.SetValue(0, 1.f - this->GetRatio());
    pRenderingContext->curCXForm.scale.SetValuef(AptColorHelper::Alpha, AptColorHelperScale::SCALE_FACTOR - (this->GetRatio() * AptColorHelperScale::SCALE_FACTOR));
    {
        AptGetUserFuncs().pfnSetColourTransform(&pRenderingContext->curCXForm);
    }
    pCharacter->morph.pStartCharacter->render(pRenderingContext, eMaskOperation, nMaskDepth, this);
    // pRenderingContext->curCXForm.scale.SetValue(0, this->GetRatio());
    pRenderingContext->curCXForm.scale.SetValuef(AptColorHelper::Alpha, this->GetRatio() * AptColorHelperScale::SCALE_FACTOR);
    {
        AptGetUserFuncs().pfnSetColourTransform(&pRenderingContext->curCXForm);
    }
    pCharacter->morph.pEndCharacter->render(pRenderingContext, eMaskOperation, nMaskDepth, this);
    pRenderingContext->popColourTransform();
}

void AptRenderItemShape::Render(AptRenderingContext *pRenderingContext, AptMaskRenderOperation eMaskOperation, int32_t nMaskDepth, const AptCIH *pCIH) const
{
    const AptCharacter *pCharacterConst = this->GetCharacterConst();
    AptCharacter *pCharacter            = (AptCharacter *)pCharacterConst; // Casted away const as AptCharacter::render needs to set m_shapeData.m_bNotLoaded

    PushMatrices(pRenderingContext, this);

    pCharacter->render(pRenderingContext, eMaskOperation, nMaskDepth, this);

    PopMatrices(pRenderingContext, this);
}
