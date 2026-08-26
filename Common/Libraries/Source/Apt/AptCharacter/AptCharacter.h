#pragma once

#include "Apt.h"
#include "AptStd/AptRect.h"
#include "AptMovie.h"

struct AptConstFile;
struct AptCharacter;
struct AptAnimationFile;

class AptRenderableGeometry;
class AptSafeQueueFixed;

class AptCIH;

struct AptCharacterShape
{
    AptRect rBounds;
    AptRenderableGeometry *pRenderUnit;
};

struct AptCharacterMorph
{
    AptCharacter *pStartCharacter;
    AptCharacter *pEndCharacter;
};

struct AptCharacterText
{
    AptRect rBounds;
    int nFontID;
    AptStringAlignment eAlignment;
    unsigned int nColour;
    float fFontHeight;
    int bReadOnly;
    int bMultiLine;
    int bWordWrap;
    char *szInitialText;
    char *szVariable;
};

struct AptCharacterFont
{
    char *szName;
    int nGlyphs;
    AptCharacter **apGlyphs;
};

struct AptCharacterButtonRecord;
struct AptActionConditionBlock;
struct AptCharacterButtonSound;

struct AptCharacterButton
{
#if defined APT_USE_BUTTONS
    int bIsMenu;

    // this section of variable (mHitTestXXXX) are used for hit test of button whose hit test region
    // are shapes

    AptRect mHitTestBoundingRect; // bounding rectangle for fast rejection
    int mHitTestTriangles;        // the triangles(which constructed the hit test region) count
    int mHitTestVertexCount;      // vertex count for the vertex table
    float *mHitTestVertexTable;   // vertex table, length = mHitTestVertexCount * 2;
    short *mHitTestIndexTable;    // index table, length = mHitTestTriangles * 3;

    int nButtonRecords;
    AptCharacterButtonRecord *aButtonRecords;
    int nActionConditions;
    AptActionConditionBlock *aActionConditions;
    AptCharacterButtonSound *pButtonSound;
#endif
};

struct AptCharacterSprite
{
    AptMovie movie;
};

struct AptCharacterSound
{
#if defined(APT_USE_SOUND_OBJECT)
    AptAssetSound zID;
#endif
};

struct AptCharacterBitmap
{
    AptAssetTexture zID;
};

/** @brief A struct to represent a new kind of AptCharacter made specifically for Direct Image Loading */
struct AptCharacterImage
{
    AptRect bounds;
    AptAssetTexture texture;
};

struct AptImport;
struct AptExport;

class AptRenderItem;

struct AptCharacterAnimation : public AptCharacterSprite
{
    // all characters defined in the animation
    int nCharacters;
    AptCharacter **apCharacters;

    unsigned int nWidth;
    unsigned int nHeight;

    unsigned int nMillisecondsPerFrame;

    int nImports;
    AptImport *aImports;

    int nExports;
    AptExport *aExports;

    intptr_t nCurrentConstantIndex;

    void Resolve(void *pAptData, AptConstFile *pConstFile, void *pUserData, AptFilePtr pFile);

    void Link(AptCharacter *pMainCharacter, void *pUserData);
    void Unresolve(void *pAptData);
    void ExecuteInitActions(AptCIH *pInst, int32_t nID); // Executes all DoInitActions for movieclip items - next 2 lines changed also:
    void ExecuteInitAction(AptCIH *pInst, int32_t nID);  // Executes the DoInitActions for a single item
    void ResetInitIndicators();                          // $953 Resets movieclip so that it can be reinitialized
    int32_t IsImport(int nID);
    int32_t GetIDFromImportFile(int32_t nID);

    void ClearCharacterList() const;
    void IncCharacterList(AptFilePtr pFile) const;

  private:
    void Fixup(void *pAptData, AptConstFile *pConstFile, void *pUserData, AptFilePtr pFile);

    intptr_t UnmapCharacter(AptCharacter *pCharacter);
    void ExportClassDefinitionAssets(AptCIH *pInst);
};

struct AptCharacterStaticTextRecords;

struct AptCharacterStaticText
{
    AptRect rBounds;
    AptMatrix matrix;
    int nFontRecords;
    AptCharacterStaticTextRecords *aRecords;
};

// from here to end, not used, to maintain binary compatibility with decoupled
using AptCharacterShapeDataT = struct AptCharacterShapeData_t
{
    mutable uint16_t m_nRef; // Ref count

#if defined(APT_SYSTEM_BIG_ENDIAN)
    uint16_t m_bNotLoaded : 1; // Flag to indicate if texture needs loading
    uint16_t m_textID : 15;    // Bitmap character id in parent animation character library (m_textID == 0 means no texture)
#else
    uint16_t m_textID : 15;    // Bitmap character id in parent animation character library (m_textID == 0 means no texture)
    uint16_t m_bNotLoaded : 1; // Flag to indicate if texture needs loading
#endif
};

using AptCharacterBitmapDataT = struct AptCharacterBitmapData_t
{
    mutable uint16_t m_nRef; // Ref count

    uint16_t m_bLoaded : 1; // Flag to indicate if loaded
    // this Flag will be used only when a bitmap is imported and then to check if all shapes in this
    // animation are to bounded to this imported texture or not

    // with new delayed load/bind system, when a texture is imported and if a shape from exporting file first
    // gets rendered then load texture happens but bind texture happens only for shapes in exporting file
    // and not for shapes in importing file so when we first render shape in importing file, we go thru all
    // characters check for bitmap and check if it is imported then we check for this flag if texture is been bound
    // or not.
    uint16_t m_bBinded : 1;
};

using AptCharacterDataT = struct AptCharacterData_t
{
    mutable uint16_t m_nRef; // Ref count
    uint16_t m_nDynamic : 1; // not used, maintain binary compatibility with decoupled
};

const static uint32_t APT_RENDER_MAX_REFCOUNT = 0xffff;

enum AptCharacterType
{
    AptCharacterType_Shape         = 1,
    AptCharacterType_Text          = 2,
    AptCharacterType_Font          = 3,
    AptCharacterType_Button        = 4,
    AptCharacterType_Sprite        = 5,
    AptCharacterType_Sound         = 6,
    AptCharacterType_Bitmap        = 7,
    AptCharacterType_Morph         = 8,
    AptCharacterType_Animation     = 9,
    AptCharacterType_StaticText    = 10,
    AptCharacterType_None          = 11,
    AptCharacterType_Video         = 12, // for now we just ignore the definvideostream, add this it to avoid crash the pipe/runtime
    AptCharacterType_Level         = 15, // Apt-specific type for Apt level instances.
    AptCharacterType_CustomControl = 16, // Apt-specific type for Custom controls.
    AptCharacterType_Image         = 17, // Apt-specific type for direct-loaded textures
    AptCharacterType_Max
};

struct AptCharacter
{
    APT_NEW_DELETE_OPERATORS
    // Added Metric defines

    AptCharacterType eType;
    AptCharacter *pParentAnim;

// from here to..
#if defined APT_DECOUPLED_RENDERING
    union
    {
        AptCharacterDataT m_data;
        AptCharacterShapeDataT m_shapeData;
        AptCharacterBitmapDataT m_bitmapData;
    };
    mutable AptFilePtr m_pAnimFile;
#endif
    // here will not be used and are here to maintain binary compatibility with decoupled assets
    union
    {
        AptCharacterShape shape;
        AptCharacterMorph morph;
        AptCharacterText text;
        AptCharacterFont font;
#if defined(APT_USE_BUTTONS)
        AptCharacterButton button;
#endif
        AptCharacterSprite sprite;
#if defined(APT_USE_SOUND_OBJECT)
        AptCharacterSound sound;
#endif
        AptCharacterBitmap bitmap;
        AptCharacterAnimation animation;
        AptCharacterStaticText statictext;
        AptCharacterImage image;
    };

  public:
    void render(AptRenderingContext *pRenderingContext, AptMaskRenderOperation eMaskOperation, int nMaskDepth, const AptRenderItem *pRenderItem);
    void GetBoundingRect(AptRenderingContext *pRenderingContext, const AptMatrix *pCurrentTransform, AptRect *pRect) const;

    void SetupCharacter();

    void AddCharacterReference() const;
    void ReleaseCharacterReference() const;

#if defined(APT_DECOUPLED_RENDERING)
    void LoadResourcesFromCharacter(); // added this function to load textures from render thread just before rendering same as before lockless.
#endif

    APT_INLINE uint32_t GetRefCount()
    {
#if defined(APT_DECOUPLED_RENDERING)
        if (eType == AptCharacterType_Shape)
        {
            return m_shapeData.m_nRef;
        }
        else if (eType == AptCharacterType_Bitmap)
        {
            return m_bitmapData.m_nRef;
        }
        else
        {
            return m_data.m_nRef;
        }
#else
        return 1;
#endif
    }

  private:
    void ReleaseAnimationFile() const;
};

/*H*************************************************************************************************
    EOF
*************************************************************************************************H*/
