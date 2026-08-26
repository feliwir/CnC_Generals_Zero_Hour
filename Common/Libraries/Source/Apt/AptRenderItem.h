#pragma once

/*** Include files ********************************************************************************/
#include "_Apt.h"
#include "Apt.h" // For DataTypes.
#include "AptCharacter/AptCharacter.h"
#include "AptRenderableGeometry.h"
#include "AptRenderableString.h"
#include "AptRenderableCustomControl.h"

/*** Defines **************************************************************************************/
#define APT_RENDER_ITEM_DEFAULT_HASH_SIZE 4
// #define APT_RI_LIST_VERIFY 1                   /* Define this to keep a linked list of Render Items for debuggine purposes */
// #define APT_RI_REFCOUNT_DEBUG 1                /* Define this to Get Debug messages useful in finding reference counting problems */
/*** Macros ***************************************************************************************/

/*** Type Definitions *****************************************************************************/
struct AptCharacter;
struct AptRenderingContext;
#if defined(APT_3D)
struct AptRI3DHelper;
#endif
class AptRenderableImage;

// Also used outside of asserts by the inlined render-manager update calls in AptCharacterInst.inl.

//------------------------------------------------------------------------------
// Added Increment / Decrement messages. Turn them on with APT_RI_REFCOUNT_DEBUG
#if defined(__GNUC__) && defined(APT_RI_REFCOUNT_DEBUG)

#define APT_RI_DEC(_, refFrom, RefType)                                                                      \
    {                                                                                                        \
        const AptRenderItem *pTTTemp = _;                                                                    \
        _                            = NULL;                                                                 \
        AptRenderItem::ReleaseReference(pTTTemp, refFrom, RefType, __PRETTY_FUNCTION__, __FILE__, __LINE__); \
    }

#define APT_RI_DECSAFE(_, refFrom, RefType)                                                                  \
    if (_)                                                                                                   \
    {                                                                                                        \
        const AptRenderItem *pTTTemp = _;                                                                    \
        _                            = NULL;                                                                 \
        AptRenderItem::ReleaseReference(pTTTemp, refFrom, RefType, __PRETTY_FUNCTION__, __FILE__, __LINE__); \
    }

#define APT_RI_INC(_, refFrom, RefType) AptRenderItem::AddReference(_, refFrom, RefType, __PRETTY_FUNCTION__, __FILE__, __LINE__);

#define APT_RI_INCSAFE(_, refFrom, RefType)                                                        \
    if (_)                                                                                         \
    {                                                                                              \
        AptRenderItem::AddReference(_, refFrom, RefType, __PRETTY_FUNCTION__, __FILE__, __LINE__); \
    }

#elif defined(_MSC_VER) && defined(APT_RI_REFCOUNT_DEBUG)

#define APT_RI_DEC(_, refFrom, RefType)                                                               \
    {                                                                                                 \
        const AptRenderItem *pTTTemp = _;                                                             \
        _                            = NULL;                                                          \
        AptRenderItem::ReleaseReference(pTTTemp, refFrom, RefType, __FUNCTION__, __FILE__, __LINE__); \
    }

#define APT_RI_DECSAFE(_, refFrom, RefType)                                                           \
    if (_)                                                                                            \
    {                                                                                                 \
        const AptRenderItem *pTTTemp = _;                                                             \
        _                            = NULL;                                                          \
        AptRenderItem::ReleaseReference(pTTTemp, refFrom, RefType, __FUNCTION__, __FILE__, __LINE__); \
    }

#define APT_RI_INC(_, refFrom, RefType) AptRenderItem::AddReference(_, refFrom, RefType, __FUNCTION__, __FILE__, __LINE__);

#define APT_RI_INCSAFE(_, refFrom, RefType)                                                 \
    if (_)                                                                                  \
    {                                                                                       \
        AptRenderItem::AddReference(_, refFrom, RefType, __FUNCTION__, __FILE__, __LINE__); \
    }

#else

#define APT_RI_DEC(_, refFrom, RefType)           \
    {                                             \
        const AptRenderItem *pTTTemp = _;         \
        _                            = NULL;      \
        AptRenderItem::ReleaseReference(pTTTemp); \
    }

#define APT_RI_DECSAFE(_, refFrom, RefType)       \
    if (_)                                        \
    {                                             \
        const AptRenderItem *pTTTemp = _;         \
        _                            = NULL;      \
        AptRenderItem::ReleaseReference(pTTTemp); \
    }

#define APT_RI_INC(_, refFrom, RefType) AptRenderItem::AddReference(_);

#define APT_RI_INCSAFE(_, refFrom, RefType) \
    if (_)                                  \
    {                                       \
        AptRenderItem::AddReference(_);     \
    }

#endif

class AptRenderItem
{
  public:
    ///////////////////////////////////////////////////////////////////////////////////////////////
    // Getters. *const*, accessible by all.
    AptCharacterType GetCharacterType() const
    {
        return (AptCharacterType)meCharacterType;
    }

    virtual AptRenderItem *Clone(int32_t nCurrentUpdateTick, bool bCreateAsNewRevisionOfObj) = 0;

    APT_INLINE int32_t GetClipDepth() const
    {
        return mnClipDepth;
    }

    /** @brief Returns the Character inside this item. */
    APT_INLINE const AptCharacter *GetCharacterConst() const
    {
        return mpCharacter;
    }

    /** @brief Returns the position / translation matrix info of the item. */
    APT_INLINE const AptMatrix *GetPositionMatrixConst() const;

    /** @brief Returns the position / translation matrix info of the mask item. */
    APT_INLINE const AptMatrix *GetMaskPositionMatrixConst() const
    {
        return mpMaskPositionMatrix;
    }

    /** @brief Returns the color matrix of the item. */
    APT_INLINE const AptCXForm *GetColorMatrixConst() const;

    /** @brief Returns the depth of the current item. */
    APT_INLINE int32_t GetDepth() const
    {
        return mnDepth;
    }

    /** @brief Returns true if this item is a mask. */
    APT_INLINE bool GetIsMask() const
    {
        return (bool)mbIsMask;
    }

    /** @brief Returns the mask pointer. */
    APT_INLINE const AptRenderItem *GetMask() const
    {
        return mpMask;
    }

    /** @brief Returns true if the item has a mask dynamically applied. */
    APT_INLINE bool GetHasMask() const
    {
        return (((bool)(mbHasMask)) && (mpMask != NULL)); // added extra check - return true only if both are consistent
    }

    /** @brief Returns true if the item is visible. */
    APT_INLINE bool GetIsVisible() const
    {
        return (bool)mbIsVisible;
    }
    APT_INLINE int32_t GetRefCount() const
    {
        return mnReferenceCount;
    }

    APT_INLINE int32_t GetCreatedOnTick() const
    {
        return mnCreatedOnTick;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////
    // Setters, *Non-Const*, Should only be used by Update using a writable pointer from Update_GetTickItemWritable

    /** @brief Return the character inst inside this item. (writable) */
    APT_INLINE AptCharacter *GetCharacterWritable()
    {
        return mpCharacter;
    }

    /** @brief Returns a writable reference to the position matrix of this item. */
    AptMatrix *GetPositionMatrixWritable();

    /** @brief Returns a writable reference to the position matrix of this item. */
    APT_INLINE AptMatrix *GetMaskPositionMatrixWritable()
    {
        return mpMaskPositionMatrix;
    }

    /** @brief Returns a writable reference to the color matrix. */
    AptCXForm *GetColorMatrixWritable();

    /** @brief Returns the depth of the current item. */
    APT_INLINE void SetDepth(int32_t newDepth)
    {
        APT_ASSERT(newDepth <= 0xffff);
        mnDepth = static_cast<uint16_t>(newDepth);
    }

#if defined(APT_3D)
    //// Getter and Setter methods for the 3D variables
    float GetZPosition() const;
    float GetZScale() const;
    float GetYRotation() const;
    float GetXRotation() const;
    void SetZPosition(float zPosition);
    void SetZScale(float zScale);
    void SetXRotation(float xRot);
    void SetYRotation(float yRot);
#endif
    /** @brief Set a new character for the item. */
    void SetCharacter(AptCharacter *pCharacter);

    /** @brief Increments the isMask property, when isMask becomes zero the movie is no longer a mask. */
    void SetIsMask(bool bIsMask, AptMatrix *pMatrix);

    /** @brief Sets whether the current item has a dynamic mask applied. */
    void SetHasMask(bool newState, AptRenderItem *pMask);

    void SetMaskMatrix(AptMatrix *pMatrix);

    /** @brief Sets whether the item is visible at all. */
    void SetIsVisible(bool newState);

    /** @brief True if the pCharacter was allocated dynamically. */
    bool GetCreatedDynamic() const
    {
        return mbCreatedDynamic;
    }

    /** @brief True if the pCharacter was allocated dynamically. */
    void SetCreatedDynamic(bool newValue)
    {
        mbCreatedDynamic = newValue ? 1 : 0;
    }

    APT_INLINE void SetClipDepth(int32_t newClipDepth)
    {
        APT_ASSERT(newClipDepth <= 0xffff);
        mnClipDepth = static_cast<uint16_t>(newClipDepth);
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////
    // These are the only functions the manager should ever touch.

    /** @brief Returns the tick this item was created on. */
    APT_INLINE int32_t Manager_GetCreatedOnTick() const
    {
        return mnCreatedOnTick;
    }

    /** @brief Returns whether this is item is marked as deleted on item creation frame. */
    APT_INLINE bool Manager_IsDeletionMark() const
    {
        return mbIsDeletionMark;
    }

    /** @brief Returns a writable pointer to the mask. */
    APT_INLINE AptRenderItem *Manager_GetMask()
    {
        return mpMask;
    }

    static bool CleanUp();

    // manager functions that can be used to update the sibling, mask, child renderitems.
    void Manager_UpdateMask(AptRenderItem *pNew) const;

    // manager functions that are used to set the sibling, child, mask items.
    void Manager_SetMask(const AptRenderItem *pNew);
    void Manager_SetDeletionMark(bool bNewValue);

    static AptRenderItem *Manager_CreateItem(AptCharacter *pCharacter, int32_t nCurrentUpdateTick);

    /** @brief Allows the manager to clone an item so that it can be placed else where. */
    AptRenderItem *Manager_CloneNewItem(int32_t nCurrentUpdateTick);

    static unsigned suCurrentRenderAnimLevel;

    //
    // NOTE: never call AddReference or ReleaseReference directly. They are only called by macros defined at the top of this file.
    //

#if defined(APT_RI_REFCOUNT_DEBUG)
    static inline void AddReference(const AptRenderItem *pObj, const void *pFrom, const char *szRefType, const char *szFuncName, const char *szFileName, int32_t nLineNumber)
#else
    static inline void AddReference(const AptRenderItem *pObj)
#endif
    {
        APT_ASSERT(pObj != NULL);
#if defined(APT_RI_REFCOUNT_DEBUG)
        APT_DEBUGPRINT(APT_DEBUG_MSG_DEFAULT_LVL, "#RI,Inc,Ck:%8.8X,Obj:%8.8X,From:%8.8X,RCt:%4.4X,%s,%s:%d\n",
                       RefCountCallCounter++, pObj, pFrom, pObj->GetRefCount(), szRefType, szFuncName, nLineNumber);
#endif
        pObj->mnReferenceCount++;
    }

#if defined(APT_RI_REFCOUNT_DEBUG)
    static inline void ReleaseReference(const AptRenderItem *pObj, const void *pFrom, const char *szRefType, const char *szFuncName, const char *szFileName, int32_t nLineNumber)
#else
    static inline void ReleaseReference(const AptRenderItem *pObj)
#endif
    {
        APT_ASSERT(pObj != NULL);
#if defined(APT_RI_REFCOUNT_DEBUG)
        APT_DEBUGPRINT(APT_DEBUG_MSG_DEFAULT_LVL, "#RI,Dec,Ck:%8.8X,Obj:%8.8X,From:%8.8X,RCt:%4.4X,%s,%s:%d\n",
                       RefCountCallCounter++, pObj, pFrom, pObj->GetRefCount(), szRefType, szFuncName, nLineNumber);
#endif

        if (!--pObj->mnReferenceCount)
        {
#if defined(APT_RI_REFCOUNT_DEBUG)
            APT_DEBUGPRINT(APT_DEBUG_MSG_DEFAULT_LVL, "#RI,Rem,Ck:%8.8X,Obj:%8.8X,From:%8.8X,RCt:%4.4X,%s,%s:%d\n",
                           RefCountCallCounter, pObj, pFrom, pObj->mnReferenceCount, szRefType, szFuncName, nLineNumber);
#endif
            delete (AptRenderItem *)pObj;
        }
    }

    virtual void PushRenderData(AptRenderingContext *pRenderingContext, AptMaskRenderOperation eMaskOperation, int32_t nMaskDepth) const;
    virtual void PushRenderDataAbsolute(AptRenderingContext *pRenderingContext) const;
    virtual void PopRenderData(AptRenderingContext *pRenderingContext, AptMaskRenderOperation eMaskOperation, int32_t nMaskDepth) const;
    virtual void Render(AptRenderingContext *pRenderingContext, AptMaskRenderOperation eMaskOperation, int32_t nMaskDepth, const AptCIH *pCIH) const;
    ;

    virtual void CopyRenderDataFrom(const AptRenderItem *pModel);
    virtual void LoadResourcesFromCharacter() const {}

#if defined(APT_RI_LIST_VERIFY)
    static void AptRenderItem::VerifyItemsBeforeRender(int32_t nCurrentRenderTick);
    static void AptRenderItem::VerifyItemsAfterRender(int32_t nCurrentRenderTick);
    static AptRenderItem *spHeadItem;
    AptRenderItem *mpNextAllocatedItem;
    AptRenderItem *mpPrevAllocatedItem;
#endif

    /** @brief True if the object is not visible or pending to be. */
    bool GetIsHiddenByTree() const
    {
        return mbPendingHiddenByTree == 1 || mbIsHiddenByTree == 1;
    }

    bool GetIsHiddenByTreeFlag() const
    {
        return (mbIsHiddenByTree == 1);
    }

    bool GetIsPendingHiddenByTree() const
    {
        return (mbPendingHiddenByTree == 1);
    }

    bool GetIsRootNonVisibleItem() const
    {
        return mbRootNonVisibleItem == 1 ? true : false;
    }

    static uint32_t sItemsAllocated;

  protected:
    /** @brief True if a Parent object is not visible visible */
    void SetIsHiddenByTree(bool newValue) const
    {
        mbPendingHiddenByTree = newValue ? 1 : 0;
        if (newValue == false)
        {
            // Can clear this immediately, but cannot set it immediately (to ensure thread safety)
            mbIsHiddenByTree = 0;
        }
    }

    void SetCharacterType(AptCharacterType type)
    {
        meCharacterType = type;
    }
    void PropagateTreeIsVisible(bool bNewState) const;

    AptRenderItem(AptCharacter *pCharacter, int32_t nCurrentTick);
    AptRenderItem(AptRenderItem *pObj, int32_t nCurrentTick, bool bCreateAsNewRevisionOfObj);

    virtual ~AptRenderItem();

    AptCharacter *mpCharacter;

  private:
    //////////////////////////////////////////////////////////////////////////
    // Render State field. Should only be accessed by CIH members and render traversal.
    // I.e. the manager should never mess with these as it should no knowledge of them.
    AptMatrix *mpPositionMatrix; // Might make this a pointer.
    AptCXForm *mpColorMatrix;    // Might make this a pointer.
    AptMatrix *mpMaskPositionMatrix;
#if defined(APT_3D)
    AptRI3DHelper *mp3DRotation;
#endif

    // The order of these is inefficient, should fix it once the structure solidifies.
    int16_t mnDepth;
    int16_t mnClipDepth;

    uint32_t mbIsVisible : 1;
    uint32_t mbIsMask : 1; // Can only mask one random object.
    uint32_t mbHasMask : 1;
    uint32_t mbIsDeletionMark : 1;
    uint32_t mbCreatedDynamic : 1;
    mutable uint32_t mbPendingHiddenByTree : 1; ///< Sets Forced writable when next revision is created.
    mutable uint32_t mbIsHiddenByTree : 1;      ///< Is writable despite the tick!
    mutable uint32_t mbRootNonVisibleItem : 1;  ///< true == we are the root non-visible.

    uint32_t meCharacterType : 6; ///< true == we are the root non-visible.

    mutable AptRenderItem *mpMask;

    //////////////////////////////////////////////////////////////////////////
    // Internal State. Should only be visible to the manager, The CIH and render traversal should never know about them.
    mutable volatile int32_t mnReferenceCount;

    const int32_t mnCreatedOnTick; // Should not change outside the constructor.

    // don't copy
    AptRenderItem(const AptRenderItem &);

    // Here are a few operators not allowed on this object, they are put in private scope to slap people who try. (They are not implemented)
    AptRenderItem &operator=(const AptRenderItem &r);
    AptRenderItem &operator*() const;

#if defined(APT_RI_REFCOUNT_DEBUG)
    static long RefCountCallCounter;
#endif
};

class AptRenderItemLevel : public AptRenderItem
{
  public:
    APT_RI_NEW_DELETE_OPERATORS
    // Added Metric defines
    AptRenderItemLevel(AptCharacter *pCharacter, int32_t nCurrentUpdateTick);
    AptRenderItemLevel(class AptRenderItemLevel *pObj, int32_t nCurrentUpdateTick, bool bCreateAsNewRevisionOfObj);
    virtual ~AptRenderItemLevel()
    {
    }

    virtual AptRenderItem *Clone(int32_t nCurrentUpdateTick, bool bCreateAsNewRevisionOfObj)
    {
        return new AptRenderItemLevel(this, nCurrentUpdateTick, bCreateAsNewRevisionOfObj);
    }
};

class AptRenderItemSprite : public AptRenderItem
{
  public:
    APT_RI_NEW_DELETE_OPERATORS
    // Added Metric defines
    AptRenderItemSprite(AptCharacter *pCharacter, int32_t nCurrentUpdateTick);
    AptRenderItemSprite(class AptRenderItemSprite *pObj, int32_t nCurrentUpdateTick, bool bCreateAsNewRevisionOfObj);
    virtual ~AptRenderItemSprite()
    {
    }

    virtual void PushRenderData(AptRenderingContext *pRenderingContext, AptMaskRenderOperation eMaskOperation, int32_t nMaskDepth) const;
    virtual void PushRenderDataAbsolute(AptRenderingContext *pRenderingContext) const;
    virtual void PopRenderData(AptRenderingContext *pRenderingContext, AptMaskRenderOperation eMaskOperation, int32_t nMaskDepth) const;

    virtual AptRenderItem *Clone(int32_t nCurrentUpdateTick, bool bCreateAsNewRevisionOfObj)
    {
        return new AptRenderItemSprite(this, nCurrentUpdateTick, bCreateAsNewRevisionOfObj);
    }

    virtual void Render(AptRenderingContext *pRenderingContext, AptMaskRenderOperation eMaskOperation, int32_t nMaskDepth, const AptCIH *pCIH) const;

    APT_INLINE uint32_t GetBlendMode() const
    {
        return mnBlendMode;
    }

    APT_INLINE void SetBlendMode(uint32_t nBlendMode)
    {
        mnBlendMode = nBlendMode;
    }

#if defined(APT_RENDER_FLAGS)
    APT_INLINE const AptNativeString &GetRenderPropertiesStr() const
    {
        return msRenderFlags;
    }

    APT_INLINE void SetRenderPropertiesStr(const AptNativeString &sProperties)
    {
        msRenderFlags = sProperties;
    }

  protected:
    AptNativeString msRenderFlags;
#endif
    uint32_t mnBlendMode; // flash supported numbers are from 0-13 something and we can use remaining bits for something else.
};

class AptRenderItemAnimation : public AptRenderItemSprite
{
  public:
    APT_RI_NEW_DELETE_OPERATORS
    // Added Metric defines

    AptRenderItemAnimation(AptCharacter *pCharacter, int32_t nCurrentUpdateTick);
    AptRenderItemAnimation(class AptRenderItemAnimation *pObj, int32_t nCurrentUpdateTick, bool bCreateAsNewRevisionOfObj);
    virtual ~AptRenderItemAnimation()
    {
    }

    virtual AptRenderItem *Clone(int32_t nCurrentUpdateTick, bool bCreateAsNewRevisionOfObj)
    {
        return new AptRenderItemAnimation(this, nCurrentUpdateTick, bCreateAsNewRevisionOfObj);
    }

    virtual void Render(AptRenderingContext *pRenderingContext, AptMaskRenderOperation eMaskOperation, int32_t nMaskDepth, const AptCIH *pCIH) const;
    ;
};

class AptRenderItemCustomControl : public AptRenderItemSprite
{
  public:
    APT_RI_NEW_DELETE_OPERATORS
    // Added Metric defines

    AptRenderItemCustomControl(AptCharacter *pCharacter, int32_t nCurrentUpdateTick);
    AptRenderItemCustomControl(class AptRenderItemCustomControl *pObj, int32_t nCurrentUpdateTick, bool bCreateAsNewRevisionOfObj);
    AptRenderItemCustomControl(class AptRenderItemSprite *pObj, int32_t nCurrentUpdateTick, bool bCreateAsNewRevisionOfObj);
    virtual ~AptRenderItemCustomControl();

    void PushRenderData(AptRenderingContext *pRenderingContext, AptMaskRenderOperation eMaskOperation, int32_t nMaskDepth) const;
    void PushRenderDataAbsolute(AptRenderingContext *pRenderingContext) const;
    void PopRenderData(AptRenderingContext *pRenderingContext, AptMaskRenderOperation eMaskOperation, int32_t nMaskDepth) const;

    virtual AptRenderItem *Clone(int32_t nCurrentUpdateTick, bool bCreateAsNewRevisionOfObj)
    {
        return new AptRenderItemCustomControl(this, nCurrentUpdateTick, bCreateAsNewRevisionOfObj);
    }

    static AptRenderItemCustomControl *CopyFromSprite(class AptRenderItemSprite *pObj, int32_t nCurrentUpdateTick, bool bCreateAsNewRevisionOfObj);

    virtual void Render(AptRenderingContext *pRenderingContext, AptMaskRenderOperation eMaskOperation, int32_t nMaskDepth, const AptCIH *pCIH) const;

    APT_INLINE const AptNativeString &GetCustomPropertiesStr() const
    {
        return m_szProperties;
    }

    APT_INLINE void SetCustomPropertiesStr(const AptNativeString &szProperties)
    {
        m_szProperties = szProperties;
    }

    APT_INLINE void SetTypeStr(const AptNativeString &szType)
    {
        m_szType = szType;
    }

    APT_INLINE const AptNativeString &GetTypeStr() const
    {
        return m_szType;
    }

    APT_INLINE void SetTargetStr(const AptNativeString &szTarget)
    {
        m_szTarget = szTarget;
    }

    APT_INLINE const AptNativeString &GetTargetStr() const
    {
        return m_szTarget;
    }

#if defined(APT_CUSTOM_CONTROL_USE_ZID)
    APT_INLINE AptRenderableCustomControl *GetRenderable() const
    {
        return mpCustomControlRender;
    }
    APT_INLINE void SetZId(AptAssetCustomControlZId zid)
    {
        if (!mpCustomControlRender || (mpCustomControlRender && mpCustomControlRender->mZid != zid))
        {
            SafeRelease(mpCustomControlRender);
            mpCustomControlRender = new AptRenderableCustomControl(zid);
            mStoredZid            = zid; // Store the ZID in variable that will be passed by reference to various functions
        }
    }
    APT_INLINE void ClearZId()
    {
        if (mpCustomControlRender)
        {
            mStoredZid            = NULL; // Kill the stored ZID so that anything that was passed a reference to the stored ZID will know that their ZID is invalid
            mpCustomControlRender = SafeRelease(mpCustomControlRender);
        }
    }
    // Return a reference to the stored ZID, so that when it get's cleared, the function that has the reference will be able to detect that
    APT_INLINE AptAssetCustomControlZId *GetZidRef()
    {
        return &mStoredZid;
    }
#endif

  protected:
    AptNativeString m_szType;
    AptNativeString m_szTarget;
    AptNativeString m_szProperties;

#if defined(APT_CUSTOM_CONTROL_USE_ZID)
    AptRenderableCustomControl *mpCustomControlRender;
    /**
     * When attempting to render a custom control, it's possible that the passed in ZID will become invalid.
     * Noteably, when a video is played, the end of video callback will often clear the video, which causes the ZID
     * to be deleted. Attempting to use that deleted ZID causes a crash. Rather than pass in the ZID, we pass in a
     * pointer to the variable that holds the ZID, so the Custom Render code can detect when the ZID is cleared.
     */
    AptAssetCustomControlZId mStoredZid;
#endif

  private:
    // don't copy
    AptRenderItemCustomControl(const AptRenderItemCustomControl &);
    AptRenderItemCustomControl &operator=(const AptRenderItemCustomControl &);
};

class AptRenderItemButton : public AptRenderItemSprite
{
  public:
    APT_RI_NEW_DELETE_OPERATORS
    // Added Metric defines

    AptRenderItemButton(AptCharacter *pCharacter, int32_t nCurrentUpdateTick);
    AptRenderItemButton(class AptRenderItemButton *pObj, int32_t nCurrentUpdateTick, bool bCreateAsNewRevisionOfObj);
    virtual ~AptRenderItemButton()
    {
    }

    virtual AptRenderItem *Clone(int32_t nCurrentUpdateTick, bool bCreateAsNewRevisionOfObj)
    {
        return new AptRenderItemButton(this, nCurrentUpdateTick, bCreateAsNewRevisionOfObj);
    }

    virtual void Render(AptRenderingContext *pRenderingContext, AptMaskRenderOperation eMaskOperation, int32_t nMaskDepth, const AptCIH *pCIH) const;
};

struct TextFormat; // Forward declaration.

class AptRenderItemDynamicText : public AptRenderItem
{
  public:
    APT_RI_NEW_DELETE_OPERATORS
    // Added Metric defines
    AptRenderItemDynamicText(AptCharacter *pCharacter, int32_t nCurrentUpdateTick);
    AptRenderItemDynamicText(class AptRenderItemDynamicText *pObj, int32_t nCurrentUpdateTick, bool bCreateAsNewRevisionOfObj);
    virtual ~AptRenderItemDynamicText();

    virtual void Render(AptRenderingContext *pRenderingContext, AptMaskRenderOperation eMaskOperation, int32_t nMaskDepth, const AptCIH *pCIH) const;

    // void EnsureStringAllocated() const ;      // not needed now.
    virtual AptRenderItem *Clone(int32_t nCurrentUpdateTick, bool bCreateAsNewRevisionOfObj)
    {
        return new AptRenderItemDynamicText(this, nCurrentUpdateTick, bCreateAsNewRevisionOfObj);
    }

    const AptNativeString &GetTextValueConst() const
    {
        return mTextValue;
    }

    AptNativeString &GetTextValueWritable()
    {
        return mTextValue;
    }

    void SetTextValue(const AptNativeString &newValue)
    {
        mTextValue = newValue;
    }

    const AptNativeString &GetVarValueConst() const
    {
        return mVarValue;
    }

    AptNativeString &GetVarValueWritable()
    {
        return mVarValue;
    }
    void SetVarValue(const AptNativeString &newValue)
    {
        mVarValue = newValue;
    }

    const AptRenderableString *GetRenderable() const
    {
        return mpRenderString;
    }

    void SetZID(AptAssetString newValue);
    uint32_t GetTextColor() const
    {
        return mnColour;
    }

    void SetTextColor(uint32_t newTextColor)
    {
        mnColour = (newTextColor);
    }

    uint32_t GetScroll() const
    {
        return mnScroll;
    }

    void SetScroll(uint32_t newScroll)
    {
        mnScroll = (newScroll);
    }

    uint32_t GetBackgroundColor() const
    {
        return mnBackColor | 0xFF000000;
    }

    void SetBackgroundColor(uint32_t newBackgroundColor)
    {
        mnBackColor = (newBackgroundColor);
    }

    uint32_t GetBorderColor() const
    {
        return mnBorderColor | 0xFF000000;
    }

    void SetBorderColor(uint32_t newBackgroundColor)
    {
        mnBorderColor = (newBackgroundColor);
    }

    AptStringAlignment GetBoxAlignment() const
    {
        return meBoxAlignment;
    }

    void SetBoxAlignment(AptStringAlignment newBoxAlignment)
    {
        meBoxAlignment = (newBoxAlignment);
    }

    AptStringAlignment GetAlignment() const
    {
        return meAlignment;
    }

    void SetAlignment(AptStringAlignment newAlignment)
    {
        meAlignment = (newAlignment);
    }

    const AptRect &GetBoundsConst() const
    {
        return mrBounds;
    }

    AptRect &GetBoundsWritable()
    {
        return mrBounds;
    }

    void SetBounds(const AptRect &newBounds)
    {
        mrBounds = (newBounds);
    }

    float GetFontSize() const
    {
        return mfFontSize;
    }

    void SetFontSize(float newFontSize)
    {
        mfFontSize = (newFontSize);
    }

    int GetFontID() const
    {
        return mnFontID;
    }

    const TextFormat *GetTextFormatConst() const
    {
        return mpMyTextFormat;
    }

    TextFormat *GetTextFormatWritable()
    {
        return mpMyTextFormat;
    }

    void SetTextFormat(TextFormat *pNewFormat);

    void ClearStateFlags(uint32_t uFlags)
    {
        meFlags &= ~(uFlags);
    }

    void SetStateFlags(uint32_t uFlags)
    {
        meFlags |= (uFlags);
    }

    uint32_t GetStateFlags() const
    {
        return meFlags;
    }

    bool GetDrawsBorder() const
    {
        return mbBorder;
    }

    void SetDrawsBorder(bool bDrawsBorder)
    {
        mbBorder = (bDrawsBorder);
    }

    bool GetDrawsBackground() const
    {
        return mbBackground;
    }

    void SetDrawsBackground(bool bDrawsBackground)
    {
        mbBackground = (bDrawsBackground);
    }

    bool GetMouseWheelEnabled() const
    {
        return mbMouseWheelEnabled;
    }

    void SetMouseWheelEnabled(bool newValue)
    {
        mbMouseWheelEnabled = (newValue);
    }

    bool GetWordWrap() const
    {
        return mbWordWrap;
    }

    void SetWordWrap(bool bEnable)
    {
        mbWordWrap = bEnable;
    }

    bool GetMultiline() const
    {
        return mbMultiline;
    }

    void SetMultiline(bool bEnable)
    {
        mbMultiline = bEnable;
    }

    APT_INLINE uint32_t GetBlendMode() const
    {
        return mnBlendMode;
    }

    APT_INLINE void SetBlendMode(uint32_t nBlendMode)
    {
        mnBlendMode = nBlendMode;
    }

    APT_INLINE int GetLeading() const
    {
        return mnLeading;
    }

    APT_INLINE void SetLeading(int leading)
    {
        mnLeading = leading;
    }

    APT_INLINE int GetTracking() const
    {
        return mnTracking;
    }

    APT_INLINE void SetTracking(int tracking)
    {
        mnTracking = tracking;
    }

  protected:
    AptNativeString mTextValue;
    AptNativeString mVarValue;
    mutable AptRenderableString *mpRenderString;
    uint32_t mnColour; // turned on again in 0.11.00
    int32_t mnScroll;

    uint32_t mnBackColor : 24;
    uint32_t mbBackground : 1;

    AptStringAlignment meAlignment : 4; // We need to start keeping a local copy since we can update this now

    uint32_t mnBorderColor : 24;
    uint32_t mbBorder : 1;
    uint32_t mbMouseWheelEnabled : 1; // added support for TextMouseWheel
    AptStringAlignment meBoxAlignment : 4;
    uint32_t mbWordWrap : 1;
    uint32_t mbMultiline : 1;

    AptRect mrBounds; // Added to update boundary box
    float mfFontSize; // We need to start keeping a local copy since we can update this now
    int32_t mnFontID; // We need to start keeping a local copy since we can update this now
    TextFormat *mpMyTextFormat;
    uint32_t meFlags;         // Text Field update flags added
    uint32_t mnBlendMode : 8; // flash supported numbers are from 0-13.
    int mnLeading : 24;
    int mnTracking : 24;

  private:
    // don't copy
    AptRenderItemDynamicText(const AptRenderItemDynamicText &);
    AptRenderItemDynamicText &operator=(const AptRenderItemDynamicText &);
};

class AptRenderItemStaticText : public AptRenderItem
{
  public:
    APT_RI_NEW_DELETE_OPERATORS
    // Added Metric defines
    AptRenderItemStaticText(AptCharacter *pCharacter, int32_t nCurrentUpdateTick);
    AptRenderItemStaticText(class AptRenderItemStaticText *pObj, int32_t nCurrentUpdateTick, bool bCreateAsNewRevisionOfObj);
    virtual ~AptRenderItemStaticText()
    {
    }

    virtual AptRenderItem *Clone(int32_t nCurrentUpdateTick, bool bCreateAsNewRevisionOfObj)
    {
        return new AptRenderItemStaticText(this, nCurrentUpdateTick, bCreateAsNewRevisionOfObj);
    }

    virtual void Render(AptRenderingContext *pRenderingContext, AptMaskRenderOperation eMaskOperation, int32_t nMaskDepth, const AptCIH *pCIH) const;
};

class AptRenderItemMorph : public AptRenderItem
{
  public:
    APT_RI_NEW_DELETE_OPERATORS
    // Added Metric defines
    AptRenderItemMorph(AptCharacter *pCharacter, int32_t nCurrentUpdateTick);
    AptRenderItemMorph(class AptRenderItemMorph *pObj, int32_t nCurrentUpdateTick, bool bCreateAsNewRevisionOfObj);
    virtual ~AptRenderItemMorph()
    {
    }

    virtual AptRenderItem *Clone(int32_t nCurrentUpdateTick, bool bCreateAsNewRevisionOfObj)
    {
        return new AptRenderItemMorph(this, nCurrentUpdateTick, bCreateAsNewRevisionOfObj);
    }

    virtual void Render(AptRenderingContext *pRenderingContext, AptMaskRenderOperation eMaskOperation, int32_t nMaskDepth, const AptCIH *pCIH) const;

    APT_INLINE float GetRatio() const
    {
        return fRatio;
    }

  private:
    float fRatio;
};

// Render item representing a simple Texture that has been loaded, not as part of any SWF or similar
class AptRenderItemLoadedTexture : public AptRenderItem
{
  public:
    APT_RI_NEW_DELETE_OPERATORS

    AptRenderItemLoadedTexture(AptCharacter *character, int currentUpdateTick);
    AptRenderItemLoadedTexture(AptRenderItemLoadedTexture *obj, int currentUpdateTick, bool createAsNewRevisionOfObj);
    virtual ~AptRenderItemLoadedTexture();

    virtual AptRenderItem *Clone(int currentUpdateTick, bool createAsNewRevisionOfObj)
    {
        return new AptRenderItemLoadedTexture(this, currentUpdateTick, createAsNewRevisionOfObj);
    }

    virtual void Render(AptRenderingContext *pRenderingContext, AptMaskRenderOperation eMaskOperation, int32_t nMaskDepth, const AptCIH *pCIH) const;

  private:
    AptRenderableImage *mLoadedTextureRenderable;
};

class AptRenderItemShape : public AptRenderItem
{
  public:
    APT_RI_NEW_DELETE_OPERATORS
    // Added Metric defines
    AptRenderItemShape(AptCharacter *pCharacter, int32_t nCurrentUpdateTick);
    AptRenderItemShape(class AptRenderItemShape *pObj, int32_t nCurrentUpdateTick, bool bCreateAsNewRevisionOfObj);
    virtual ~AptRenderItemShape()
    {
    }

    virtual AptRenderItem *Clone(int32_t nCurrentUpdateTick, bool bCreateAsNewRevisionOfObj)
    {
        return new AptRenderItemShape(this, nCurrentUpdateTick, bCreateAsNewRevisionOfObj);
    }

    virtual void Render(AptRenderingContext *pRenderingContext, AptMaskRenderOperation eMaskOperation, int32_t nMaskDepth, const AptCIH *pCIH) const;
    ;

    // added this function to make sure that all the textures from SWF files are loaded before any shape from that SWF file is
    // rendered. This also makes sure that pfnLoadTexture gets called from render thread like it used to get called before lockless Apt.
#if defined(APT_DECOUPLED_RENDERING)
    virtual void LoadResourcesFromCharacter() const
    {
        mpCharacter->LoadResourcesFromCharacter();
    }
#endif
};

#if defined(APT_3D)
// Structure used to maintain 3D space coords for RenderItems.
struct AptRI3DHelper
{
  public:
    APT_NEW_DELETE_OPERATORS

    AptRI3DHelper()
    {
    }

    void AptRI3DHelperCopy(const AptRI3DHelper *pRotHelper)
    {
        mfZ      = pRotHelper->mfZ;
        mfZScale = pRotHelper->mfZScale;
        mfXRot   = pRotHelper->mfXRot;
        mfYRot   = pRotHelper->mfYRot;
    }

    // Getter and Setter methods for the 3D variables
    APT_INLINE float GetZPosition() const { return mfZ; }
    APT_INLINE float GetZScale() const { return mfZScale; }
    APT_INLINE float GetXRotation() const { return mfXRot; }
    APT_INLINE float GetYRotation() const { return mfYRot; }
    APT_INLINE void SetZPosition(float zPos) { mfZ = zPos; }
    APT_INLINE void SetZScale(float zScale) { mfZScale = zScale; }
    APT_INLINE void SetXRotation(float xRot) { mfXRot = xRot; }
    APT_INLINE void SetYRotation(float yRot) { mfYRot = yRot; }

  private:
    float mfZ{0.f};
    float mfZScale{1.f};
    float mfXRot{0.f};
    float mfYRot{0.f};
};
#endif

/*** Variables ************************************************************************************/

/*** Functions ************************************************************************************/

