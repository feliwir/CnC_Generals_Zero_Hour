/*** Include files ********************************************************************************/
#include "_Apt.h"
#include "AptCIH.h"
#include "AptCharacterInst.h"
#include "AptBCRenderTree.h"
#include "MainInline.h"

#if !defined(APT_ENABLE_INLINE)
#include "AptBCRenderTree.inl"
#endif

/*** Defines **************************************************************************************/

/*** Macros ***************************************************************************************/

/*** Type Definitions *****************************************************************************/

/*** Function Prototypes **************************************************************************/

/*** Variables ************************************************************************************/

// Private variables

// Public variables

/*** Private Functions ****************************************************************************/

/*** Public Functions *****************************************************************************/
AptRenderItem *AptBCRenderTreeManager::Update_CreateItem(AptCharacter *pCharacter, int32_t nCurrentUpdateTick)
{
    return AptRenderItem::Manager_CreateItem(pCharacter, nCurrentUpdateTick);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// These functions use AptCIH Next and previous, thus the CIH must be setup before these are called.
// If there is a writable item at this tick count we move it. It there is not, we create a new item
// and insert it at the new location.

void AptBCRenderTreeManager::Update_ItemMoved(AptCIH *pItemOwner, int32_t nCurrentUpdateTick)
{
    if (pItemOwner->GetCharacterInst() == NULL)
        return;

    if (pItemOwner->GetDisplayListPrevious() == NULL && pItemOwner->GetDisplayListParent() == NULL)
    {
        Update_SetRootItem(pItemOwner, nCurrentUpdateTick);
    }
}

void AptBCRenderTreeManager::Update_ItemRemoved(AptRenderItem *pItem, int32_t nCurrentUpdateTick)
{
    pItem->Manager_SetDeletionMark(true);
}

void AptBCRenderTreeManager::Update_ItemInserted(AptCIH *pItemOwner, int32_t nCurrentUpdateTick)
{
    AptRenderItem *pItem = pItemOwner->GetCharacterInst()->GetRenderItemWritable();
    pItem->Manager_SetDeletionMark(false);
    Update_ItemMoved(pItemOwner, nCurrentUpdateTick);
}

void AptBCRenderTreeManager::Update_CloneItem(const AptCIH *pCIH, AptCIH *pNeedsCIHRenderState, int32_t nCurrentUpdateTick)
{
    // We aren't modifying this, and const correctedness is getting to be a headache. for now typecast the const away.
    AptRenderItem *pItem    = (AptRenderItem *)pCIH->GetCharacterInst()->GetRenderItem();
    AptRenderItem *pNewItem = pItem->Manager_CloneNewItem(nCurrentUpdateTick);

    pNeedsCIHRenderState->GetCharacterInst()->SetRenderItem(pNewItem);
}

void AptBCRenderTreeManager::Update_SetRootItem(AptCIH *pRoot, int32_t nCurrentUpdateTick)
{
    if (pRoot == NULL)
    {
        return;
    }
    AptRenderItem *pItem = pRoot->GetCharacterInst()->GetRenderItemWritable();

    if (mpRootListHead == NULL)
    {
        mpRootListHead = AptRenderItemRootList::StartTree(pItem);
    }
    else if (mpRootListHead->mpRoot != pItem)
    {
        // New Root Detected!!!
        mpRootListHead->InsertNewRoot(pItem);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Apt Render Traversal functions

const AptRenderItem *AptBCRenderTreeManager::Render_GetChild(const AptRenderItem *pCurrent, int32_t nCurrentRenderTick) const
{
    return NULL;
}

const AptRenderItem *AptBCRenderTreeManager::Render_GetSibling(const AptRenderItem *pCurrent, int32_t nCurrentRenderTick) const
{
    return NULL;
}

const AptRenderItem *AptBCRenderTreeManager::Render_GetMask(const AptRenderItem *pCurrent, int32_t nCurrentRenderTick) const
{
    AptRenderItem *pCurrentWritable = (AptRenderItem *)pCurrent;
    pCurrentWritable->Manager_UpdateMask((AptRenderItem *)pCurrentWritable->GetMask());

    AptRenderItem *pSibling = pCurrentWritable->Manager_GetMask();
    APT_ASSERT((pSibling == NULL || !pSibling->Manager_IsDeletionMark()) && "ERROR: Trying to render a Deleted object!");
    return pSibling;
}

void AptBCRenderTreeManager::Render_Shutdown()
{
    if (mpRootListHead != NULL)
    {
        mpRootListHead->Shutdown();
        mpRootListHead = NULL;
    }
}
