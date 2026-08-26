#include "_Apt.h"
#include "AptStd/AptCXForm.h"
#include "MainInline.h"

AptColorHelper::AptColorHelper()
{
#if defined(APT_USE_FLASH_COLOR_RANGE)
    mfVal[0] = 0.f;
    mfVal[1] = 0.f;
    mfVal[2] = 0.f;
    mfVal[3] = 0.f;
#else
    mnVal = 0x00000000;
#endif
}

void AptColorHelper::CopyFromFloatArray(const float *paCXForm)
{
#if defined(APT_USE_FLASH_COLOR_RANGE)
    memcpy(mfVal, paCXForm, sizeof(float) * 4);
#else
    uint32_t nTmp = (uint32_t)(paCXForm[0] * 255.f);
    mnVal         = (mnVal & 0x00ffffff) | (nTmp << 24);
    nTmp          = (uint32_t)(paCXForm[1] * 255.f);
    mnVal         = (mnVal & 0xff00ffff) | (nTmp << 16);
    nTmp          = (uint32_t)(paCXForm[2] * 255.f);
    mnVal         = (mnVal & 0xffff00ff) | (nTmp << 8);
    nTmp          = (uint32_t)(paCXForm[3] * 255.f);
    mnVal         = (mnVal & 0xffffff00) | (nTmp << 0);
#endif
}

/** Copies the 32-bit color into a 4-float array (assumed to have room for 4 floats). */
void AptColorHelper::CopyToFloatArray4(float *fColorArray) const
{
#if defined(APT_USE_FLASH_COLOR_RANGE)
    memcpy(fColorArray, mfVal, sizeof(float) * 4);
#else
    fColorArray[0] = GetValuef(Alpha);
    fColorArray[1] = GetValuef(Red);
    fColorArray[2] = GetValuef(Green);
    fColorArray[3] = GetValuef(Blue);
#endif
}

/**
 * @return an ARGB channel value in 0.0-1.0 (A=0, R=1, G=2, B=3); -1..1 if built with
 * APT_USE_FLASH_COLOR_RANGE.
 */
float AptColorHelper::GetValuef(AptColorValue eColor) const
{
#if defined(APT_USE_FLASH_COLOR_RANGE)
    int32_t eColorTest = (int32_t)eColor;
    if (eColorTest >= 0 && eColorTest < 4)
    {
        return mfVal[(int32_t)eColor];
    }

    return 0.f;
#else
    return GetValue(eColor) / 255.f;
#endif
}

/**
 * @return an ARGB channel value in 0-255 (A=0, R=1, G=2, B=3); -255..255 if built with
 * APT_USE_FLASH_COLOR_RANGE. eColor << 3 fast-multiplies by 8 to pick the nth byte out of mnVal.
 */
int32_t AptColorHelper::GetValue(AptColorValue eColor) const
{
#if defined(APT_USE_FLASH_COLOR_RANGE)
    int32_t eColorTest = (int32_t)eColor;
    if (eColorTest >= 0 && eColorTest < 4)
    {
        return (uint32_t)(mfVal[(int32_t)eColor] * 255.f);
    }

    return 0;
#else
    return (mnVal & (0xff000000u >> ((int32_t)eColor << 3))) >> ((3 - (int32_t)eColor) << 3);
#endif
}

AptCXForm::AptCXForm(AptCXForm *pCXForm)
{
    AptCXFormCopy(pCXForm);
}

AptCXForm::AptCXForm(AptFloatArrayCXForm *pFloatCXForm)
{
    AptFloatArrayCXFormCopy(pFloatCXForm);
}

AptCXForm::AptCXForm(AptUint32CXForm *pUintCXForm)
{
    AptUint32CXFormCopy(pUintCXForm);
}

void AptCXForm::AptCXFormCopy(const AptCXForm *pCXForm)
{
    if (pCXForm != NULL)
    {
        memcpy(this, pCXForm, sizeof(AptCXForm));
    }
}

void AptCXForm::AptFloatArrayCXFormCopy(const AptFloatArrayCXForm *pCXForm)
{
    if (pCXForm != NULL)
    {
        scale.CopyFromFloatArray(pCXForm->scale);
        translate.CopyFromFloatArray(pCXForm->translate);
    }
}

void AptCXForm::AptUint32CXFormCopy(const AptUint32CXForm *pCXForm)
{
    if (pCXForm != NULL)
    {
#if defined(APT_USE_FLASH_COLOR_RANGE)
        uint32_t uScale = pCXForm->nScale;
        uint32_t uBias  = pCXForm->nBias;
        scale.SetValuef(AptColorHelper::Alpha, (((uScale >> 24) & 0xFF) / 254.f) * AptColorHelperScale::SCALE_FACTOR);
        scale.SetValuef(AptColorHelper::Red, (((uScale >> 16) & 0xFF) / 254.f) * AptColorHelperScale::SCALE_FACTOR);
        scale.SetValuef(AptColorHelper::Green, (((uScale >> 8) & 0xFF) / 254.f) * AptColorHelperScale::SCALE_FACTOR);
        scale.SetValuef(AptColorHelper::Blue, (((uScale >> 0) & 0xFF) / 254.f) * AptColorHelperScale::SCALE_FACTOR);

        translate.SetValuef(AptColorHelper::Alpha, (uBias >> 24) & 0xFF);
        translate.SetValuef(AptColorHelper::Red, (uBias >> 16) & 0xFF);
        translate.SetValuef(AptColorHelper::Green, (uBias >> 8) & 0xFF);
        translate.SetValuef(AptColorHelper::Blue, (uBias >> 0) & 0xFF);
#else
        scale.CopyFromUint32ARGB(&pCXForm->nScale);
        translate.CopyFromUint32ARGB(&pCXForm->nBias);
#endif
    }
}
