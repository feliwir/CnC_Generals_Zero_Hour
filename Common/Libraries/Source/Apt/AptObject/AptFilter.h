/**
    Defines low-level data structures for the flash.filters.* classes.
*/

#pragma once

class AptObject;
class AptValue;
class AptNativeHash;

/** @brief AptFilter is a base class for memory-mapped structures in AptFilter.cpp. */
struct AptFilter
{
    // Corresponds to Flash filter type.
    // 0 = DropShadowFilter
    // 1 = BlurFilter
    // 2 = GlowFilter
    // 3 = BevelFilter
    // 4 = GradientGlowFilter
    // 5 = ConvolutionFilter
    // 6 = ColorMatrixFilter
    // 7 = GradientBevelFilter
    uint32_t mnFilterID;

    /** @brief Certain filter structs contain pointers to other data, so it is necessary to resolve/unresolve them. */
    static void Resolve(AptFilter *pFilter, uint8_t *pBase);
    static void Unresolve(AptFilter *pFilter, uint8_t *pBase);

    /** @brief Returns a new filter script object based on the memory-mapped data from a PlaceObject3 tag. */
    static AptObject *CreateFilterObject(const AptFilter *pFilter, AptValue *pItem);
};

/** @brief Drop Shadow Filter */
struct AptFilterDropShadow : public AptFilter
{
    // memory-mapped data from PlaceObject3 tag
    uint32_t mnARGB;
    uint32_t mnBlurX;
    uint32_t mnBlurY;
    uint32_t mnAngle;
    uint32_t mnDistance;
    uint16_t mnStrength;
    uint16_t mnFlags;

    // Takes memory-mapped data and creates entries in a script object's hash table.
    static void SetProperties(const AptFilter *pFilter, AptNativeHash *pHash);
};

/** @brief Blur Filter */
struct AptFilterBlur : public AptFilter
{
    // memory-mapped data from PlaceObject3 tag
    uint32_t mnBlurX;
    uint32_t mnBlurY;
    uint16_t mnFlags;

    // Takes memory-mapped data and creates entries in a script object's hash table.
    static void SetProperties(const AptFilter *pFilter, AptNativeHash *pHash);
};

/** @brief Glow Filter */
struct AptFilterGlow : public AptFilter
{
    uint32_t mnARGB;
    uint32_t mnBlurX;
    uint32_t mnBlurY;
    uint16_t mnStrength;
    uint16_t mnFlags;

    static void SetProperties(const AptFilter *pFilter, AptNativeHash *pHash);
};

/** @brief Bevel Filter */
struct AptFilterBevel : public AptFilter
{
    uint32_t mnARGBHighlight;
    uint32_t mnARGBShadow;
    uint32_t mnBlurX;
    uint32_t mnBlurY;
    uint32_t mnAngle;
    uint32_t mnDistance;
    uint16_t mnStrength;
    uint16_t mnFlags;

    static void SetProperties(const AptFilter *pFilter, AptNativeHash *pHash);
};

/**
 * @brief Gradient Glow Filter.
 * @note the exact same data structure is used for the Gradient Bevel Filter.
 */
struct AptFilterGradientGlow : public AptFilter
{
    // memory-mapped data from PlaceObject3 tag.
    uint32_t mnNumColors;
    uint32_t *mpGradColors; // mpGradColors and mpGradRatios must be resolved
    uint8_t *mpGradRatios;  //  before they can be read from.
    uint32_t mnBlurX;
    uint32_t mnBlurY;
    uint32_t mnAngle;
    uint32_t mnDistance;
    uint16_t mnStrength;
    uint16_t mnFlags;

    static void SetProperties(const AptFilter *pFilter, AptNativeHash *pHash);

    /** @brief Resolves the two arrays in the memory-mapped data */
    static void Resolve(AptFilter *pFilter, uint8_t *pBase);

    /** @brief Unresolves the two arrays in the memory-mapped data */
    static void Unresolve(AptFilter *pFilter, uint8_t *pBase);
};

/** @brief Color Matrix Filter (a.k.a. the Adjust Color Filter) */
struct AptFilterColorMatrix : public AptFilter
{
    float mafColorValues[20];

    static void SetProperties(const AptFilter *pFilter, AptNativeHash *pHash);
};
