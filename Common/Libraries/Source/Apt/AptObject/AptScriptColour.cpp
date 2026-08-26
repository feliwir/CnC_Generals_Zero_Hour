#include "_Apt.h"
#include "AptObject/AptScriptColour.h"
#include "AptCIH.h"
#include "AptCharacterInst.h"
#include "AptValue/AptInteger.h"
#include "AptValue/AptFloat.h"

#include "MainInline.h"

#if APT_USE_SCRIPTCOLOUR_OBJECT

AptScriptColour::AptScriptColour(AptValue *const pMovie)
    : AptObject(AptVFT_ScriptColour)
{
    // Allows text fields to obtain a color transform without asserting.
    if (pMovie != NULL && pMovie->isCIH())
    {
        AptCIH *pCih = pMovie->c_cih();
        if (pCih->IsSpriteInst() || pCih->IsDynamicTextInst() || pCih->IsImageInst())
        {
            pSprite = pCih;
            APT_INC(pSprite);
        }
        else
        {
            pSprite = 0;
        }
    }
    else
    {
        pSprite = 0;
    }
}
#endif

AptScriptColour::~AptScriptColour()
{
    // Do not touch GC objects in the destructor.
    pSprite = NULL;
}

void AptScriptColour::CleanNativeFunctions()
{
#if APT_USE_SCRIPTCOLOUR_OBJECT
    NATIVE_MEMBER_FUNCTION_DESTROY(setRGB);
    NATIVE_MEMBER_FUNCTION_DESTROY(getRGB);
    NATIVE_MEMBER_FUNCTION_DESTROY(getTransform);
    NATIVE_MEMBER_FUNCTION_DESTROY(setTransform);
#endif
}

#if APT_USE_SCRIPTCOLOUR_OBJECT

AptValue *AptScriptColour::objectMemberLookup(AptValue *const pContext, const AptNativeString *const pName) const
{
    if (pName->CompareNoCase(*StringPool::GetString(SC_setRGB)) == 0)
    {
        NATIVE_MEMBER_FUNCTION_DISPATCH(setRGB);
    }
    else if (pName->CompareNoCase(*StringPool::GetString(SC_getRGB)) == 0)
    {
        NATIVE_MEMBER_FUNCTION_DISPATCH(getRGB);
    }
    else if (pName->CompareNoCase(*StringPool::GetString(SC_getTransform)) == 0)
    {
        NATIVE_MEMBER_FUNCTION_DISPATCH(getTransform);
    }
    else if (pName->CompareNoCase(*StringPool::GetString(SC_setTransform)) == 0)
    {
        NATIVE_MEMBER_FUNCTION_DISPATCH(setTransform);
    }

    return (NULL);
}

NATIVE_MEMBER_FUNCTION(AptScriptColour, setRGB)
{
    AptValue *pParam = gAptActionInterpreter.stackAt(0);
    AptCIH *pSprite  = pThis->c_scriptcolour()->pSprite;
    if (pSprite)
    {
        int nInt          = pParam->toInteger();
        AptCXForm *cxform = pSprite->GetColorMatrixWritable(); // Per Flash docs, setRGB doesn't touch the alpha transform.
        cxform->translate.SetValuef(AptColorHelper::Red, (nInt >> 16) & 0xFF);
        cxform->translate.SetValuef(AptColorHelper::Green, (nInt >> 8) & 0xFF);
        cxform->translate.SetValuef(AptColorHelper::Blue, (nInt >> 0) & 0xFF);

        cxform->scale.SetValuef(AptColorHelper::Red, 0.f);
        cxform->scale.SetValuef(AptColorHelper::Green, 0.f);
        cxform->scale.SetValuef(AptColorHelper::Blue, 0.f);
        pSprite->SetASChanged(true);
    }
    return gpUndefinedValue;
}

NATIVE_MEMBER_FUNCTION(AptScriptColour, getRGB)
{
    AptCIH *pSprite = pThis->c_scriptcolour()->pSprite;
    if (pSprite)
    {
        const AptColorHelperTranslate &vColorAdd4 = pSprite->GetColorMatrixConst()->translate;
        float fSrcR                               = vColorAdd4.GetValuef(AptColorHelper::Red);
        float fSrcG                               = vColorAdd4.GetValuef(AptColorHelper::Green);
        float fSrcB                               = vColorAdd4.GetValuef(AptColorHelper::Blue);

        if (fSrcR < 0.f)
            fSrcR = 0.0f;
        if (fSrcG < 0.f)
            fSrcG = 0.0f;
        if (fSrcB < 0.f)
            fSrcB = 0.0f;

        if (fSrcR > 255.f)
            fSrcR = 255.0f;
        if (fSrcG > 255.f)
            fSrcG = 255.0f;
        if (fSrcB > 255.f)
            fSrcB = 255.0f;

        uint32_t uDst = 0;
        uDst |= ((uint32_t)fSrcR) << 16;
        uDst |= ((uint32_t)fSrcG) << 8;
        uDst |= ((uint32_t)fSrcB) << 0;
        return AptInteger::Create((int)uDst);
    }
    return gpUndefinedValue;
}

NATIVE_MEMBER_FUNCTION(AptScriptColour, getTransform)
{
    if (nParams > 0)
        return gpUndefinedValue;

    AptScriptColour *pTemp = pThis->c_scriptcolour();
    APT_ASSERT(pTemp);
    AptCIH *pSprite = pTemp->pSprite;

    // Flash admits undefined parameters here.
    if (pSprite && pSprite->getIsDefined())
    {
        AptCharacterInst *pCharInst = pSprite->GetCharacterInst();
        AptObject *pObject          = new AptObject(AptVFT_Object);

        const AptCXForm *cxform = pCharInst->GetColorMatrixConst();

#if defined(APT_USE_FLASH_COLOR_RANGE)
        AptValue *pRA = AptFloat::Create((cxform->scale.GetValuef(AptColorHelper::Red)));
        AptValue *pGA = AptFloat::Create((cxform->scale.GetValuef(AptColorHelper::Green)));
        AptValue *pBA = AptFloat::Create((cxform->scale.GetValuef(AptColorHelper::Blue)));
        AptValue *pAA = AptFloat::Create((cxform->scale.GetValuef(AptColorHelper::Alpha)));
        AptValue *pRB = AptFloat::Create((cxform->translate.GetValuef(AptColorHelper::Red)));
        AptValue *pGB = AptFloat::Create((cxform->translate.GetValuef(AptColorHelper::Green)));
        AptValue *pBB = AptFloat::Create((cxform->translate.GetValuef(AptColorHelper::Blue)));
        AptValue *pAB = AptFloat::Create((cxform->translate.GetValuef(AptColorHelper::Alpha)));
#else
        AptValue *pRA = AptInteger::Create((cxform->scale.GetValue(AptColorHelper::Red)));
        AptValue *pGA = AptInteger::Create((cxform->scale.GetValue(AptColorHelper::Green)));
        AptValue *pBA = AptInteger::Create((cxform->scale.GetValue(AptColorHelper::Blue)));
        AptValue *pAA = AptInteger::Create((cxform->scale.GetValue(AptColorHelper::Alpha)));
        AptValue *pRB = AptInteger::Create((cxform->translate.GetValue(AptColorHelper::Red)));
        AptValue *pGB = AptInteger::Create((cxform->translate.GetValue(AptColorHelper::Green)));
        AptValue *pBB = AptInteger::Create((cxform->translate.GetValue(AptColorHelper::Blue)));
        AptValue *pAB = AptInteger::Create((cxform->translate.GetValue(AptColorHelper::Alpha)));
#endif
        pObject->Set(StringPool::GetString(SC_ra), pRA);
        pObject->Set(StringPool::GetString(SC_ga), pGA);
        pObject->Set(StringPool::GetString(SC_ba), pBA);
        pObject->Set(StringPool::GetString(SC_aa), pAA);
        pObject->Set(StringPool::GetString(SC_rb), pRB);
        pObject->Set(StringPool::GetString(SC_gb), pGB);
        pObject->Set(StringPool::GetString(SC_bb), pBB);
        pObject->Set(StringPool::GetString(SC_ab), pAB);

        return pObject;
    }
    return gpUndefinedValue;
}

NATIVE_MEMBER_FUNCTION(AptScriptColour, setTransform)
{
    if (nParams <= 0)
        return gpUndefinedValue;

    AptValue *pParam = gAptActionInterpreter.stackAt(0);
    // Flash admits undefined parameters here.
    if (pParam->isUndefined())
    {
        return gpUndefinedValue;
    }
    AptScriptColour *pTemp = pThis->c_scriptcolour();
    APT_ASSERT(pTemp);
    AptCIH *pSprite = pTemp->pSprite;
    APT_ASSERT(pSprite);

    if (pSprite == NULL || pSprite->isUndefined())
        return gpUndefinedValue;

    if (pParam->getIsDefined() && pParam->isObject())
    {
        AptCharacterInst *pCharInst = pSprite->GetCharacterInst();
        AptObject *pObject          = pParam->c_object();

        AptCXForm *cxform = pCharInst->GetColorMatrixWritable();

        AptValue *pRA = pObject->Lookup(StringPool::GetString(SC_ra));
        if (pRA != NULL)
        {
#if defined(APT_USE_FLASH_COLOR_RANGE)
            cxform->scale.SetValuef(AptColorHelper::Red, pRA->toFloat());
#else
            cxform->scale.SetValue(AptColorHelper::Red, pRA->toInteger());
#endif
        }

        AptValue *pRB = pObject->Lookup(StringPool::GetString(SC_rb));
        if (pRB != NULL)
        {
#if defined(APT_USE_FLASH_COLOR_RANGE)
            cxform->translate.SetValuef(AptColorHelper::Red, pRB->toFloat());
#else
            cxform->translate.SetValue(AptColorHelper::Red, pRB->toInteger());
#endif
        }

        AptValue *pGA = pObject->Lookup(StringPool::GetString(SC_ga));
        if (pGA != NULL)
        {
#if defined(APT_USE_FLASH_COLOR_RANGE)
            cxform->scale.SetValuef(AptColorHelper::Green, pGA->toFloat());
#else
            cxform->scale.SetValue(AptColorHelper::Green, pGA->toInteger());
#endif
        }

        AptValue *pGB = pObject->Lookup(StringPool::GetString(SC_gb));
        if (pGB != NULL)
        {
#if defined(APT_USE_FLASH_COLOR_RANGE)
            cxform->translate.SetValuef(AptColorHelper::Green, pGB->toFloat());
#else
            cxform->translate.SetValue(AptColorHelper::Green, pGB->toInteger());
#endif
        }

        AptValue *pBA = pObject->Lookup(StringPool::GetString(SC_ba));
        if (pBA != NULL)
        {
#if defined(APT_USE_FLASH_COLOR_RANGE)
            cxform->scale.SetValuef(AptColorHelper::Blue, pBA->toFloat());
#else
            cxform->scale.SetValue(AptColorHelper::Blue, pBA->toInteger());
#endif
        }

        AptValue *pBB = pObject->Lookup(StringPool::GetString(SC_bb));
        if (pBB != NULL)
        {
#if defined(APT_USE_FLASH_COLOR_RANGE)
            cxform->translate.SetValuef(AptColorHelper::Blue, pBB->toFloat());
#else
            cxform->translate.SetValue(AptColorHelper::Blue, pBB->toInteger());
#endif
        }

        AptValue *pAA = pObject->Lookup(StringPool::GetString(SC_aa));
        if (pAA != NULL)
        {
#if defined(APT_USE_FLASH_COLOR_RANGE)
            cxform->scale.SetValuef(AptColorHelper::Alpha, pAA->toFloat());
#else
            cxform->scale.SetValue(AptColorHelper::Alpha, pAA->toInteger());
#endif
        }

        AptValue *pAB = pObject->Lookup(StringPool::GetString(SC_ab));
        if (pAB != NULL)
        {
#if defined(APT_USE_FLASH_COLOR_RANGE)
            cxform->translate.SetValuef(AptColorHelper::Alpha, pAB->toFloat());
#else
            cxform->translate.SetValue(AptColorHelper::Alpha, pAB->toInteger());
#endif
        }

        if (pRA || pRB || pGA || pGB || pBA || pBB || pAA || pAB)
            pSprite->SetASChanged(true);
    }

    return gpUndefinedValue;
}

void AptScriptColour::RegisterReferences()
{
    if (APT_REFERENCES_REGISTERED(this))
        return;

    AptObject::RegisterReferences();

    APT_REGISTER_REFERENCE_SAFE(pSprite, "CIH", APT_REFREG_IS_APTCIH);
    return;
}

void AptScriptColour::DestroyGCPointers()
{
    APT_DECSAFE(pSprite);
    pSprite = NULL;
    AptObject::DestroyGCPointers();
}
#endif // APT_USE_SCRIPTCOLOUR_OBJECT
