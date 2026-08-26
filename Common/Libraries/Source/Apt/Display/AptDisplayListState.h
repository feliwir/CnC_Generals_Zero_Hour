#pragma once

#include "AptValue/AptValue.h"
#include "AptCharacter/AptCharacter.h"

struct AptControl;
struct AptEventActionSet;
struct AptFilter;

struct AptDisplayListState
{
    APT_NEW_DELETE_OPERATORS
    // Added Metric defines

    AptDisplayListState();
    ~AptDisplayListState();

    AptCIH *insert(AptCIH *pPrev, AptCIH *pNewItem);
    AptCIH *insert(int nDepth, AptCharacter *pCharacter, AptCIH *pParent, AptCIH *pPrev, AptCIH *pItemAtDepth);
    AptCIH *insert(int nDepth, AptCharacter *pCharacter, AptCIH *pParent);
    AptCIH *insert(int nDepth, AptCIH *pParent, AptCIH *pItem, AptCIH *pPrev, AptCIH *pItemAtDepth);
    AptCIH *insert(int nDepth, AptCIH *pItem);
    AptCIH *remove(int nDepth);
    void swapDepths(AptCIH *p0, AptCIH *p1);
    AptCIH *removeItem(AptCIH *pItem);
    static AptCIH *remove(AptCIH *pItem);
    AptCIH *ChangeDepth(int nDepth, AptCIH *pItem);

    void findInst(int nDepth, const AptNativeString *pName, AptCIH **ppItem);
    int getLength();
    AptCIH *getValue(int nIndex);
    bool HasRenderData() const;
    void GetMovieclipInfo(AptMovieclipInformation *pMCInfo) const;
    void AddToDelayReleaseList(AptCIH *pItem, const bool bDestroyGC);

    /** @brief Report all of our references. */
    void RegisterReferences(const AptValue *pFrom); // this is not virtual because it is not designed to be overridden.

    APT_INLINE AptCIH *GetFirstItem()
    {
        return pHead;
    }

    APT_INLINE const AptCIH *GetFirstItem() const
    {
        return pHead;
    }

  protected:
    void findInst(int nDepth, const AptNativeString *pName, AptCIH **ppPrev, AptCIH **ppItem);
    AptCIH *pHead;
};

/**
 * @brief This struct is used by the AptPseudoDisplayList and the AptPsuedoCIH to maintain any placeObject2 controls.  RemoveObject2 controls do not need to
 * maintain this information.
 */
struct AptPseudoData2T
{
    APT_NEW_DELETE_OPERATORS
    // Added Metric defines

    explicit AptPseudoData2T(AptControl *pControl, int32_t nFrame, AptCharacter *pNewCharacter);
    ~AptPseudoData2T()
    {
        pCharacter = NULL;
        matrix     = NULL;
        ncxform    = NULL;
    }

    AptCharacter *pCharacter;
    AptMatrix *matrix;
    AptUint32CXForm *ncxform;
    AptEventActionSet *pActions;
    float fRatio;
    int32_t eFlags;
    int32_t nFrameCreated : 16;
    int32_t nClipDepth : 16;
};

/** @brief Same as above, except this struct maintains PlaceObject3 controls. */
struct AptPseudoData3T
{
    APT_NEW_DELETE_OPERATORS
    // Added Metric defines
    explicit AptPseudoData3T(AptControl *pControl, int32_t nFrame, AptCharacter *pNewCharacter);
    ~AptPseudoData3T()
    {
        pCharacter = NULL;
        matrix     = NULL;
        ncxform    = NULL;
    }

    AptCharacter *pCharacter;
    AptMatrix *matrix;
    AptUint32CXForm *ncxform;
    AptEventActionSet *pActions;
    float fRatio;
    int32_t eFlags;
    int32_t nFrameCreated : 16;
    int32_t nClipDepth : 16;
    int32_t nBlendMode;
    uint32_t nNumFilters;
    AptFilter **ppFilters;
};

/** @brief This struct is used by the AptPseudoDisplayList to represent a placeObject2 or removeObject2 control in flashes display list */
using AptPseudoCIHT = struct AptPseudoCIH_t
{
    APT_NEW_DELETE_OPERATORS
    // Added Metric defines

    explicit AptPseudoCIH_t(AptControl *pNewControl, int32_t nFrame, int32_t nDpth, AptCharacter *pNewCharacter);
    ~AptPseudoCIH_t();

    AptControl *pControl;
    union
    {
        AptPseudoData2T *pControlInfo2;
        AptPseudoData3T *pControlInfo3;
    };
    int32_t nDepth;

    AptPseudoCIH_t *GetDisplayListNext()
    {
        return pNext;
    }
    AptPseudoCIH_t *GetDisplayListPrevious()
    {
        return pPrev;
    }

    void SetDisplayListNext(AptPseudoCIH_t *pNewNext)
    {
        pNext = pNewNext;
    }
    void SetDisplayListPrevious(AptPseudoCIH_t *pNewPrev)
    {
        pPrev = pNewPrev;
    }

  private:
    // don't copy
    AptPseudoCIH_t(const AptPseudoCIH_t &);
    AptPseudoCIH_t &operator=(const AptPseudoCIH_t &);

    AptPseudoCIH_t *pNext;
    AptPseudoCIH_t *pPrev;
};

/**
 * @brief This class is used to represents the display list of a sprite when a gotoAndX command is called.  Its data is later merged in with the sprites main
 * display list.
 */
class AptPseudoDisplayList
{
  public:
    APT_NEW_DELETE_OPERATORS
    // Added Metric defines

    explicit AptPseudoDisplayList(AptCIH *pParent)
    {
        pHead      = new AptPseudoCIHT(NULL, 0, 0, NULL);
        pParentCIH = pParent;
    }
    ~AptPseudoDisplayList()
    {
        ClearList();
        delete pHead;
    }

    void ClearList();
    void Insert(AptPseudoCIHT *pItem);
    void Insert(AptPseudoCIHT *pNewItem, AptPseudoCIHT *pPrev, AptPseudoCIHT *pOldItem);
    void Remove(AptPseudoCIHT *pItem);
    void FindInst(int32_t nDepth, AptPseudoCIHT **ppPrev, AptPseudoCIHT **ppItem);

    APT_INLINE
    AptPseudoCIHT *GetFirstItem()
    {
        return pHead->GetDisplayListNext();
    }

    APT_INLINE
    AptCIH *GetParentSprite()
    {
        return pParentCIH;
    }

  private:
    void Insert(AptPseudoCIHT *pPrev, AptPseudoCIHT *pNewItem);
    void Remove(int32_t nDepth);

    AptPseudoCIHT *pHead;
    AptCIH *pParentCIH;
};

/*H*************************************************************************************************
    EOF
*************************************************************************************************H*/
