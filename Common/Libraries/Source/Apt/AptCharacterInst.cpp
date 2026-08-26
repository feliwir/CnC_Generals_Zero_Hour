/*** Include files ********************************************************************************/

#include "_Apt.h"
#include "AptCIH.h"
#include "AptCharacterInst.h"
#include "AptRenderItem.h"
#include "AptObject/AptGlobalObject.h"

#include "MainInline.h"

#if !defined(APT_ENABLE_INLINE)
#include "AptCharacterInst.inl"
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

#if defined(APT_GATHER_MOVIECLIP_METRICS)
void AptCharacterInst::UpdateMovieClipInfo(AptCharacterType eType, AptMovieclipInformation *pMCInfo, int nChange)
{
    switch (eType)
    {
    case AptCharacterType_Shape:
        pMCInfo->nShapes += nChange;
        break;
    case AptCharacterType_Sprite:
        pMCInfo->nMovieClips += nChange;
        break;
    case AptCharacterType_Button:
#if defined APT_USE_BUTTONS
        pMCInfo->nButtons += nChange;
#endif
        break;
    case AptCharacterType_Text:
        pMCInfo->nDynamicText += nChange;
        break;
    case AptCharacterType_StaticText:
        pMCInfo->nStaticText += nChange;
        break;
    case AptCharacterType_Morph:
        pMCInfo->nMorph += nChange;
        break;
    case AptCharacterType_Animation:
        pMCInfo->nAnimations += nChange;
        break;
    case AptCharacterType_CustomControl:
        pMCInfo->nCustomControls += nChange;
        break;
    default:
        break;
    }
}
#endif

AptCharacterInst *AptCharacterInst::CreateCharacterInst(AptCharacter *pCharacter)
{
    AptCharacterInst *pInst = NULL;

    if (pCharacter == NULL) // we will create a level inst for this.
    {
        pInst = new AptCharacterLevelInst(NULL);
        return pInst;
    }
    switch (pCharacter->eType)
    {
    case AptCharacterType_Sprite:
        pInst = new AptCharacterSpriteInst(pCharacter);
        break;
    case AptCharacterType_Text:
        pInst = new AptCharacterTextInst(pCharacter);
        break;
    case AptCharacterType_StaticText:
        pInst = new AptCharacterStaticTextInst(pCharacter);
        break;
    case AptCharacterType_Shape:
        pInst = new AptCharacterShapeInst(pCharacter);
        break;
    case AptCharacterType_Morph:
        pInst = new AptCharacterMorphInst(pCharacter);
        break;

    case AptCharacterType_Button:
    {
#if defined APT_USE_BUTTONS
        {
            pInst = new AptCharacterButtonInst(pCharacter);
        }
#else
        {
            APT_FAIL("Buttons are not compiled into this version, please recompile with buttons enabled.");
        }
#endif
        break;
    }

    default:
    {
        APT_FAIL("Uncertain how to make an instance for this type");
        break;
    }
    }
    return pInst;
}

AptCharacterInst::AptCharacterInst(AptCharacter *pCharacter) : mpNativeHash(NULL)
{
    mpRenderItem   = (GetTargetSim() == NULL) ? NULL : GetTargetSim()->GetRenderManager()->Update_CreateItem(pCharacter, AptGetLib()->mnCurrUpdateTick);
    mnCharInstType = pCharacter ? pCharacter->eType : AptCharacterType_Level;

#if defined(APT_GATHER_MOVIECLIP_METRICS)
    AptCharacterInst::UpdateMovieClipInfo(mnCharInstType, &gAptMovieclipInformation, 1);
#endif

    if (mpRenderItem != NULL)
        APT_RI_INC(mpRenderItem, this, "CharacterInst");
}

AptCharacterInst::~AptCharacterInst(void)
{
    // This assert points out that we did not call destroy GC pointers before deleting the object.
    //  This will happen however, when the object is garbage collected (swept). It points out minor
    //  imperfections in GC handling but nothing to worry about (Perhaps to be addressed in a
    //  future major version update.. if we ever run out of real problems to address :).

    APT_ASSERT(mpNativeHash == NULL);
    if (mpNativeHash != NULL)
    {
        delete mpNativeHash;
        mpNativeHash = NULL;
    }

#if defined(APT_GATHER_MOVIECLIP_METRICS)
    if (mpRenderItem != NULL)
        AptCharacterInst::UpdateMovieClipInfo(mpRenderItem->GetCharacterType(), &gAptMovieclipInformation, -1);
    else
        AptCharacterInst::UpdateMovieClipInfo(mnCharInstType, &gAptMovieclipInformation, -1);
#endif

    // APT_ASSERT(mpRenderItem != NULL);
    if (mpRenderItem != NULL)
        APT_RI_DEC(mpRenderItem, this, "CharacterInst");
    mpRenderItem = NULL;
}

AptRenderItem *AptCharacterInst::GetRenderItemWritable(void)
{
    return const_cast<AptRenderItem *>(mpRenderItem);
}

void AptCharacterInst::SetRenderItem(AptRenderItem *pItem)
{
    APT_ASSERT(mpRenderItem);
    if (pItem != mpRenderItem)
    {
        APT_RI_INC(pItem, this, "CharacterInst");
        APT_RI_DEC(mpRenderItem, this, "CharacterInst");
        mpRenderItem = pItem;
    }
}

void AptCharacterInst::MoveRenderDataFrom(const AptCharacterInst *pModel)
{
    CopyRenderDataFrom(pModel);

    SetDepth(pModel->GetDepth());

    if (pModel->mpNativeHash != NULL)
    {
        if (mpNativeHash != NULL)
        {
            mpNativeHash->DestroyGCPointers();
            delete mpNativeHash;
        }

        mpNativeHash = pModel->mpNativeHash;

        AptCharacterInst *pTemp = (AptCharacterInst *)pModel;
        pTemp->mpNativeHash     = NULL;
    }
}

void AptCharacterInst::CopyRenderDataFrom(const AptCharacterInst *pModel)
{
    /*lint -e(613) */ GetRenderItemWritable()->CopyRenderDataFrom(pModel->GetRenderItem());
}

#if defined APT_USE_BUTTONS
AptCharacterButtonInst::AptCharacterButtonInst(AptCharacter *pCharacter) : AptCharacterInst(pCharacter),
                                                                           mDisplayList()
{
    SetNativeHash(new AptNativeHash(4));
    mnState = AptCharacterButtonRecordState_None;
}
#endif

AptCharacterSpriteInstBase::AptCharacterSpriteInstBase(AptCharacter *pCharacter) : AptCharacterInst(pCharacter), mDisplayList()
{
    mnFrame             = -1;
    mpClipActions       = 0;
    mnGotoAnded         = 0;
    mbJustLoaded        = 1;
    mbIsPlaying         = 1;
    mnIsCustomControl   = (uint32_t)CustomControlState_Unknown;
    mnObjectClipActions = AptEventActionFlag_Invalid;

    SetNativeHash(new AptNativeHash(APT_OBJECTHASHSIZE));

    // by default set the __proto__ to MovieClip.prototype and then overwrite it if
    // class registration is found in AssociateInstToClass
    // This will take care of setting __proto__ for every movieclip derived from
    // AptCharacterSpriteInstBase which could be AptCharacterSpriteInst or
    // could be AptChracterAnimationInst (the very basic movieclip created in AptLinker::Update())
    AptValue *pMovieClip          = AptGetLib()->mpGlobalGlobalObject->Lookup(StringPool::GetString(SC_MovieClip));
    AptValue *pPrototypeMovieClip = pMovieClip->GetNativeHashVirtual()->GetPrototype();
    GetNativeHash()->Set__Proto__(pPrototypeMovieClip);
}

AptCharacterSpriteInstBase::~AptCharacterSpriteInstBase(void)
{
    mpClipActions = NULL;
}

bool AptCharacterSpriteInstBase::GetCreatedDynamic(void) const
{
    return GetRenderItemSprite()->GetCreatedDynamic();
}
void AptCharacterSpriteInstBase::SetCreatedDynamic(bool bNewValue)
{
    GetRenderItemSpriteWritable()->SetCreatedDynamic(bNewValue);
}

void AptCharacterSpriteInstBase::PreDestroy(void)
{
    mDisplayList.PreDestroy();
}

void AptCharacterTextInst::SetText(AptCIH *const pParent)
{
    const AptNativeString &varValue = GetVarValueConst();
    if (varValue.IsEmpty())
    {
        return;
    }

    if (varValue.c_str()[0] == '$')
    {
        SetTextValue(varValue); // Still need to set mTextValue. ()
        return;
    }

    // This text is associated with a variable
    AptCIH *pLocalParent = pParent;

    while (pLocalParent && !pLocalParent->IsSpriteInstBase())
    {
        if (pLocalParent->GetDisplayListParent() == NULL)
        {
            break;
        }
        pLocalParent = pLocalParent->GetDisplayListParent();
    }

    AptValue *pValue = gAptActionInterpreter.getVariable(pLocalParent, NULL, &varValue);
    if (pValue->isUndefined())
    {
        //  The value is undefined, we assume it's because the variable doesn't exist
        //  But we have the same result if the variable contains undefined
        //  There is no way yet to detect between the two cases, this can be a difference between Flash and Apt

        //  We need to create the variable with the initialized text
        AptString *pString = AptString::Create();
        if (GetCharacterConst()->text.szInitialText != NULL)
        {
            pString->str = GetCharacterConst()->text.szInitialText;
        }
        else
        {
            pString->str = "";
        }

        SetTextValue(pString->str);
        if (gAptActionInterpreter.setVariable(pLocalParent, NULL, &varValue, pString) == false)
        {
            // For an unknown reason we couldn't set the variable
            //  The string is allocated but nobody will keep a pointer on it
            //  If we don't delete the string here, the deferred release will catch it
        }
    }
    else
    {
        //  The value is defined, set the accordingly
        pValue->toString(GetTextValueWritable());
    }
}

void AptCharacterTextInst::UpdateText(AptCIH *const pParent)
{
    const AptNativeString &varValue = GetVarValueConst();

    if (varValue.IsEmpty())
    {
        return;
    }

    if (varValue.c_str()[0] == '$')
    {
        return;
    }

    // This text is associated with a variable
    AptNativeString strNewText;
    AptCIH *pLocalParent = pParent;

    while (pLocalParent && !pLocalParent->IsSpriteInstBase())
    {
        if (pLocalParent->GetDisplayListParent() == NULL)
        {
            break;
        }
        pLocalParent = pLocalParent->GetDisplayListParent();
    }

    AptValue *pValue = gAptActionInterpreter.getVariable(pLocalParent, NULL, &varValue);
    APT_ASSERT(pValue != NULL);

    if (pValue->isUndefined()) //  If the value was undefined, we use an empty string
    {
        if (GetCharacterConst()->text.szInitialText)
        {
            strNewText = GetCharacterConst()->text.szInitialText;
        }
        else
        {
            strNewText = "";
        }
    }
    else
    {
        // There is an associated variable, use it
        pValue->toString(strNewText);
    }

    if (GetTextValueConst() != strNewText)
    {
        //  The text is different, update it
        SetTextValue(strNewText);

        // Note these can be done in the SetTextValue Functions
        ClearStateFlags(APT_TEXTFIELD_NONE);
        SetStateFlags(APT_TEXTFIELD_FUPDATE);
    }
}

AptCharacterTextInst::AptCharacterTextInst(AptCharacter *pCharacter) : AptCharacterInst(pCharacter)
{
    mnMaxScroll  = 0;
    mfTextWidth  = 0;
    mfTextHeight = 0;
    mfLength     = 0;
}

AptCharacterTextInst::~AptCharacterTextInst(void)
{
}

void AptCharacterSpriteInstBase::SetBlendMode(uint32_t nBlendMode)
{
    GetRenderItemSpriteWritable()->SetBlendMode(nBlendMode);
}

uint32_t AptCharacterSpriteInstBase::GetBlendMode() const
{
    return GetRenderItemSprite()->GetBlendMode();
}
