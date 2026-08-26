#pragma once

/*** Include files ********************************************************************************/
#include "AptRenderItem.h"
// #include "AptCIH.h"
/*** Defines **************************************************************************************/
class AptCIH; // Forward declaration
class AptCharacterInst;

/*** Macros ***************************************************************************************/

/*** Type Definitions *****************************************************************************/

class AptBCRenderTreeManager
{

  public:
    APT_NEW_DELETE_OPERATORS
    // Added Metric defines

    AptBCRenderTreeManager() : mpRootListHead(NULL)
    {
    }

    AptRenderItem *Update_CreateItem(AptCharacter *pCharacter, int32_t nCurrentUpdateTick);

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    // These functions use AptCIH Next and previous, thus the CIH must be setup before these are called.
    // If there is a writable item at this tick count we move it. It there is not, we create a new item
    // and insert it at the new location.

    void Update_ItemInserted(AptCIH *pItemOwner, int32_t nCurrentUpdateTick);
    void Update_ItemMoved(AptCIH *pItemOwner, int32_t nCurrentUpdateTick);
    void Update_ItemRemoved(AptRenderItem *pItem, int32_t nCurrentUpdateTick);

    void Update_CloneItem(const AptCIH *pCIH, AptCIH *pNeedsCIHRenderState, int32_t nCurrentUpdateTick);
    void Update_SetRootItem(AptCIH *pRoot, int32_t nCurrentUpdateTick);

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    // Apt Render Traversal functions

    const AptRenderItem *Render_GetChild(const AptRenderItem *pCurrent, int32_t nCurrentRenderTick) const;
    const AptRenderItem *Render_GetSibling(const AptRenderItem *pCurrent, int32_t nCurrentRenderTick) const;
    const AptRenderItem *Render_GetMask(const AptRenderItem *pCurrent, int32_t nCurrentRenderTick) const;

    void Render_Shutdown();

  protected:
    // Need to keep a list of roots, when the root changes we add an Item into this linked list
    // This way when the tick comes to replace the root, we replace can it.
    using AptRenderItemRootList = struct _AptRenderItemRootList
    {

        APT_NEW_DELETE_OPERATORS

        AptRenderItem *mpRoot;

        void Shutdown()
        {
            struct _AptRenderItemRootList *pCurrent = this;
            struct _AptRenderItemRootList *pNext    = this->mpNextRoot;

            // Find an item that would not be valid for this render tick. (or the end of the list)
            while (pCurrent != NULL)
            {
                pNext = pCurrent->mpNextRoot;
                APT_RI_DECSAFE(pCurrent->mpRoot, pCurrent, "RootItem");
                delete pCurrent;
                pCurrent = pNext;
            }
        }

        void InsertNewRoot(AptRenderItem *pNewRoot)
        {
            struct _AptRenderItemRootList *pCurrent = this;
            struct _AptRenderItemRootList *pNext    = this->mpNextRoot;

            // Find the end of the list.
            while (pNext != NULL)
            {
                // Loop.
                pCurrent = pNext;
                pNext    = pCurrent->mpNextRoot;
            }

            // Create the new object
            pNext = new _AptRenderItemRootList();

            APT_RI_INC(pNewRoot, pNext, "RootItem");

            pNext->mpRoot     = pNewRoot;
            pNext->mpNextRoot = NULL;

            // Insert it.
            pCurrent->mpNextRoot = pNext;
        }

        static struct _AptRenderItemRootList *StartTree(AptRenderItem *pNewRoot)
        {

            struct _AptRenderItemRootList *pNew = new _AptRenderItemRootList();

            APT_RI_INC(pNewRoot, pNew, "RootItem");

            pNew->mpRoot     = pNewRoot;
            pNew->mpNextRoot = NULL;
            return pNew;
        }

      private:
        struct _AptRenderItemRootList *mpNextRoot;
    };

    // This is mutable because the root can be updated to a later revision without modifying the
    // state of the tree. Operations that allow the root to be changed to a different object are
    // non-const.
    mutable AptRenderItemRootList *mpRootListHead;
};

/*** Variables ************************************************************************************/

/*** Functions ************************************************************************************/

