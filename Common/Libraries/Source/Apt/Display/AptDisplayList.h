#pragma once

#include "Apt.h"
#include "Display/AptDisplayListState.h"

struct AptRenderingContext;
struct AptControlPlaceObject2;
struct AptControlPlaceObject3;
struct AptControlRemoveObject2;
struct AptCharacter;
struct AptEventActionSet;
struct AptRect;
struct AptFilter;

class AptValue;
class AptNativeHash;
class AptCIH;

struct AptDisplayList
{
    const static int BASE_MOVIE_DEPTH = 16384;

  public:
    explicit AptDisplayList();
    ~AptDisplayList();
    // - after 0.19.01 -added new parameter/s to do selective display of animation levels. This will only work at highest root levels.
    // Added Mask Depth as a parameter to enable nested masks.
    void render(AptRenderingContext *pRenderingContext, AptMaskRenderOperation eMaskOperation, int nMaskDepth, AptAnimLevelE eAnimLevels = AptAnimLevel_ALL, bool bUseDepthCompare = false);
    void DeallocAssetStringRecursive();
    void GetBoundingRect(AptRenderingContext *pRenderingContext, const AptMatrix *pCurrentTransform, AptRect *pRect) const;

    AptCIH *placeObject(AptCIH *pItem,
                        int nTargetDepth,
                        AptCharacter *pCharacter,
                        const AptNativeString *szName,
                        AptCIH *pParent             = NULL,
                        int bForceNewInstance       = 0,
                        int nClipDepth              = -1,
                        AptCXForm *pCXForm          = NULL,
                        AptMatrix *pMatrix          = NULL,
                        AptEventActionSet *pActions = NULL,
                        float fRatio                = 0.f,
                        AptValue *pInitObject       = NULL,
                        int32_t nBlendMode          = -1,
                        uint32_t nNumFilters        = 0,
                        AptFilter **pFilters        = NULL);
    AptCIH *placeObjectNCXForm(AptCIH *pItem,
                               int nTargetDepth,
                               AptCharacter *pCharacter,
                               const AptNativeString *szName,
                               AptCIH *pParent             = NULL,
                               int bForceNewInstance       = 0,
                               int nClipDepth              = -1,
                               AptUint32CXForm *pnCXForm   = NULL,
                               AptMatrix *pMatrix          = NULL,
                               AptEventActionSet *pActions = NULL,
                               float fRatio                = 0.f,
                               int32_t nBlendMode          = -1,
                               uint32_t nNumFilters        = 0,
                               AptFilter **pFilters        = NULL);
    AptCIH *placeObject(AptControlPlaceObject2 *pPlaceObject2, AptCIH *pParent);
    AptCIH *placeObject(AptControlPlaceObject3 *pPlaceObject3, AptCIH *pParent);
    AptCIH *placeObject(AptPseudoCIHT *pNewItem, AptCIH *pParentSprite);
    void removeObject(AptCIH *pItem);
    void removeObject(int nRemovalDepth);
    void removeObject(AptControlRemoveObject2 *pRemoveObject2);
    void removeClonedObject(AptCIH *pCIH);
    void clear(bool bClean = false);
    // - after 0.19.01 -added new parameter/s to do selective update/tick of animation levels. This will only work at highest root levels.
    uint32_t tick(AptAnimLevelE eAnimLevels = AptAnimLevel_ALL, bool bUseDepthCompare = false);
    /**
     * @brief This is added as gpPool has now parameterized pool sizes.
     * So before deleting gpPool we call gpPool->preDestroy which will call this function
     * to clear the display list inside gpPool.
     * This should pnly be called from gpPool->PreDestroy.
     */
    void PreDestroy(); // This is not virtual because it is not designed to be overridden.

    static uint32_t GeneralisedProcess(AptDisplayList *pDL, void *pVoid, AptAnimLevelE eAnimLevels = AptAnimLevel_ALL, bool bUseDepthCompare = false);

    /**
     * @return A pointer to the AptDisplayListState structure used to hold the state of this display list.
     * @note Items referenced by this pointer are still referenced-tracked through the global pools.
     */
    inline AptDisplayListState *getState() const { return pState; }
    void useState(AptDisplayListState *pNewState);
    void mergeState(AptPseudoDisplayList *pNewState, AptNativeHash *pOrigObject, bool bJumpAhead);
    void validate(AptCIH *pParent);
    AptDisplayListState *pState;

    static void _addToSetCaches(AptCIH *pItem, int bQueueClipEvents = 1);

    static void _drawCharacterInstOpti(const class AptRenderItem *pRI);
    static void _drawCharacterInstOpti(const AptMatrix *pCurrMatrix, const AptCXForm *pCurrCXForm);
    static void _drawCharacterInstAbsoluteOpti(const class AptRenderItem *pRI);

    void dumpDisplayList(const char *szFileName);

  private:
    // don't copy
    AptDisplayList(const AptDisplayList &);
    AptDisplayList &operator=(const AptDisplayList &);

    void instantiateCharacter(
        int nTargetDepth,
        AptCharacter *pCharacter,
        const AptNativeString *pName,
        AptCIH *pParent,
        int bForceNewInstance,
        int nClipDepth,
        AptCIH **pItem, int *pbNeedNewInst);
    AptCIH *AddToDisplayList(AptNativeHash *pHash, AptPseudoCIHT *pNewControl, AptCIH *pParentCIH);
    void RemoveFromDisplayList(AptNativeHash *pHash, AptCIH *pTmp);
    void ReplaceDisplyListItem(AptNativeHash *pHash, AptCIH *pOriginalItem, AptPseudoCIHT *pNewItem, AptCIH *pParent);
};

/*H*************************************************************************************************
    EOF
*************************************************************************************************H*/
