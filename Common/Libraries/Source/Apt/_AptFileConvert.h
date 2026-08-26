#pragma once

/**
 * Pointer-size-explicit mirrors of every structure that swfc serializes into .apt
 * and .const files.
 *
 * The file format is a direct memory image: pointer members are stored as
 * file-relative offsets (or index/magic values) in slots whose width matches the
 * pointer size of the tool that built the file. A file built by a 32-bit swfc
 * therefore has different struct sizes AND member offsets than the native structs
 * of a 64-bit process.
 *
 * Each AptXxxT<PTR_T> below mirrors the corresponding native struct with all
 * pointer-sized members replaced by AptSizedPtr<PTR_T, ...>. The <uint32_t>
 * instantiations describe the 32-bit file layout and are what the converter in
 * AptFileConvert.cpp reads. The <uintptr_t> instantiations must be layout-identical
 * to the native structs; APT_CONVERT_CHECK_* static_asserts enforce this so the
 * mirrors cannot silently drift when the native structs (or the feature defines
 * that shape them) change.
 */

#include "_Apt.h"
#include "AptCharacter/AptCharacter.h"
#include "AptObject/AptFilter.h"

/**
 * A file-relative pointer slot of explicit width. Carries an offset, character
 * index, or magic value - never a live pointer, so it is never dereferenced.
 */
template <typename PTR_T, typename POINTEE>
struct AptSizedPtr
{
    PTR_T value;

    APT_INLINE bool IsNull() const { return value == 0; }
    APT_INLINE uint64_t Offset() const { return static_cast<uint64_t>(value); }
};

// ---------------------------------------------------------------------------
// _AptActions.h mirrors
// ---------------------------------------------------------------------------

template <typename PTR_T>
struct AptConstantPoolT
{
    int nItems;
    AptSizedPtr<PTR_T, AptValue *> apItems;
};

template <typename PTR_T>
struct AptAction_GetUrlT
{
    AptSizedPtr<PTR_T, char> szUrl;
    AptSizedPtr<PTR_T, char> szWin;
};

template <typename PTR_T>
struct AptAction_SetTargetT
{
    AptSizedPtr<PTR_T, char> szTarget;
};

template <typename PTR_T>
struct AptAction_GotoLabelT
{
    AptSizedPtr<PTR_T, char> szLabel;
};

template <typename PTR_T>
struct AptAction_PushStringT
{
    AptSizedPtr<PTR_T, char> szStringToBePushed;
};

template <typename PTR_T>
struct AptAction_PushT
{
    AptConstantPoolT<PTR_T> items;
};

template <typename PTR_T>
struct AptAction_WithT
{
    AptSizedPtr<PTR_T, unsigned char> pEnd;
};

template <typename PTR_T>
struct AptAction_DefineFunctionT
{
    AptSizedPtr<PTR_T, const char> szName;
    int nParams;
    AptSizedPtr<PTR_T, char *> aszParams;
    int nCodeSize;
    AptConstantPoolT<PTR_T> constantPool;
};

template <typename PTR_T>
struct AptRegisterParamT
{
    uint32_t nRegister;
    AptSizedPtr<PTR_T, char> szParamName;
};

template <typename PTR_T>
struct AptAction_DefineFunction2T
{
    AptSizedPtr<PTR_T, const char> szName;
    int nParams;
    short nRegisterCount;
    short nFlags;
    AptSizedPtr<PTR_T, AptRegisterParam> aszParams;
    int nCodeSize;
    AptConstantPoolT<PTR_T> constantPool;
};

template <typename PTR_T>
struct AptAction_TryCatchFinallyBlockT
{
    uint32_t uTryCodeSize;
    uint32_t uCatchCodeSize;
    uint32_t uFinallyCodeSize;
    uint8_t uFlags;
    uint8_t uAlignment1;
    uint8_t uAlignment2;
    uint8_t uCaughtRegister;
    AptSizedPtr<PTR_T, char> szCaughtVarName;

    APT_INLINE bool putCaughtObjectInRegister() const
    {
        return (uFlags & TCFB_PutCaughtObjectInRegister) != 0;
    }
};

template <typename PTR_T>
struct AptActionBlockT
{
    AptSizedPtr<PTR_T, unsigned char> aActionStream;
};

template <typename PTR_T>
struct AptEventActionBlockT
{
    int nTriggers;
    int nKeyCode;
    AptActionBlockT<PTR_T> actions;
};

template <typename PTR_T>
struct AptEventActionSetT
{
    int nEventActions;
    AptSizedPtr<PTR_T, AptEventActionBlock> aEventActions;
};

template <typename PTR_T>
struct AptActionConditionBlockT
{
    int nConditions;
    AptActionBlockT<PTR_T> actions;
};

// ---------------------------------------------------------------------------
// _Apt.h control/frame mirrors
// ---------------------------------------------------------------------------

template <typename PTR_T>
struct AptControlDoActionT
{
    AptActionBlockT<PTR_T> actions;
};

template <typename PTR_T>
struct AptControlDoInitActionT
{
    int nSpriteID;
    AptActionBlockT<PTR_T> actions;
};

template <typename PTR_T>
struct AptControlFrameLabelT
{
    AptSizedPtr<PTR_T, char> szLabel;
};

template <typename PTR_T>
struct AptControlPlaceObject2T
{
    AptPlaceObjectFlags eFlags;
    int nDepth;
    int nCharacterID;
    AptMatrix matrix;
    AptUint32CXForm ncxform;
    float fRatio;
    AptSizedPtr<PTR_T, char> szName;
    int nClipDepth;
    AptSizedPtr<PTR_T, AptEventActionSet> pActions;
};

template <typename PTR_T>
struct AptControlPlaceObject3T
{
    AptPlaceObjectFlags eFlags;
    int nDepth;
    int nCharacterID;
    AptMatrix matrix;
    AptUint32CXForm ncxform;
    float fRatio;
    AptSizedPtr<PTR_T, char> szName;
    int nClipDepth;
    AptSizedPtr<PTR_T, AptEventActionSet> pActions;
    int32_t nBlendMode;
    uint32_t nNumFilters;
    AptSizedPtr<PTR_T, AptFilter *> ppFilters;
};

template <typename PTR_T>
struct AptControlT
{
    AptControlType eType;
    union
    {
        AptControlDoActionT<PTR_T> action;
        AptControlDoInitActionT<PTR_T> initAction;
        AptControlFrameLabelT<PTR_T> frameLabel;
        AptControlPlaceObject2T<PTR_T> placeObject2;
        AptControlPlaceObject3T<PTR_T> placeObject3;
        AptControlRemoveObject2 removeObject2;
        AptControlBackgroundColour backgroundColour;
#if defined(APT_USE_SOUND_OBJECT)
        AptControlSound startSound;
        AptControlSound startSoundStream;
#endif
    };
};

template <typename PTR_T>
struct AptFrameT
{
    int nControls;
    AptSizedPtr<PTR_T, AptControl *> apControls;
};

template <typename PTR_T>
struct AptCharacterButtonRecordT
{
#if defined APT_USE_BUTTONS
    AptCharacterButtonRecordState eStates;
    AptSizedPtr<PTR_T, AptCharacter> pCharacter;
    int nLayer;
    AptMatrix matrix;
    AptFloatArrayCXForm cxform;
#endif
};

template <typename PTR_T>
struct AptCharacterButtonSoundT
{
#if defined APT_USE_BUTTONS
    AptSizedPtr<PTR_T, AptCharacter> pOverUpToIdle;
    AptSizedPtr<PTR_T, AptCharacter> pIdleToOverUp;
    AptSizedPtr<PTR_T, AptCharacter> pOverUpToOverDown;
    AptSizedPtr<PTR_T, AptCharacter> pOverDownToOverUp;
#endif
};

template <typename PTR_T>
struct AptCharacterStaticTextRecordsT
{
    int nFontID;
    AptFloatArrayCXForm cxform;
    float fXOffset;
    float fYOffset;
    float fScale;
    int nGlyphs;
    AptSizedPtr<PTR_T, AptCharacterGlyphEntry> aGlyphs;
};

template <typename PTR_T>
struct AptImportT
{
    AptSizedPtr<PTR_T, char> szFile;
    AptSizedPtr<PTR_T, char> szName;
    int nID;
    AptSizedPtr<PTR_T, AptFile> file;
};

template <typename PTR_T>
struct AptExportT
{
    AptSizedPtr<PTR_T, char> szName;
    int nID;
};

// ---------------------------------------------------------------------------
// AptCharacter.h / AptMovie.h mirrors
// ---------------------------------------------------------------------------

template <typename PTR_T>
struct AptMovieT
{
    int nFrames;
    AptSizedPtr<PTR_T, AptFrame> aFrames;
    AptSizedPtr<PTR_T, AptNativeHash> phLabels;
};

template <typename PTR_T>
struct AptCharacterShapeT
{
    AptRect rBounds;
    AptSizedPtr<PTR_T, AptRenderableGeometry> pRenderUnit;
};

template <typename PTR_T>
struct AptCharacterMorphT
{
    AptSizedPtr<PTR_T, AptCharacter> pStartCharacter;
    AptSizedPtr<PTR_T, AptCharacter> pEndCharacter;
};

template <typename PTR_T>
struct AptCharacterTextT
{
    AptRect rBounds;
    int nFontID;
    AptStringAlignment eAlignment;
    unsigned int nColour;
    float fFontHeight;
    int bReadOnly;
    int bMultiLine;
    int bWordWrap;
    AptSizedPtr<PTR_T, char> szInitialText;
    AptSizedPtr<PTR_T, char> szVariable;
};

template <typename PTR_T>
struct AptCharacterFontT
{
    AptSizedPtr<PTR_T, char> szName;
    int nGlyphs;
    AptSizedPtr<PTR_T, AptCharacter *> apGlyphs;
};

template <typename PTR_T>
struct AptCharacterButtonT
{
#if defined APT_USE_BUTTONS
    int bIsMenu;
    AptRect mHitTestBoundingRect;
    int mHitTestTriangles;
    int mHitTestVertexCount;
    AptSizedPtr<PTR_T, float> mHitTestVertexTable;
    AptSizedPtr<PTR_T, short> mHitTestIndexTable;
    int nButtonRecords;
    AptSizedPtr<PTR_T, AptCharacterButtonRecord> aButtonRecords;
    int nActionConditions;
    AptSizedPtr<PTR_T, AptActionConditionBlock> aActionConditions;
    AptSizedPtr<PTR_T, AptCharacterButtonSound> pButtonSound;
#endif
};

template <typename PTR_T>
struct AptCharacterSpriteT
{
    AptMovieT<PTR_T> movie;
};

template <typename PTR_T>
struct AptCharacterSoundT
{
#if defined(APT_USE_SOUND_OBJECT)
    AptSizedPtr<PTR_T, void> zID;
#endif
};

template <typename PTR_T>
struct AptCharacterBitmapT
{
    AptSizedPtr<PTR_T, void> zID;
};

template <typename PTR_T>
struct AptCharacterImageT
{
    AptRect bounds;
    AptSizedPtr<PTR_T, void> texture;
};

template <typename PTR_T>
struct AptCharacterAnimationT : public AptCharacterSpriteT<PTR_T>
{
    int nCharacters;
    AptSizedPtr<PTR_T, AptCharacter *> apCharacters;

    unsigned int nWidth;
    unsigned int nHeight;
    unsigned int nMillisecondsPerFrame;

    int nImports;
    AptSizedPtr<PTR_T, AptImport> aImports;

    int nExports;
    AptSizedPtr<PTR_T, AptExport> aExports;

    // intptr_t in the native struct, so its width follows the pointer size
    AptSizedPtr<PTR_T, void> nCurrentConstantIndex;
};

template <typename PTR_T>
struct AptCharacterStaticTextT
{
    AptRect rBounds;
    AptMatrix matrix;
    int nFontRecords;
    AptSizedPtr<PTR_T, AptCharacterStaticTextRecords> aRecords;
};

template <typename PTR_T>
struct AptCharacterT
{
    AptCharacterType eType;
    AptSizedPtr<PTR_T, AptCharacter> pParentAnim;

#if defined APT_DECOUPLED_RENDERING
#error "The 32/64-bit file converter does not mirror the decoupled-rendering character layout"
#endif
    union
    {
        AptCharacterShapeT<PTR_T> shape;
        AptCharacterMorphT<PTR_T> morph;
        AptCharacterTextT<PTR_T> text;
        AptCharacterFontT<PTR_T> font;
#if defined(APT_USE_BUTTONS)
        AptCharacterButtonT<PTR_T> button;
#endif
        AptCharacterSpriteT<PTR_T> sprite;
#if defined(APT_USE_SOUND_OBJECT)
        AptCharacterSoundT<PTR_T> sound;
#endif
        AptCharacterBitmapT<PTR_T> bitmap;
        AptCharacterAnimationT<PTR_T> animation;
        AptCharacterStaticTextT<PTR_T> statictext;
        AptCharacterImageT<PTR_T> image;
    };
};

// ---------------------------------------------------------------------------
// AptFilter.h mirror (only the gradient filters contain pointers)
// ---------------------------------------------------------------------------

template <typename PTR_T>
struct AptFilterGradientGlowT
{
    uint32_t mnFilterID;
    uint32_t mnNumColors;
    AptSizedPtr<PTR_T, uint32_t> mpGradColors;
    AptSizedPtr<PTR_T, uint8_t> mpGradRatios;
    uint32_t mnBlurX;
    uint32_t mnBlurY;
    uint32_t mnAngle;
    uint32_t mnDistance;
    uint16_t mnStrength;
    uint16_t mnFlags;
};

// ---------------------------------------------------------------------------
// _AptFile.h mirror (AptConstFileT lives in _AptFile.h)
// ---------------------------------------------------------------------------

template <typename PTR_T>
struct AptConstantTableT
{
    AptVirtualFunctionTable_Indices eType;
    AptSizedPtr<PTR_T, char> uValue; // union of szString/fFloat/nInteger/... - copied by value
};

// ---------------------------------------------------------------------------
// Layout-parity checks: the <uintptr_t> instantiation of every mirror must match
// the native struct exactly.
// ---------------------------------------------------------------------------

#define APT_CONVERT_CHECK_SIZE(_mirror_, _native_)                 \
    static_assert(sizeof(_mirror_<uintptr_t>) == sizeof(_native_), \
                  "converter mirror out of sync with " #_native_)
#define APT_CONVERT_CHECK_MEMBER(_mirror_, _native_, _member_)                             \
    static_assert(offsetof(_mirror_<uintptr_t>, _member_) == offsetof(_native_, _member_), \
                  "converter mirror out of sync with " #_native_ "::" #_member_)

APT_CONVERT_CHECK_SIZE(AptConstantPoolT, AptConstantPool);
APT_CONVERT_CHECK_MEMBER(AptConstantPoolT, AptConstantPool, apItems);
APT_CONVERT_CHECK_SIZE(AptAction_GetUrlT, AptAction_GetUrl);
APT_CONVERT_CHECK_MEMBER(AptAction_GetUrlT, AptAction_GetUrl, szWin);
APT_CONVERT_CHECK_SIZE(AptAction_SetTargetT, AptAction_SetTarget);
APT_CONVERT_CHECK_SIZE(AptAction_GotoLabelT, AptAction_GotoLabel);
APT_CONVERT_CHECK_SIZE(AptAction_PushStringT, AptAction_PushString);
APT_CONVERT_CHECK_SIZE(AptAction_PushT, AptAction_Push);
APT_CONVERT_CHECK_SIZE(AptAction_WithT, AptAction_With);
APT_CONVERT_CHECK_SIZE(AptAction_DefineFunctionT, AptAction_DefineFunction);
APT_CONVERT_CHECK_MEMBER(AptAction_DefineFunctionT, AptAction_DefineFunction, aszParams);
APT_CONVERT_CHECK_MEMBER(AptAction_DefineFunctionT, AptAction_DefineFunction, nCodeSize);
APT_CONVERT_CHECK_MEMBER(AptAction_DefineFunctionT, AptAction_DefineFunction, constantPool);
APT_CONVERT_CHECK_SIZE(AptRegisterParamT, AptRegisterParam);
APT_CONVERT_CHECK_MEMBER(AptRegisterParamT, AptRegisterParam, szParamName);
APT_CONVERT_CHECK_SIZE(AptAction_DefineFunction2T, AptAction_DefineFunction2);
APT_CONVERT_CHECK_MEMBER(AptAction_DefineFunction2T, AptAction_DefineFunction2, aszParams);
APT_CONVERT_CHECK_MEMBER(AptAction_DefineFunction2T, AptAction_DefineFunction2, nCodeSize);
APT_CONVERT_CHECK_MEMBER(AptAction_DefineFunction2T, AptAction_DefineFunction2, constantPool);
APT_CONVERT_CHECK_SIZE(AptAction_TryCatchFinallyBlockT, AptAction_TryCatchFinallyBlock);
APT_CONVERT_CHECK_MEMBER(AptAction_TryCatchFinallyBlockT, AptAction_TryCatchFinallyBlock, uCaughtRegister);
APT_CONVERT_CHECK_MEMBER(AptAction_TryCatchFinallyBlockT, AptAction_TryCatchFinallyBlock, szCaughtVarName);
APT_CONVERT_CHECK_SIZE(AptActionBlockT, AptActionBlock);
APT_CONVERT_CHECK_SIZE(AptEventActionBlockT, AptEventActionBlock);
APT_CONVERT_CHECK_MEMBER(AptEventActionBlockT, AptEventActionBlock, actions);
APT_CONVERT_CHECK_SIZE(AptEventActionSetT, AptEventActionSet);
APT_CONVERT_CHECK_MEMBER(AptEventActionSetT, AptEventActionSet, aEventActions);
APT_CONVERT_CHECK_SIZE(AptActionConditionBlockT, AptActionConditionBlock);
APT_CONVERT_CHECK_MEMBER(AptActionConditionBlockT, AptActionConditionBlock, actions);

APT_CONVERT_CHECK_SIZE(AptControlPlaceObject2T, AptControlPlaceObject2);
APT_CONVERT_CHECK_MEMBER(AptControlPlaceObject2T, AptControlPlaceObject2, szName);
APT_CONVERT_CHECK_MEMBER(AptControlPlaceObject2T, AptControlPlaceObject2, nClipDepth);
APT_CONVERT_CHECK_MEMBER(AptControlPlaceObject2T, AptControlPlaceObject2, pActions);
APT_CONVERT_CHECK_SIZE(AptControlPlaceObject3T, AptControlPlaceObject3);
APT_CONVERT_CHECK_MEMBER(AptControlPlaceObject3T, AptControlPlaceObject3, nNumFilters);
APT_CONVERT_CHECK_MEMBER(AptControlPlaceObject3T, AptControlPlaceObject3, ppFilters);
APT_CONVERT_CHECK_SIZE(AptControlT, AptControl);
APT_CONVERT_CHECK_SIZE(AptFrameT, AptFrame);
APT_CONVERT_CHECK_MEMBER(AptFrameT, AptFrame, apControls);
APT_CONVERT_CHECK_SIZE(AptCharacterButtonRecordT, AptCharacterButtonRecord);
APT_CONVERT_CHECK_MEMBER(AptCharacterButtonRecordT, AptCharacterButtonRecord, cxform);
APT_CONVERT_CHECK_SIZE(AptCharacterButtonSoundT, AptCharacterButtonSound);
APT_CONVERT_CHECK_SIZE(AptCharacterStaticTextRecordsT, AptCharacterStaticTextRecords);
APT_CONVERT_CHECK_MEMBER(AptCharacterStaticTextRecordsT, AptCharacterStaticTextRecords, aGlyphs);
APT_CONVERT_CHECK_SIZE(AptImportT, AptImport);
APT_CONVERT_CHECK_MEMBER(AptImportT, AptImport, nID);
APT_CONVERT_CHECK_MEMBER(AptImportT, AptImport, file);
APT_CONVERT_CHECK_SIZE(AptExportT, AptExport);
APT_CONVERT_CHECK_MEMBER(AptExportT, AptExport, nID);

APT_CONVERT_CHECK_SIZE(AptMovieT, AptMovie);
APT_CONVERT_CHECK_MEMBER(AptMovieT, AptMovie, aFrames);
APT_CONVERT_CHECK_MEMBER(AptMovieT, AptMovie, phLabels);
APT_CONVERT_CHECK_SIZE(AptCharacterShapeT, AptCharacterShape);
APT_CONVERT_CHECK_SIZE(AptCharacterMorphT, AptCharacterMorph);
APT_CONVERT_CHECK_SIZE(AptCharacterTextT, AptCharacterText);
APT_CONVERT_CHECK_MEMBER(AptCharacterTextT, AptCharacterText, szInitialText);
APT_CONVERT_CHECK_MEMBER(AptCharacterTextT, AptCharacterText, szVariable);
APT_CONVERT_CHECK_SIZE(AptCharacterFontT, AptCharacterFont);
APT_CONVERT_CHECK_MEMBER(AptCharacterFontT, AptCharacterFont, apGlyphs);
APT_CONVERT_CHECK_SIZE(AptCharacterButtonT, AptCharacterButton);
APT_CONVERT_CHECK_MEMBER(AptCharacterButtonT, AptCharacterButton, mHitTestVertexTable);
APT_CONVERT_CHECK_MEMBER(AptCharacterButtonT, AptCharacterButton, aButtonRecords);
APT_CONVERT_CHECK_MEMBER(AptCharacterButtonT, AptCharacterButton, aActionConditions);
APT_CONVERT_CHECK_MEMBER(AptCharacterButtonT, AptCharacterButton, pButtonSound);
APT_CONVERT_CHECK_SIZE(AptCharacterSpriteT, AptCharacterSprite);
APT_CONVERT_CHECK_SIZE(AptCharacterSoundT, AptCharacterSound);
APT_CONVERT_CHECK_SIZE(AptCharacterBitmapT, AptCharacterBitmap);
APT_CONVERT_CHECK_SIZE(AptCharacterImageT, AptCharacterImage);
APT_CONVERT_CHECK_SIZE(AptCharacterAnimationT, AptCharacterAnimation);
APT_CONVERT_CHECK_MEMBER(AptCharacterAnimationT, AptCharacterAnimation, apCharacters);
APT_CONVERT_CHECK_MEMBER(AptCharacterAnimationT, AptCharacterAnimation, aImports);
APT_CONVERT_CHECK_MEMBER(AptCharacterAnimationT, AptCharacterAnimation, aExports);
APT_CONVERT_CHECK_MEMBER(AptCharacterAnimationT, AptCharacterAnimation, nCurrentConstantIndex);
APT_CONVERT_CHECK_SIZE(AptCharacterStaticTextT, AptCharacterStaticText);
APT_CONVERT_CHECK_MEMBER(AptCharacterStaticTextT, AptCharacterStaticText, aRecords);
APT_CONVERT_CHECK_SIZE(AptCharacterT, AptCharacter);
APT_CONVERT_CHECK_MEMBER(AptCharacterT, AptCharacter, pParentAnim);
APT_CONVERT_CHECK_MEMBER(AptCharacterT, AptCharacter, animation);

APT_CONVERT_CHECK_SIZE(AptFilterGradientGlowT, AptFilterGradientGlow);
APT_CONVERT_CHECK_MEMBER(AptFilterGradientGlowT, AptFilterGradientGlow, mpGradColors);
APT_CONVERT_CHECK_MEMBER(AptFilterGradientGlowT, AptFilterGradientGlow, mpGradRatios);
APT_CONVERT_CHECK_MEMBER(AptFilterGradientGlowT, AptFilterGradientGlow, mnFlags);

APT_CONVERT_CHECK_SIZE(AptConstFileT, AptConstFile);
APT_CONVERT_CHECK_MEMBER(AptConstFileT, AptConstFile, pMainCharacter);
APT_CONVERT_CHECK_MEMBER(AptConstFileT, AptConstFile, nConstants);
APT_CONVERT_CHECK_MEMBER(AptConstFileT, AptConstFile, aConstants);
APT_CONVERT_CHECK_SIZE(AptConstantTableT, AptConstantTable);

#undef APT_CONVERT_CHECK_SIZE
#undef APT_CONVERT_CHECK_MEMBER

#if APT_PLATFORM_PTR_SIZE == 8
/**
 * Converts a .apt/.const blob pair built by a 32-bit swfc into freshly allocated
 * native-layout (64-bit) blobs. The output blobs are still unresolved - they hold
 * offsets exactly as a 64-bit swfc would have written them - so the regular
 * Resolve/Fixup/Unresolve pipeline runs on them unchanged.
 *
 * Both output buffers are allocated with APT_MALLOC_BLOCK and owned by the caller
 * (free with APT_FREE_BLOCK and the returned sizes). Returns false and allocates
 * nothing on malformed input.
 */
bool AptConvertFile32(const void *pAptData, const void *pConstTable,
                      void **ppNewAptData, size_t *pnNewAptSize,
                      void **ppNewConstTable, size_t *pnNewConstSize);
#endif
