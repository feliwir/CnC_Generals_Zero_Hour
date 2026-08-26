/*** Include files ********************************************************************************/
#include "AptRenderItem.h" // Needs This
// #include "AptRenderItem.inl" // Needs This
#include "AptBCRenderTree.h"
#include "AptTarget.h"
/*** Defines **************************************************************************************/

/*** Macros ***************************************************************************************/

/*** Type Definitions *****************************************************************************/

/*** Function Prototypes **************************************************************************/

/*** Variables ************************************************************************************/

// Private variables

// Public variables

/*** Private Functions ****************************************************************************/

/*** Public Functions *****************************************************************************/

//----------------------------------------------------------------------------------------------
// Get Render item const and writable are provided first because other call these and expect them to be inlined.
const AptRenderItem *AptCharacterInst::GetRenderItem(void) const
{
    APT_ASSERT(mpRenderItem);
    return mpRenderItem;
}

//----------------------------------------------------------------------------------------------
// Give out our character inst type casted
const AptRenderItemSprite *AptCharacterInst::GetRenderItemSprite(void) const
{
    APT_ASSERT(IsSpriteInst() || IsAnimationInst());
    return (const AptRenderItemSprite *)GetRenderItem();
}

const AptRenderItemDynamicText *AptCharacterInst::GetRenderItemDynamicText(void) const
{
    APT_ASSERT(IsDynamicTextInst());
    return (const AptRenderItemDynamicText *)GetRenderItem();
}

const AptRenderItemStaticText *AptCharacterInst::GetRenderItemStaticText(void) const
{
    APT_ASSERT(IsStaticTextInst());
    return (const AptRenderItemStaticText *)this;
}

const AptRenderItemMorph *AptCharacterInst::GetRenderItemMorph(void) const
{
    APT_ASSERT(IsMorphInst());
    return (const AptRenderItemMorph *)GetRenderItem();
}

const AptRenderItemButton *AptCharacterInst::GetRenderItemButton(void) const
{
    APT_ASSERT(IsButtonInst());
    return (const AptRenderItemButton *)GetRenderItem();
}

const AptRenderItemAnimation *AptCharacterInst::GetRenderItemAnimation(void) const
{
    APT_ASSERT(IsAnimationInst());
    return (const AptRenderItemAnimation *)GetRenderItem();
}

const AptRenderItemShape *AptCharacterInst::GetRenderItemShape(void) const
{
    APT_ASSERT(IsShapeInst());
    return (const AptRenderItemShape *)GetRenderItem();
}

const AptRenderItemCustomControl *AptCharacterInst::GetRenderItemCustomControl(void) const
{
    APT_ASSERT(IsCustomControlInst());
    return (const AptRenderItemCustomControl *)GetRenderItem();
}

AptRenderItemSprite *AptCharacterInst::GetRenderItemSpriteWritable(void)
{
    APT_ASSERT(IsSpriteInst());
    return (AptRenderItemSprite *)GetRenderItemWritable();
}

AptRenderItemDynamicText *AptCharacterInst::GetRenderItemDynamicTextWritable(void)
{
    APT_ASSERT(IsDynamicTextInst());
    return (AptRenderItemDynamicText *)GetRenderItemWritable();
}

AptRenderItemStaticText *AptCharacterInst::GetRenderItemStaticTextWritable(void)
{
    APT_ASSERT(IsStaticTextInst());
    return (AptRenderItemStaticText *)GetRenderItemWritable();
}

AptRenderItemMorph *AptCharacterInst::GetRenderItemMorphWritable(void)
{
    APT_ASSERT(IsMorphInst());
    return (AptRenderItemMorph *)GetRenderItemWritable();
}

AptRenderItemButton *AptCharacterInst::GetRenderItemButtonWritable(void)
{
    APT_ASSERT(IsButtonInst());
    return (AptRenderItemButton *)GetRenderItemWritable();
}

AptRenderItemAnimation *AptCharacterInst::GetRenderItemAnimationWritable(void)
{
    APT_ASSERT(IsAnimationInst());
    return (AptRenderItemAnimation *)GetRenderItemWritable();
}

AptRenderItemShape *AptCharacterInst::GetRenderItemShapeWritable(void)
{
    APT_ASSERT(IsShapeInst());
    return (AptRenderItemShape *)GetRenderItemWritable();
}

AptRenderItemCustomControl *AptCharacterInst::GetRenderItemCustomControlWritable(void)
{
    APT_ASSERT(IsCustomControlInst());
    return (AptRenderItemCustomControl *)GetRenderItemWritable();
}

const AptMatrix *AptCharacterInst::GetPositionMatrixConst(void) const
{
    return GetRenderItem()->GetPositionMatrixConst();
}

const AptCXForm *AptCharacterInst::GetColorMatrixConst(void) const
{
    return GetRenderItem()->GetColorMatrixConst();
}

int32_t AptCharacterInst::GetClipDepth(void) const
{
    return GetRenderItem()->GetClipDepth();
}

#if defined(APT_3D)
float AptCharacterInst::GetZ(void) const
{
    return GetRenderItem()->GetZPosition();
}

void AptCharacterInst::SetZ(float z)
{
    GetRenderItemWritable()->SetZPosition(z);
}

float AptCharacterInst::GetZScale(void) const
{
    return GetRenderItem()->GetZScale();
}

void AptCharacterInst::SetZScale(float zScale)
{
    GetRenderItemWritable()->SetZScale(zScale);
}

float AptCharacterInst::GetYRot(void) const
{
    return GetRenderItem()->GetYRotation();
}

void AptCharacterInst::SetYRot(float yRot)
{
    GetRenderItemWritable()->SetYRotation(yRot);
}

float AptCharacterInst::GetXRot(void) const
{
    return GetRenderItem()->GetXRotation();
}

void AptCharacterInst::SetXRot(float xRot)
{
    GetRenderItemWritable()->SetXRotation(xRot);
}
#endif // #if defined(APT_3D)

AptMatrix *AptCharacterInst::GetPositionMatrixWritable(void)
{
    return GetRenderItemWritable()->GetPositionMatrixWritable();
}

AptCXForm *AptCharacterInst::GetColorMatrixWritable(void)
{
    return GetRenderItemWritable()->GetColorMatrixWritable();
}

void AptCharacterInst::SetClipDepth(int32_t nDepth)
{
    GetRenderItemWritable()->SetClipDepth(nDepth);
}

void AptCharacterInst::ItemMoved(AptCIH *pItemOwner)
{
    GetTargetSim()->GetRenderManager()->Update_ItemMoved(pItemOwner, AptGetLib()->mnCurrUpdateTick);
}

void AptCharacterInst::ItemInserted(AptCIH *pItemOwner)
{
    GetTargetSim()->GetRenderManager()->Update_ItemInserted(pItemOwner, AptGetLib()->mnCurrUpdateTick);
}

void AptCharacterInst::CloneItem(const AptCIH *pJoeShmoe, AptCIH *pWantsJoesRenderState)
{
    GetTargetSim()->GetRenderManager()->Update_CloneItem(pJoeShmoe, pWantsJoesRenderState, AptGetLib()->mnCurrUpdateTick);
}

void AptCharacterInst::SetRootItem(AptCIH *pRoot)
{
    GetTargetSim()->GetRenderManager()->Update_SetRootItem(pRoot, AptGetLib()->mnCurrUpdateTick);
}

int32_t AptCharacterInst::GetDepth(void) const
{
    return GetRenderItem()->GetDepth();
}

const AptCharacter *AptCharacterInst::GetCharacterConst(void) const
{
    return GetRenderItem()->GetCharacterConst();
}

AptCharacter *AptCharacterInst::GetCharacterWritable(void)
{
    return GetRenderItemWritable()->GetCharacterWritable();
}

bool AptCharacterInst::GetIsMask(void) const
{
    return GetRenderItem()->GetIsMask();
}

bool AptCharacterInst::GetHasMask(void) const
{
    return GetRenderItem()->GetHasMask();
}

bool AptCharacterInst::GetIsVisible(void) const
{
    return GetRenderItem()->GetIsVisible();
}

void AptCharacterInst::SetDepth(int32_t newDepth)
{
    GetRenderItemWritable()->SetDepth(newDepth);
}

void AptCharacterInst::SetCharacter(AptCharacter *pCharacter)
{
    GetRenderItemWritable()->SetCharacter(pCharacter);
}

void AptCharacterInst::SetIsMask(bool bIsMask, AptMatrix *pMatrix)
{
    GetRenderItemWritable()->SetIsMask(bIsMask, pMatrix);
}
void AptCharacterInst::SetHasMask(bool bNewState, AptRenderItem *pMask)
{
    GetRenderItemWritable()->SetHasMask(bNewState, pMask);
}
void AptCharacterInst::SetIsVisible(bool bNewState)
{
    GetRenderItemWritable()->SetIsVisible(bNewState);
}

void AptCharacterInst::DestroyGCPointers(void)
{
    if (mpRenderItem != NULL)
    {
        GetTargetSim()->GetRenderManager()->Update_ItemRemoved(GetRenderItemWritable(), AptGetLib()->mnCurrUpdateTick);
    }

    if (mpNativeHash != NULL)
    {
        mpNativeHash->DestroyGCPointers();
        delete mpNativeHash;
        mpNativeHash = NULL;
    }
}

const AptNativeString &AptCharacterTextInst::GetTextValueConst(void) const
{
    return GetRenderItemDynamicText()->GetTextValueConst();
}

AptNativeString &AptCharacterTextInst::GetTextValueWritable(void)
{
    return GetRenderItemDynamicTextWritable()->GetTextValueWritable();
}

void AptCharacterTextInst::SetTextValue(const AptNativeString &newValue)
{
    GetRenderItemDynamicTextWritable()->SetTextValue(newValue);
}

const AptNativeString &AptCharacterTextInst::GetVarValueConst(void) const
{
    return GetRenderItemDynamicText()->GetVarValueConst();
}

AptNativeString &AptCharacterTextInst::GetVarValueWritable(void)
{
    return GetRenderItemDynamicTextWritable()->GetVarValueWritable();
}

void AptCharacterTextInst::SetVarValue(const AptNativeString &newValue)
{
    GetRenderItemDynamicTextWritable()->SetVarValue(newValue);
}

AptRenderableString *AptCharacterTextInst::GetRenderable(void)
{
    return const_cast<AptRenderableString *>(GetRenderItemDynamicText()->GetRenderable());
}

void AptCharacterTextInst::SetZID(const AptAssetString &newValue)
{
    GetRenderItemDynamicTextWritable()->SetZID(newValue);
}

uint32_t AptCharacterTextInst::GetTextColor(void) const
{
    return GetRenderItemDynamicText()->GetTextColor();
}

void AptCharacterTextInst::SetTextColor(uint32_t newTextColor)
{
    GetRenderItemDynamicTextWritable()->SetTextColor(newTextColor);
}

uint32_t AptCharacterTextInst::GetMaxScroll(void) const
{
    return mnMaxScroll;
}

void AptCharacterTextInst::SetMaxScroll(uint32_t maxScroll)
{
    mnMaxScroll = (maxScroll);
}

uint32_t AptCharacterTextInst::GetScroll(void) const
{
    return GetRenderItemDynamicText()->GetScroll();
}

void AptCharacterTextInst::SetScroll(uint32_t newScroll)
{
    GetRenderItemDynamicTextWritable()->SetScroll(newScroll);
}

uint32_t AptCharacterTextInst::GetBackgroundColor(void) const
{
    return GetRenderItemDynamicText()->GetBackgroundColor();
}

void AptCharacterTextInst::SetBackgroundColor(uint32_t newBackgroundColor)
{
    GetRenderItemDynamicTextWritable()->SetBackgroundColor(newBackgroundColor);
}

uint32_t AptCharacterTextInst::GetBorderColor(void) const
{
    return GetRenderItemDynamicText()->GetBorderColor();
}

void AptCharacterTextInst::SetBorderColor(uint32_t newBackgroundColor)
{
    GetRenderItemDynamicTextWritable()->SetBorderColor(newBackgroundColor);
}

AptStringAlignment AptCharacterTextInst::GetBoxAlignment(void) const
{
    return GetRenderItemDynamicText()->GetBoxAlignment();
}

void AptCharacterTextInst::SetBoxAlignment(AptStringAlignment newBoxAlignment)
{
    GetRenderItemDynamicTextWritable()->SetBoxAlignment(newBoxAlignment);
}

AptStringAlignment AptCharacterTextInst::GetAlignment(void) const
{
    return GetRenderItemDynamicText()->GetAlignment();
}

void AptCharacterTextInst::SetAlignment(AptStringAlignment newAlignment)
{
    GetRenderItemDynamicTextWritable()->SetAlignment(newAlignment);
}

float_t AptCharacterTextInst::GetTextWidth(void) const
{
    return mfTextWidth;
}

void AptCharacterTextInst::SetTextWidth(float_t newTextWidth)
{
    mfTextWidth = (newTextWidth);
}

float_t AptCharacterTextInst::GetTextHeight(void) const
{
    return mfTextHeight;
}

void AptCharacterTextInst::SetTextHeight(float_t newTextHeight)
{
    mfTextHeight = (newTextHeight);
}

float_t AptCharacterTextInst::GetLength(void) const
{
    return mfLength;
}

void AptCharacterTextInst::SetLength(float_t newLength)
{
    mfLength = (newLength);
}

const AptRect &AptCharacterTextInst::GetBoundsConst(void) const
{
    return GetRenderItemDynamicText()->GetBoundsConst();
}

AptRect &AptCharacterTextInst::GetBoundsWritable(void)
{
    return GetRenderItemDynamicTextWritable()->GetBoundsWritable();
}

void AptCharacterTextInst::SetBounds(const AptRect &newBounds)
{
    GetRenderItemDynamicTextWritable()->SetBounds(newBounds);
}

float_t AptCharacterTextInst::GetFontSize(void) const
{
    return GetRenderItemDynamicText()->GetFontSize();
}

void AptCharacterTextInst::SetFontSize(float_t newFontSize)
{
    GetRenderItemDynamicTextWritable()->SetFontSize(newFontSize);
}

int32_t AptCharacterTextInst::GetFontID(void) const
{
    return GetRenderItemDynamicText()->GetFontID();
}

const TextFormat *AptCharacterTextInst::GetTextFormatConst(void) const
{
    return GetRenderItemDynamicText()->GetTextFormatConst();
}

TextFormat *AptCharacterTextInst::GetTextFormatWritable(void)
{
    return GetRenderItemDynamicTextWritable()->GetTextFormatWritable();
}

void AptCharacterTextInst::SetTextFormat(TextFormat *pNewFormat)
{
    GetRenderItemDynamicTextWritable()->SetTextFormat(pNewFormat);
}

void AptCharacterTextInst::ClearStateFlags(uint32_t uFlags)
{
    GetRenderItemDynamicTextWritable()->ClearStateFlags(uFlags);
}

void AptCharacterTextInst::SetStateFlags(uint32_t uFlags)
{
    GetRenderItemDynamicTextWritable()->SetStateFlags(uFlags);
}

uint32_t AptCharacterTextInst::GetStateFlags(void) const
{
    return GetRenderItemDynamicText()->GetStateFlags();
}

bool AptCharacterTextInst::GetCreatedDynamic(void) const
{
    return GetRenderItemDynamicText()->GetCreatedDynamic();
}

void AptCharacterTextInst::SetCreatedDynamic(bool newValue)
{
    GetRenderItemDynamicTextWritable()->SetCreatedDynamic(newValue);
}

bool AptCharacterTextInst::GetDrawsBorder(void) const
{
    return GetRenderItemDynamicText()->GetDrawsBorder();
}

void AptCharacterTextInst::SetDrawsBorder(bool bDrawsBorder)
{
    GetRenderItemDynamicTextWritable()->SetDrawsBorder(bDrawsBorder);
}

bool AptCharacterTextInst::GetDrawsBackground(void) const
{
    return GetRenderItemDynamicText()->GetDrawsBackground();
}

void AptCharacterTextInst::SetDrawsBackground(bool bDrawsBackground)
{
    GetRenderItemDynamicTextWritable()->SetDrawsBackground(bDrawsBackground);
}

bool AptCharacterTextInst::GetMouseWheelEnabled(void) const
{
    return GetRenderItemDynamicText()->GetMouseWheelEnabled();
}

void AptCharacterTextInst::SetMouseWheelEnabled(bool newValue)
{
    GetRenderItemDynamicTextWritable()->SetMouseWheelEnabled(newValue);
}

bool AptCharacterTextInst::GetWordWrap(void) const
{
    return GetRenderItemDynamicText()->GetWordWrap();
}

void AptCharacterTextInst::SetWordWrap(bool newValue)
{
    GetRenderItemDynamicTextWritable()->SetWordWrap(newValue);
}

bool AptCharacterTextInst::GetMultiline(void) const
{
    return GetRenderItemDynamicText()->GetMultiline();
}

void AptCharacterTextInst::SetMultiline(bool newValue)
{
    GetRenderItemDynamicTextWritable()->SetMultiline(newValue);
}

void AptCharacterTextInst::SetBlendMode(uint32_t nBlendMode)
{
    GetRenderItemDynamicTextWritable()->SetBlendMode(nBlendMode);
}

uint32_t AptCharacterTextInst::GetBlendMode() const
{
    return GetRenderItemDynamicText()->GetBlendMode();
}

int AptCharacterTextInst::GetLeading() const
{
    return GetRenderItemDynamicText()->GetLeading();
}

void AptCharacterTextInst::SetLeading(const int leading)
{
    GetRenderItemDynamicTextWritable()->SetLeading(leading);
}

int AptCharacterTextInst::GetTracking() const
{
    return GetRenderItemDynamicText()->GetTracking();
}

void AptCharacterTextInst::SetTracking(const int tracking)
{
    GetRenderItemDynamicTextWritable()->SetTracking(tracking);
}

#if defined APT_USE_BUTTONS
void AptCharacterButtonInst::SetBlendMode(uint32_t nBlendMode)
{
    GetRenderItemButtonWritable()->SetBlendMode(nBlendMode);
}

uint32_t AptCharacterButtonInst::GetBlendMode() const
{
    return GetRenderItemButton()->GetBlendMode();
}
#endif
