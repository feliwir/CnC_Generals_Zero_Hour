#pragma once

#include "AptDefine.h"

// forward declarations
struct AptFloatArrayCXForm;
struct AptUint32CXForm;

/**
 * @brief AptColorHelper is an abstract base class.  This holds the data for
 * an ARGB color.
 */
struct AptColorHelper
{
    enum AptColorValue
    {
        Alpha = 0,
        Red   = 1,
        Green = 2,
        Blue  = 3,
        InvalidColor
    };

    /** @brief Constructor */
    explicit AptColorHelper();
    virtual ~AptColorHelper() {}

    /** @brief Copies the float array color */
    void CopyFromFloatArray(const float *paCXForm);

    int32_t GetValue(AptColorValue eColor) const;
    float GetValuef(AptColorValue eColor) const;
    void CopyToFloatArray4(float *fColorArray) const;

  protected:
#if defined(APT_USE_FLASH_COLOR_RANGE)
    float mfVal[4]; // Values are limited by scale_factor
#else
    uint32_t mnVal; // Values range from 0 - 255
#endif
};

/**
 * @brief AptColorHelperTemplate defines some useful operations for working with colors.
 * The primary methods are SetValuef() and GetValuef().  The other methods
 * are included for compatibility with older Aux layers and should be avoided
 * whenever possible.
 */
template <int scale_factor>
struct AptColorHelperTemplate : public AptColorHelper
{
    /** @brief The scale factor defines the limits of the color component values. */
    enum
    {
        SCALE_FACTOR = scale_factor
    };

    virtual ~AptColorHelperTemplate() {}

    /**
     * Set an ARGB color component value.
     * Assumptions are that ABS(fVal) <= scale_factor if APT_USE_FLASH_COLOR_RANGE is defined;
     * otherwise assumes 0 <= fVal <= 255.
     * @param eColor color component to set: A = 0, R = 1, G = 2, B = 3
     * @param fVal color component value
     */
    void SetValuef(AptColorValue eColor, float fVal)
    {
#if defined(APT_USE_FLASH_COLOR_RANGE)
        float fTmp = fVal;
        fTmp       = fTmp < (-scale_factor) ? (-scale_factor) : fTmp;
        fTmp       = fVal > scale_factor ? scale_factor : fTmp;

        int32_t eColorTest = (int32_t)eColor;
        if (eColorTest >= 0 && eColorTest < InvalidColor)
        {
            mfVal[(int32_t)eColor] = fTmp;
        }
#else
        SetValue(eColor, (int32_t)fVal);
#endif
    }

    /**
     * Returns a ARGB color component value
     * @param eColor color component to get: A = 0, R = 1, G = 2, B = 3
     * @return float             - if APT_USE_FLASH_COLOR_RANGE is defined, ABS(return_value) <= scale_factor; otherwise 0 <= return_value <= 255.
     */
    float GetValuef(AptColorValue eColor) const
    {
#if defined(APT_USE_FLASH_COLOR_RANGE)

        int32_t eColorTest = (int32_t)eColor;
        if (eColorTest >= 0 && eColorTest < InvalidColor)
        {
            return mfVal[(int32_t)eColor];
        }
        return 0.f;
#else
        return (float)GetValue(eColor);
#endif
    }

    /** @brief Copy function */
    void Copy(const AptColorHelperTemplate<scale_factor> *pNewColor)
    {
#if defined(APT_USE_FLASH_COLOR_RANGE)
        memcpy(mfVal, pNewColor->mfVal, sizeof(float) * 4);
#else
        mnVal = pNewColor->mnVal;
#endif
    }

#if !defined(APT_USE_FLASH_COLOR_RANGE)

    /**
     * Returns a ARGB value from 0-255 where A = 0, R = 1, G = 2, B = 3
     * @param eColor color wanted
     * @return float             - color
     */
    int32_t GetValue(AptColorValue eColor) const
    {
        return (mnVal & (0xff000000u >> ((int32_t)eColor << 3))) >> ((3 - (int32_t)eColor) << 3);
    }

    /**
     * Sets the color to the passed 32 bit ARGB int
     * @param nVal Output
     */
    void SetValue(uint32_t nVal)
    {
        mnVal = nVal;
    }

    /**
     * Set the ARGB value where A = 0, R = 1, G = 2, B = 3 to the passed in color.  Assumptions are that nVal is 0 to 255.
     * @param eColor Input      nVal              -
     */
    void SetValue(AptColorValue eColor, int32_t nVal)
    {
        nVal  = nVal < 0 ? 0 : nVal;
        nVal  = nVal > 255 ? 255 : nVal;
        mnVal = (mnVal & (~(0xff000000u >> (eColor << 3)))) | (nVal << ((3 - eColor) << 3));
    }

    /**
     * Copies the 32bit color to another 32bit color
     * @param nColor Output
     */
    void CopyToUint32ARGB(uint32_t *nColor) const
    {
        *nColor = mnVal;
    }

    /**
     * Copies the 32bit color to another 32bit color
     * @param nColor Output
     */
    void CopyToUint32RGBA(uint32_t *nColor) const
    {
        *nColor = ((mnVal & 0x00ffffff) << 8) | (mnVal & 0xff000000) >> 24;
    }

    /** @brief Copies the 32bit color to another 32bit color in ARGB form */
    void CopyFromUint32ARGB(const uint32_t *nColor)
    {
        mnVal = *nColor;
    }

    /** @brief Getters for the ARGB values of the AptColorHelper object */
    int32_t GetA() const
    {
        return (int32_t)((mnVal & 0xff000000) >> 24);
    }

    int32_t GetR() const
    {
        return (int32_t)((mnVal & 0x00ff0000) >> 16);
    }

    int32_t GetG() const
    {
        return (int32_t)((mnVal & 0x0000ff00) >> 8);
    }

    int32_t GetB() const
    {
        return (int32_t)((mnVal & 0x000000ff));
    }

    /**
     * returns the 32bit color member for VMX optimizations
     * @param None .
     * @return uint32_t          -
     */
    uint32_t *GetUint32ARGBMember()
    {
        return &mnVal;
    }

    /**
     * returns the 32bit color
     * @param None .
     * @return uint32_t          -
     */
    uint32_t GetUint32RGBA() const
    {
        return mnVal;
    }

    /**
     * returns the 32bit color
     * @param None .
     * @return uint32_t          -
     */
    uint32_t GetUint32ARGB() const
    {
        return mnVal;
    }

    // these are here to keep backward compatibility.
    float GetAf() const
    {
        return (float)((mnVal & 0xff000000) >> 24) / 255.f;
    }

    float GetRf() const
    {
        return (float)((mnVal & 0x00ff0000) >> 16) / 255.f;
    }

    float GetGf() const
    {
        return (float)((mnVal & 0x0000ff00) >> 8) / 255.f;
    }

    float GetBf() const
    {
        return (float)((mnVal & 0x000000ff)) / 255.f;
    }

#endif // APT_USE_FLASH_COLOR_RANGE
};

/**
 * @brief AptColorHelperScale represents a color "scale" value that
 * is multiplied by another color during a transform operation.
 */
struct AptColorHelperScale
#if defined(APT_USE_FLASH_COLOR_RANGE)
    : public AptColorHelperTemplate<100>
#else
    : public AptColorHelperTemplate<255>
#endif // APT_USE_FLASH_COLOR_RANGE
{
};

/**
 * @brief AptColorHelperTranslate represents a color "translate" value that
 * is added to another color during a transform operation.
 */
struct AptColorHelperTranslate : public AptColorHelperTemplate<255>
{
};

/**
 * @brief AptCXForm encapsulates the scale and translate values for a color transformation.
 */
struct AptCXForm
{
    /** @brief Constructors */
    AptCXForm() {} // Do Nothing
    AptCXForm(AptCXForm *pCXForm);
    AptCXForm(AptFloatArrayCXForm *pFloatCXForm);
    AptCXForm(AptUint32CXForm *pUintCXForm);

    /** @brief Copy functions */
    void AptCXFormCopy(const AptCXForm *pCXForm);
    void AptFloatArrayCXFormCopy(const AptFloatArrayCXForm *pCXForm);
    void AptUint32CXFormCopy(const AptUint32CXForm *pCXForm);

    AptColorHelperScale scale;
    AptColorHelperTranslate translate;

    APT_NEW_DELETE_OPERATORS
};
