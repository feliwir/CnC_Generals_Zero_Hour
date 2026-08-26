#pragma once

/*** Include files ********************************************************************************/
#include "AptRenderItem.h"
#include "Display/AptDisplayList.h"
#include "_AptValue.h"
#include "AptCharacter/AptCharacter.h"
#include "AptRenderableString.h"

/*** Defines **************************************************************************************/

/*** Macros ***************************************************************************************/

/*** Type Definitions *****************************************************************************/
class AptRenderItem;
class AptRenderItemSprite;
class AptRenderItemDynamicText;
class AptRenderItemStaticText;
class AptRenderItemMorph;
class AptRenderItemButton;
class AptRenderItemAnimation;
class AptRenderItemShape;
class AptRenderItemCustomControl;
class AptCharacterImageInst;
struct AptAnimationFile;

class AptCharacterInst
{
  public:

#if defined(APT_GATHER_MOVIECLIP_METRICS)
    static void UpdateMovieClipInfo(AptCharacterType eType, AptMovieclipInformation *pMCInfo, int nChange);
#endif

    static AptCharacterInst *CreateCharacterInst(AptCharacter *pCharacter);
    void CopyRenderDataFrom(const AptCharacterInst *pModel);
    void MoveRenderDataFrom(const AptCharacterInst *pModel);

    //! virtual AptUpdateItemType GetCharacterType(void) const = 0;
    AptCharacterType GetCharacterType(void) const
    {
        return (AptCharacterType)mnCharInstType;
    }

    //----------------------------------------------------------------------------------------------
    // Simple and repetitive functions that return true if the object is of the specified type.

    bool IsSpriteInst(void) const
    {
        return GetCharacterType() == AptCharacterType_Sprite || GetCharacterType() == AptCharacterType_CustomControl;
    }
    bool IsSpriteInstBase(void) const
    {
        return GetCharacterType() == AptCharacterType_Sprite || GetCharacterType() == AptCharacterType_Animation;
    }
    bool IsAnimationInst(void) const
    {
        return GetCharacterType() == AptCharacterType_Animation;
    }
    bool IsButtonInst(void) const
    {
        return GetCharacterType() == AptCharacterType_Button;
    }
    bool IsDynamicTextInst(void) const
    {
        return GetCharacterType() == AptCharacterType_Text;
    }
    bool IsStaticTextInst(void) const
    {
        return GetCharacterType() == AptCharacterType_StaticText;
    }
    bool IsMorphInst(void) const
    {
        return GetCharacterType() == AptCharacterType_Morph;
    }
    bool IsShapeInst(void) const
    {
        return GetCharacterType() == AptCharacterType_Shape;
    }
    bool IsLevelInst(void) const
    {
        return GetCharacterType() == AptCharacterType_Level;
    }
    bool IsImageInst(void) const
    {
        return GetCharacterType() == AptCharacterType_Image;
    }
    bool IsCustomControlInst(void) const
    {
        return GetCharacterType() == AptCharacterType_CustomControl;
    }

    //----------------------------------------------------------------------------------------------
    // Update Item Functions
    AptNativeHash *GetNativeHash(void) const
    {
        return mpNativeHash;
    }
    bool ContainsNativeHash(void) const;
    APT_INLINE void DestroyGCPointers(void);
    void RegisterReferences(void) const;

    //----------------------------------------------------------------------------------------------
    // Render Item Functions

    APT_INLINE const AptMatrix *GetPositionMatrixConst(void) const;
    APT_INLINE const AptCXForm *GetColorMatrixConst(void) const;
    APT_INLINE int32_t GetClipDepth(void) const;

    APT_INLINE AptMatrix *GetPositionMatrixWritable(void);
    APT_INLINE AptCXForm *GetColorMatrixWritable(void);
    APT_INLINE void SetClipDepth(int32_t nDepth);

#if defined(APT_3D)
    APT_INLINE float GetZ(void) const;
    APT_INLINE void SetZ(float z);

    APT_INLINE float GetZScale(void) const;
    APT_INLINE void SetZScale(float zScale);

    APT_INLINE float GetYRot(void) const;
    APT_INLINE void SetYRot(float yRot);

    APT_INLINE float GetXRot(void) const;
    APT_INLINE void SetXRot(float xRot);
#endif // #if defined(APT_3D)

    //----------------------------------------------------------------------------------------------
    // Render Item Management Functions
    static APT_INLINE void ItemMoved(AptCIH *pItemOwner);
    static APT_INLINE void ItemInserted(AptCIH *pItemOwner);
    static APT_INLINE void SetRootItem(AptCIH *pRoot);
    static APT_INLINE void CloneItem(const AptCIH *pJoeShmoe, AptCIH *pWantsJoesRenderState);

    //----------------------------------------------------------------------------------------------
    // Informational / Status Functions
    APT_INLINE AptCharacterInst *GetCharacterInst(void)
    {
        APT_ASSERT(this);
        return this;
    }
    APT_INLINE AptCharacterSpriteInst *GetSpriteInst(void)
    {
        APT_ASSERT(IsSpriteInst());
        return (AptCharacterSpriteInst *)this;
    }
    APT_INLINE AptCharacterSpriteInstBase *GetSpriteInstBase(void)
    {
        APT_ASSERT(IsSpriteInstBase());
        return (AptCharacterSpriteInstBase *)this;
    }
    APT_INLINE AptCharacterTextInst *GetDynamicTextInst(void)
    {
        APT_ASSERT(IsDynamicTextInst());
        return (AptCharacterTextInst *)this;
    }
    APT_INLINE AptCharacterStaticTextInst *GetStaticTextInst(void)
    {
        APT_ASSERT(IsStaticTextInst());
        return (AptCharacterStaticTextInst *)this;
    }
    APT_INLINE AptCharacterMorphInst *GetMorphInst(void)
    {
        APT_ASSERT(IsMorphInst());
        return (AptCharacterMorphInst *)this;
    }
    APT_INLINE AptCharacterButtonInst *GetButtonInst(void)
    {
        APT_ASSERT(IsButtonInst());
        return (AptCharacterButtonInst *)this;
    }
    APT_INLINE AptCharacterAnimationInst *GetAnimationInst(void)
    {
        APT_ASSERT(IsAnimationInst());
        return (AptCharacterAnimationInst *)this;
    }
    APT_INLINE AptCharacterShapeInst *GetShapeInst(void)
    {
        APT_ASSERT(IsShapeInst());
        return (AptCharacterShapeInst *)this;
    }
    APT_INLINE AptCharacterLevelInst *GetLevelInst(void)
    {
        APT_ASSERT(IsLevelInst());
        return (AptCharacterLevelInst *)this;
    }
    APT_INLINE AptCharacterImageInst *GetImageInst(void)
    {
        APT_ASSERT(IsImageInst());
        return (AptCharacterImageInst *)this;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////
    // Getters. *const*, accessible by all.

    APT_INLINE AptCharacter *GetCharacterWritable(void);
    APT_INLINE const AptCharacter *GetCharacterConst(void) const;

    APT_INLINE int32_t GetDepth(void) const;
    APT_INLINE bool GetIsMask(void) const;
    APT_INLINE bool GetHasMask(void) const;
    APT_INLINE bool GetIsVisible(void) const;

    APT_INLINE void SetDepth(int32_t newDepth);
    APT_INLINE void SetCharacter(AptCharacter *pCharacter);
    APT_INLINE void SetIsMask(bool bIsMask, AptMatrix *pMatrix);
    APT_INLINE void SetHasMask(bool bNewState, AptRenderItem *pMask);
    APT_INLINE void SetIsVisible(bool bNewState);

    virtual uint32_t GetBlendMode() const
    {
        return 0;
    }

    virtual void SetBlendMode(uint32_t /* nBlendMode */) {}

    /**
        @brief Default implementation provides access to this object's members. derived classes should override these
        to provide access to their members if they have any. Those should still call these when done.
    */
    const char *GetCustomRenderData(void) const;
    void SetCustomRenderData(const char *pNewValue);

    const AptRenderItem *mpRenderItem{0}; // Don't use this directly, always get the manager to make sure it is the latest.

    explicit AptCharacterInst(AptCharacter *pCharacter);
    virtual ~AptCharacterInst(void);

    APT_INLINE const AptRenderItem *GetRenderItem(void) const;
    AptRenderItem *GetRenderItemWritable(void);
    void SetRenderItem(AptRenderItem *pItem);

    //! Sets native has to pNativeHash
    void SetNativeHash(AptNativeHash *pNativeHash)
    {
        mpNativeHash = pNativeHash;
    }

  protected:
    // Render Items are hidden to keep us encapsulated.

    APT_INLINE const AptRenderItemSprite *GetRenderItemSprite(void) const;
    APT_INLINE const AptRenderItemDynamicText *GetRenderItemDynamicText(void) const;
    APT_INLINE const AptRenderItemStaticText *GetRenderItemStaticText(void) const;
    APT_INLINE const AptRenderItemMorph *GetRenderItemMorph(void) const;
    APT_INLINE const AptRenderItemButton *GetRenderItemButton(void) const;
    APT_INLINE const AptRenderItemAnimation *GetRenderItemAnimation(void) const;
    APT_INLINE const AptRenderItemShape *GetRenderItemShape(void) const;
    APT_INLINE const AptRenderItemCustomControl *GetRenderItemCustomControl(void) const;

    APT_INLINE AptRenderItemSprite *GetRenderItemSpriteWritable(void);
    APT_INLINE AptRenderItemDynamicText *GetRenderItemDynamicTextWritable(void);
    APT_INLINE AptRenderItemStaticText *GetRenderItemStaticTextWritable(void);
    APT_INLINE AptRenderItemMorph *GetRenderItemMorphWritable(void);
    APT_INLINE AptRenderItemButton *GetRenderItemButtonWritable(void);
    APT_INLINE AptRenderItemAnimation *GetRenderItemAnimationWritable(void);
    APT_INLINE AptRenderItemShape *GetRenderItemShapeWritable(void);
    APT_INLINE AptRenderItemCustomControl *GetRenderItemCustomControlWritable(void);

    // Ease of using an enum here for debugging comes with the penalty of a wasted bit.
    // This is because enum's are signed. If you set the highest bit it will be sign extended
    // thus turning a 15 into a -1, etc. :-0
    AptCharacterType mnCharInstType : 6;

    AptNativeHash *mpNativeHash;
};

class AptCharacterButtonInst : public AptCharacterInst
{
  public:
#if defined APT_USE_BUTTONS
    explicit AptCharacterButtonInst(AptCharacter *pCharacter);

    virtual void PreDestroy(void);

    void UpdateObjectMethods(const AptNativeString *pVar, AptCIH *pCIH, int32_t bRemove = 0);

    APT_INLINE uint32_t GetBlendMode() const;
    APT_INLINE void SetBlendMode(uint32_t nBlendMode);

    AptCharacterButtonRecordState mnState;
    AptDisplayList mDisplayList;

#else
    explicit AptCharacterButtonInst(AptCharacter *pCharacter) : AptCharacterInst(pCharacter)
    {
        APT_ASSERT(0 && "YOU COMPILED THIS OUT, COMPILE IT BACK IN TO USE BUTTONS")
    }

    void UpdateObjectMethods(const AptNativeString *pVar, AptCIH *pCIH, int32_t bRemove = 0) {}
#endif
    APT_NEW_DELETE_OPERATORS
    // Added Metric defines
};

class AptCharacterShapeInst : public AptCharacterInst
{
  public:
    explicit AptCharacterShapeInst(AptCharacter *pCharacter) : AptCharacterInst(pCharacter)
    {
    }

    APT_NEW_DELETE_OPERATORS
    // Added Metric defines
};

class AptCharacterStaticTextInst : public AptCharacterInst
{
  public:
    explicit AptCharacterStaticTextInst(AptCharacter *pCharacter) : AptCharacterInst(pCharacter)
    {
    }

    APT_NEW_DELETE_OPERATORS
    // Added Metric defines
};

class AptCharacterMorphInst : public AptCharacterInst
{
  public:
    explicit AptCharacterMorphInst(AptCharacter *pCharacter) : AptCharacterInst(pCharacter)
    {
    }

    float_t mfRatio;

    APT_NEW_DELETE_OPERATORS
    // Added Metric defines
};

class AptCharacterSpriteInstBase : public AptCharacterInst
{
  public:
    explicit AptCharacterSpriteInstBase(AptCharacter *pCharacter);

    int32_t mnFrame;
    int32_t mnObjectClipActions : 24;
    uint32_t mbJustLoaded : 1;
    uint32_t mbIsPlaying : 1;
    uint32_t mnIsCustomControl : 2; // CustomControlState

    AptEventActionSet *mpClipActions;
    AptDisplayList mDisplayList;
    int32_t mnGotoAnded; // flag used to check frame number validity on this CharInst

    virtual ~AptCharacterSpriteInstBase(void);

    virtual void PreDestroy(void);

    //! Set the event in the ClipActions
    APT_INLINE void SetClipAction(int32_t nEvent)
    {
        mnObjectClipActions |= nEvent;
    }
    //! Removes the event from the ClipActions
    APT_INLINE void RemoveClipAction(int32_t nEvent)
    {
        mnObjectClipActions &= ~nEvent;
    }
    //! Checks if the event is part of the ClipActions
    APT_INLINE int32_t HasClipAction(int32_t nEvent)
    {
        return (mnObjectClipActions & nEvent);
    }

    bool GetCreatedDynamic(void) const;
    void SetCreatedDynamic(bool bNewValue);

    /**
        @brief for getting/setting uint32_t            mnBlendMode inside AptRenderItemSprite
        tried making these APT_INLINE, but somehow ps3 bulkbuild=false gives error for that.
    */
    uint32_t GetBlendMode() const;
    void SetBlendMode(uint32_t nBlendMode);

    enum CustomControlState
    {
        CustomControlState_Unknown            = 0,
        CustomControlState_IsCustomControl    = 1,
        CustomControlState_IsNotCustomControl = 2,
        CustomControlState_IsCustomControlZid = 3
    };

  private:
    AptCharacterSpriteInstBase(const AptCharacterSpriteInstBase &);
    AptCharacterSpriteInstBase &operator=(const AptCharacterSpriteInstBase &);
};

/** @brief Apt-specific class used for direct-load images */
class AptCharacterImageInst : public AptCharacterInst
{
  public:
    explicit AptCharacterImageInst(AptCharacter *character, AptFilePtr file)
        : AptCharacterInst(character), mFile(file)
    {
    }

    APT_NEW_DELETE_OPERATORS

    // private:
    AptFilePtr mFile;
};

class AptCharacterSpriteInst : public AptCharacterSpriteInstBase
{
  public:
    explicit AptCharacterSpriteInst(AptCharacter *pCharacter) : AptCharacterSpriteInstBase(pCharacter)
    {
    }

#if defined(APT_PLATFORM_WINDOWS) || (defined(APT_PLATFORM_MICROSOFT) && defined(APT_PLATFORM_CONSOLE) && !defined(APT_PLATFORM_XENON))
    /** @brief returns the name of the object where the movie clip was dropped. */
    APT_INLINE const AptNativeString &GetDropTarget() const
    {
        return mszDropTarget;
    }

    /** @brief Sets a new drop target for the object */
    APT_INLINE void SetDropTarget(const AptNativeString &newTarget)
    {
        mszDropTarget = newTarget;
    }
#endif

    APT_NEW_DELETE_OPERATORS
    // Added Metric defines

  private:
#if defined(APT_PLATFORM_WINDOWS) || (defined(APT_PLATFORM_MICROSOFT) && defined(APT_PLATFORM_CONSOLE) && !defined(APT_PLATFORM_XENON))
    // Drag and drop is only applicable to movie clips and not to any AptCIH so we define this here to save memory
    AptNativeString mszDropTarget; // to hold the dropped target symbol name when drag 'n dropping
#endif
};

class AptCharacterAnimationInst : public AptCharacterSpriteInstBase
{
  public:
    explicit AptCharacterAnimationInst(AptCharacter *pCharacter, AptFilePtr file);

    virtual ~AptCharacterAnimationInst(void);

    uint32_t mnLeftoverTime;
    AptFilePtr mpFile;

  public:
    int32_t GetSwfVersion(void);
    virtual void PreDestroy(void);

  private:
    APT_NEW_DELETE_OPERATORS
    // Added Metric defines
};

struct TextFormat; // Forward declaration.

// this structure is moved after AptCharacter as
// it needs to refer to the destructor of one of its members i.e. pCharacter
class AptCharacterTextInst : public AptCharacterInst
{
  public:
    explicit AptCharacterTextInst(AptCharacter *pCharacter);

    // AptNativeString     mTextValue;
    APT_INLINE const AptNativeString &GetTextValueConst(void) const;
    APT_INLINE AptNativeString &GetTextValueWritable(void);
    APT_INLINE void SetTextValue(const AptNativeString &newValue);

    // AptNativeString     mVarValue;
    APT_INLINE const AptNativeString &GetVarValueConst(void) const;
    APT_INLINE AptNativeString &GetVarValueWritable(void);
    APT_INLINE void SetVarValue(const AptNativeString &newValue);

    // AptAssetString      zID;
    APT_INLINE AptRenderableString *GetRenderable(void);
    APT_INLINE void SetZID(const AptAssetString &newValue);

    // uint32_t        nColour;            // turned on again in 0.11.00
    APT_INLINE uint32_t GetTextColor(void) const;
    APT_INLINE void SetTextColor(uint32_t newTextColor);

    // int32_t                 nMaxScroll;
    APT_INLINE uint32_t GetMaxScroll(void) const;
    APT_INLINE void SetMaxScroll(uint32_t maxScroll);

    // int32_t                 nScroll;
    APT_INLINE uint32_t GetScroll(void) const;
    APT_INLINE void SetScroll(uint32_t newScroll);

    // uint32_t        nBackColor;
    APT_INLINE uint32_t GetBackgroundColor(void) const;
    APT_INLINE void SetBackgroundColor(uint32_t newBackgroundColor);

    // uint32_t        nBorderColor;
    APT_INLINE uint32_t GetBorderColor(void) const;
    APT_INLINE void SetBorderColor(uint32_t newBackgroundColor);

    // AptStringAlignment  eBoxAlignment;
    APT_INLINE AptStringAlignment GetBoxAlignment(void) const;
    APT_INLINE void SetBoxAlignment(AptStringAlignment newBoxAlignment);

    // AptStringAlignment  eAlignment;         // We need to start keeping a local copy since we can update this now
    APT_INLINE AptStringAlignment GetAlignment(void) const;
    APT_INLINE void SetAlignment(AptStringAlignment newAlignment);

    // float_t               fTextWidth;
    APT_INLINE float_t GetTextWidth(void) const;
    APT_INLINE void SetTextWidth(float_t newTextWidth);
    // float_t               fTextHeight;
    APT_INLINE float_t GetTextHeight(void) const;
    APT_INLINE void SetTextHeight(float_t newTextHeight);

    // float_t               fLength;
    APT_INLINE float_t GetLength(void) const;
    APT_INLINE void SetLength(float_t newLength);

    // AptRect             rBounds;            // Added to update boundary box
    APT_INLINE const AptRect &GetBoundsConst(void) const;
    APT_INLINE AptRect &GetBoundsWritable(void);
    APT_INLINE void SetBounds(const AptRect &newBounds);

    // float_t               fFontSize;          // We need to start keeping a local copy since we can update this now
    APT_INLINE float_t GetFontSize(void) const;
    APT_INLINE void SetFontSize(float_t newFontSize);

    // int32_t                 nFontID;            // We need to start keeping a local copy since we can update this now
    APT_INLINE int32_t GetFontID(void) const;

    // TextFormat *        pMyTextFormat;
    APT_INLINE const TextFormat *GetTextFormatConst(void) const;
    APT_INLINE TextFormat *GetTextFormatWritable(void);
    APT_INLINE void SetTextFormat(TextFormat *pNewFormat);

    // uint32_t        eFlags;             // Text Field update flags added
    APT_INLINE void ClearStateFlags(uint32_t uFlags);
    APT_INLINE void SetStateFlags(uint32_t uFlags);
    APT_INLINE uint32_t GetStateFlags(void) const;

    // uint32_t            bCreatedDynamic:1;
    APT_INLINE bool GetCreatedDynamic(void) const;
    APT_INLINE void SetCreatedDynamic(bool newValue);

    // uint32_t            bBorder:1;
    APT_INLINE bool GetDrawsBorder(void) const;
    APT_INLINE void SetDrawsBorder(bool bDrawsBorder);

    // uint32_t            bBackground:1;
    APT_INLINE bool GetDrawsBackground(void) const;
    APT_INLINE void SetDrawsBackground(bool bDrawsBackground);

    // uint32_t            bMouseWheelEnabled:1;   // added support for TextMouseWheel
    APT_INLINE bool GetMouseWheelEnabled(void) const;
    APT_INLINE void SetMouseWheelEnabled(bool newValue);

    // uint32_t            bWordWrap:1;
    APT_INLINE bool GetWordWrap() const;
    APT_INLINE void SetWordWrap(bool bEnable);

    // uint32_t            bMultiline:1;
    APT_INLINE bool GetMultiline() const;
    APT_INLINE void SetMultiline(bool bEnable);

    // uint32_t            mnBlendMode;
    APT_INLINE uint32_t GetBlendMode() const;
    APT_INLINE void SetBlendMode(uint32_t nBlendMode);

    APT_INLINE int GetLeading() const;
    APT_INLINE void SetLeading(const int leading);

    APT_INLINE int GetTracking() const;
    APT_INLINE void SetTracking(const int tracking);

    virtual ~AptCharacterTextInst(void);

    void SetText(AptCIH *const pParent);
    void UpdateText(AptCIH *const pParent);

    APT_NEW_DELETE_OPERATORS
    // Added Metric defines

  private:
    // Dynamically generated properties, -- moved to character inst.

    int32_t mnMaxScroll;  // Cached Property.
    float_t mfTextWidth;  // Cached Property.
    float_t mfTextHeight; // Cached Property.
    float_t mfLength;     // Cached Property.  (Why is this a float_t).
};

class AptCharacterLevelInst : public AptCharacterInst
{
  public:
    explicit AptCharacterLevelInst(AptCharacter *pCharacter) : AptCharacterInst(pCharacter)
    {
    }

    APT_NEW_DELETE_OPERATORS
    // Added Metric defines
};

/*** Variables ************************************************************************************/

/*** Functions ************************************************************************************/

