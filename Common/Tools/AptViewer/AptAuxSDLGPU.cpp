#include "Apt.h"
#include "AptValueFactory.h"
#include "AptAuxSDLGPU.h"

#include "ftapt.h"

#include <SDL3/SDL.h>

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <deque>
#include <time.h>

#define APTAUX_MAXCHARSPERLEVEL 1024
#define APTAUX_MAXTEXPERLEVEL 1024
#define APTAUX_MAXLEVELS 128
#define APTAUX_MAX_PATH 1024

// ---------------------------------------------------------------------------
// data structures (mirroring AptAuxPCOpenGL.h, minus the .big/.geo support)
// ---------------------------------------------------------------------------

enum AptAuxUnitType
{
    AptAuxUnitType_None = 0,
    AptAuxUnitType_TriSolid = 1,
    AptAuxUnitType_TriTiled = 4,
    AptAuxUnitType_TriClipped = 5,
    AptAuxUnitType_LineSolid = 6
};

struct AptAuxTexture
{
    SDL_GPUTexture *pTexture = nullptr;
    int nWidth = 0;
    int nHeight = 0;
};

struct AptAuxUnit
{
    AptAuxUnitType eType = AptAuxUnitType_None;
    int nVertices = 0;           // number of float2 vertices in pBuffer
    SDL_GPUBuffer *pBuffer = nullptr;
    unsigned int nColour = 0;
    int nTextureID = -1;         // file-level texture id (index into aIDToTexture)
    float afMatrix[16];          // texture-space matrix from the .ru file
};

struct AptAuxBitmapMapping
{
    bool bMapped = false;
    unsigned int nBitmapCharID = 0;
    int nTextureID = 0;
};

enum AptAuxCharacterType
{
    AptAuxCharacterType_None = 0,
    AptAuxCharacterType_Geometry = 1,
    AptAuxCharacterType_Texture = 2
};

struct AptAuxIDToChar
{
    AptAuxCharacterType eType = AptAuxCharacterType_None;
    void *pData = nullptr;
};

struct AptAuxLayerInfo;

struct AptAuxShape
{
    AptAuxLayerInfo *pLevel = nullptr;
    int nUnits = 0;
    AptAuxUnit *apUnits[500];
};

struct AptAuxLayerInfo
{
    int bInUse = 0;
    void *pAptData = nullptr;
    AptAuxTexture *aIDToTexture[APTAUX_MAXTEXPERLEVEL];
    AptAuxIDToChar aIDToChar[APTAUX_MAXCHARSPERLEVEL];
    char szBaseFilename[256];
};

struct LoadParams
{
    char szName[256];
    AptAuxLayerInfo *pLevel;
    AptFilePtr pAsyncLoadContext;
    void *pMainData;
    void *pConstTable;
};

static AptGpuRenderer *gpRenderer;
static AptAuxFontList *gpFonts;
static AptAuxLayerInfo gLevelInfo[APTAUX_MAXLEVELS];
static LoadParams *gaFinishedLoads[256];
static int gnFinishedLoads = 0;
static int gnActiveMasks = 0;
static unsigned int gnBackgroundColor = 0;
static FILE *gpSavedInputsFile = nullptr;

static float gafColourScale[4] = { 1, 1, 1, 1 };     // [a, r, g, b], like the GL aux
static float gafColourTranslate[4] = { 0, 0, 0, 0 };
static float gafVertexMatrix[16];

// ---------------------------------------------------------------------------
// file helpers
// ---------------------------------------------------------------------------

static void *_loadFileSize(const char *szFile, int *pnSize)
{
    FILE *pIn = fopen(szFile, "rb");
    if (!pIn)
    {
        return nullptr;
    }
    fseek(pIn, 0, SEEK_END);
    int nSize = (int)ftell(pIn);
    rewind(pIn);
    void *pData = malloc((size_t)nSize + 1);
    fread(pData, 1, (size_t)nSize, pIn);
    ((char *)pData)[nSize] = 0; // convenient for the text parsers
    fclose(pIn);
    *pnSize = nSize;
    return pData;
}

static void *_loadFile(const char *szFile)
{
    int nSize;
    return _loadFileSize(szFile, &nSize);
}

/**
 * Split a string into tokens on a multi-character delimiter.
 */
static char *strsep_m(char **in, const char *delim)
{
    char *token = *in;
    if (token == NULL)
    {
        return NULL;
    }
    char *end = strstr(token, delim);
    if (end)
    {
        *end = '\0';
        end += strlen(delim);
    }
    *in = end;
    return token;
}

// ---------------------------------------------------------------------------
// misc Apt callbacks
// ---------------------------------------------------------------------------

static void *auxMalloc(size_t nSize)
{
    return malloc(nSize);
}

static void debugPrint(const char *szFormat, ...)
{
    va_list args;
    char szBuf[8192];
    va_start(args, szFormat);
    vsnprintf(szBuf, sizeof(szBuf), szFormat, args);
    va_end(args);
    printf("%s", szBuf);
}

static void auxAssert(const char *szMessage, const char *szFile, unsigned int nLine)
{
    printf("%s(%d) : Condition \"%s\"\n", szFile, nLine, szMessage);
}

static void auxAssertMsg(const char *szMessage, const char *szDescription, const char *szFile, unsigned int nLine)
{
    printf("%s(%d) : Condition \"%s\" : Description: \"%s\"\n", szFile, nLine, szMessage, szDescription);
}

static void command(const char *szCommand, const char *szParams)
{
    if (strcmp(szCommand, "setinputroot") == 0)
    {
        if (strcmp(szParams, "void") == 0)
        {
            AptSetInputRoot(0);
        }
        else
        {
            AptSetInputRoot(szParams);
        }
    }
    AptGetUserFuncs().pfnDebugPrint("external command: %s('%s')\n", szCommand, szParams);
}

#define APTAUX_MAX_EXTERN_VARS 32
static struct
{
    char szName[64];
    char szValue[256];
} gaExternVars[APTAUX_MAX_EXTERN_VARS];
static int gnExternVars = 0;

void AptAuxSDLGPU_SetExternVariable(const char *szName, const char *szValue)
{
    if (gnExternVars < APTAUX_MAX_EXTERN_VARS)
    {
        snprintf(gaExternVars[gnExternVars].szName, sizeof(gaExternVars[0].szName), "%s", szName);
        snprintf(gaExternVars[gnExternVars].szValue, sizeof(gaExternVars[0].szValue), "%s", szValue);
        gnExternVars++;
    }
}

static AptValue *getVariableDefault(const char *szVar)
{
    static bool sbTrace = getenv("APTVIEWER_DEBUG") != nullptr;
    if (sbTrace)
    {
        printf("getExternVariable: '%s'\n", szVar);
    }
    for (int i = 0; i < gnExternVars; i++)
    {
        if (strcmp(gaExternVars[i].szName, szVar) == 0)
        {
            return AptValueFactory::CreateString(gaExternVars[i].szValue);
        }
    }
    if (strcmp(szVar, "in_apt") == 0)
    {
        return AptValueFactory::CreateString("1");
    }
    if (strcmp(szVar, "gPlatform") == 0)
    {
        return AptValueFactory::CreateString("PC");
    }
    // unknown game variables default to an empty (falsy) string; override with --extern
    return AptValueFactory::CreateString("");
}

static AptValue *loadVariables(const char *szCommand)
{
    int nSize;
    char *pData = (char *)_loadFileSize(szCommand, &nSize);
    if (!pData)
    {
        return AptValueFactory::CreateString("");
    }
    AptValue *pValue = AptValueFactory::CreateString(pData);
    free(pData);
    return pValue;
}

static AptValue *loadVariablesNULL()
{
    AptGetUserFuncs().pfnDebugPrint("loadVariables: called with NULL parameters\n");
    return AptValueFactory::CreateString("");
}

static void sendVariables(const char *szUrl, const char *szTarget, const char *szMethod, const char *szProp, int bIsSendAndLoad)
{
    AptGetUserFuncs().pfnDebugPrint("sendVariables : '%s' at target '%s ----%s ----%s '\n", szProp, szTarget, szMethod, szUrl);
}

static int getBytesTotal(const char *szFileName, AptGetBytesEnum eGetBytes)
{
    int nRet = 0;
    if (eGetBytes == AptGetBytesEnum_MovieClip)
    {
        char szBuf[APTAUX_MAX_PATH];
        snprintf(szBuf, sizeof(szBuf), "%s.apt", szFileName);
        FILE *pIn = fopen(szBuf, "rb");
        if (pIn)
        {
            fseek(pIn, 0, SEEK_END);
            nRet = (int)ftell(pIn);
            fclose(pIn);
        }
    }
    return nRet;
}

static int getBytesLoaded(const char *szFileName, AptGetBytesEnum eGetBytes)
{
    return getBytesTotal(szFileName, eGetBytes);
}

static void debugSetScreenGrabPending(const char *szScreenGrabName)
{
}

static void debugAddSavedInput(AptSavedInputRecord *pRecord, int nSize)
{
    if (!gpSavedInputsFile)
    {
        gpSavedInputsFile = fopen("saved.inputs", "wb");
    }
    if (gpSavedInputsFile)
    {
        fwrite(pRecord, (size_t)nSize, 1, gpSavedInputsFile);
    }
}

static void setBackgroundColour(unsigned int nColour)
{
    gnBackgroundColor = nColour;
}

unsigned int AptAuxSDLGPU_GetBackgroundColour()
{
    return gnBackgroundColor;
}

static float getStageWidth()
{
    return (float)gpRenderer->GetStageWidth();
}

static float getStageHeight()
{
    return (float)gpRenderer->GetStageHeight();
}

static void getRealTimeClock(AptSysClock *pClock, bool bLocal)
{
    time_t now = time(nullptr);
    struct tm t;
    if (bLocal)
    {
        localtime_r(&now, &t);
    }
    else
    {
        gmtime_r(&now, &t);
    }
    pClock->Second = t.tm_sec;
    pClock->Minute = t.tm_min;
    pClock->Hour = t.tm_hour;
    pClock->Date = t.tm_mday;
    pClock->Day = t.tm_wday;
    pClock->Month = t.tm_mon;
    pClock->Year = t.tm_year + 1900;
    pClock->Hundredths = 0;
}

// ---------------------------------------------------------------------------
// transforms
// ---------------------------------------------------------------------------

static void setVertexMatrixFromMat44(const AptMath::Mat44T *pMatrix)
{
    memset(gafVertexMatrix, 0, sizeof(gafVertexMatrix));
    gafVertexMatrix[0] = pMatrix->m[AptMath::MAT_A];
    gafVertexMatrix[1] = pMatrix->m[AptMath::MAT_B];
    gafVertexMatrix[2] = pMatrix->m[AptMath::MAT_C];
    gafVertexMatrix[4] = pMatrix->m[AptMath::MAT_D];
    gafVertexMatrix[5] = pMatrix->m[AptMath::MAT_E];
    gafVertexMatrix[6] = pMatrix->m[AptMath::MAT_F];
    gafVertexMatrix[8] = pMatrix->m[AptMath::MAT_G];
    gafVertexMatrix[9] = pMatrix->m[AptMath::MAT_H];
    gafVertexMatrix[10] = pMatrix->m[AptMath::MAT_I];
    gafVertexMatrix[12] = pMatrix->m[AptMath::MAT_X];
    gafVertexMatrix[13] = pMatrix->m[AptMath::MAT_Y];
    gafVertexMatrix[14] = pMatrix->m[AptMath::MAT_Z];
    gafVertexMatrix[15] = 1.f;
}

static void applyRenderInfo(const AptRenderInfo *pRenderInfo)
{
    setVertexMatrixFromMat44(&pRenderInfo->mTransform.Pos44);
    pRenderInfo->mTransform.vColorMul4.CopyToFloatArray4(gafColourScale);
    pRenderInfo->mTransform.vColorAdd4.CopyToFloatArray4(gafColourTranslate);

    // With APT_USE_FLASH_COLOR_RANGE (the default) the scale is a percentage
    // (0..100, AptColorHelperScale::SCALE_FACTOR) and the translate a color
    // offset (-255..255, AptColorHelperTranslate::SCALE_FACTOR); normalize both
    // to the 0..1 range the shaders work in.
    for (int i = 0; i < 4; i++)
    {
        gafColourScale[i] /= (float)AptColorHelperScale::SCALE_FACTOR;
        gafColourTranslate[i] /= (float)AptColorHelperTranslate::SCALE_FACTOR;
    }
}

// AptGetUserFuncs().pfnSetVertexMatrix / pfnSetColourTransform (legacy path, still required members)
static void setVertexMatrix(AptMath::Mat44T *pMatrix)
{
    setVertexMatrixFromMat44(pMatrix);
}

static void setColourTransform(AptCXForm *pCXForm)
{
    pCXForm->scale.CopyToFloatArray4(gafColourScale);
    pCXForm->translate.CopyToFloatArray4(gafColourTranslate);
}

// ---------------------------------------------------------------------------
// textures (.tga, loose files)
// ---------------------------------------------------------------------------

#pragma pack(push, 1)
typedef struct
{
    char idlength;
    char colourmaptype;
    char datatypecode;
    short int colourmaporigin;
    short int colourmaplength;
    char colourmapdepth;
    short int x_origin;
    short int y_origin;
    short width;
    short height;
    char bitsperpixel;
    char imagedescriptor;
} TGAHEADER;
#pragma pack(pop)

static void drawImage(AptAssetTexture string, const AptRenderInfo* renderInfo)
{
    printf("drawImage: %p\n", string);
}

static AptAssetTexture loadTexture(void *pUserData, int nID)
{
    AptAuxLayerInfo *pLevel = (AptAuxLayerInfo *)pUserData;

    AptAuxBitmapMapping *pMapping = (AptAuxBitmapMapping *)pLevel->aIDToChar[nID].pData;
    if (pMapping && pMapping->bMapped)
    {
        nID = pMapping->nTextureID;
    }
    if (nID >= 0 && nID < APTAUX_MAXTEXPERLEVEL && pLevel->aIDToTexture[nID])
    {
        return pLevel->aIDToTexture[nID];
    }

    char szBuf[APTAUX_MAX_PATH];
    snprintf(szBuf, sizeof(szBuf), "art/Textures/apt_%s_%d.tga", pLevel->szBaseFilename, nID);
    int nSize;
    unsigned char *pData = (unsigned char *)_loadFileSize(szBuf, &nSize);
    if (!pData)
    {
        AptGetUserFuncs().pfnDebugPrint("Couldn't open texture: '%s'\n", szBuf);
        return nullptr;
    }

    TGAHEADER *pHeader = (TGAHEADER *)pData;
    if (pHeader->bitsperpixel != 32 || pHeader->datatypecode != 2)
    {
        AptGetUserFuncs().pfnDebugPrint("Unsupported .tga format (need uncompressed 32bpp): '%s'\n", szBuf);
        free(pData);
        return nullptr;
    }
    unsigned char *pPixels = pData + sizeof(TGAHEADER) + pHeader->idlength;

    // tga is bgra and bottom-up; we want top-down rgba
    for (int y = 0; y < pHeader->height; y++)
    {
        for (int x = 0; x < pHeader->width; x++)
        {
            unsigned char nTemp = pPixels[(y * pHeader->width + x) * 4 + 0];
            pPixels[(y * pHeader->width + x) * 4 + 0] = pPixels[(y * pHeader->width + x) * 4 + 2];
            pPixels[(y * pHeader->width + x) * 4 + 2] = nTemp;
        }
    }
    if (!(pHeader->imagedescriptor & 0x20))
    {
        for (int y = 0; y < pHeader->height / 2; y++)
        {
            unsigned char *pSrc = &pPixels[y * pHeader->width * 4];
            unsigned char *pDst = &pPixels[(pHeader->height - y - 1) * pHeader->width * 4];
            for (int x = 0; x < pHeader->width * 4; x++)
            {
                unsigned char nTemp = pSrc[x];
                pSrc[x] = pDst[x];
                pDst[x] = nTemp;
            }
        }
    }

    AptAuxTexture *pTexture = new AptAuxTexture;
    pTexture->pTexture = gpRenderer->CreateTextureRGBA(pPixels, pHeader->width, pHeader->height);
    pTexture->nWidth = pHeader->width;
    pTexture->nHeight = pHeader->height;
    free(pData);

    if (nID >= 0 && nID < APTAUX_MAXTEXPERLEVEL)
    {
        pLevel->aIDToTexture[nID] = pTexture;
    }
    return pTexture;
}

static void freeTexture(AptAssetTexture texture)
{
    AptAuxTexture *pTexture = (AptAuxTexture *)texture;
    if (pTexture)
    {
        gpRenderer->DestroyTexture(pTexture->pTexture);
        // note: the texture stays cached in aIDToTexture per level; levels are
        // freed wholesale in freeAnimation
        pTexture->pTexture = nullptr;
    }
}

static void bindTexture(void *pUserData, int nID, AptAssetTexture texture)
{
    AptAuxLayerInfo *pLevel = (AptAuxLayerInfo *)pUserData;
    pLevel->aIDToChar[nID].eType = AptAuxCharacterType_Texture;
    pLevel->aIDToChar[nID].pData = texture;
}

// ---------------------------------------------------------------------------
// bitmap-character mappings (<base>.dat) and rendering units (<base>_geometry/<id>.ru)
// ---------------------------------------------------------------------------

static bool loadMappings(const char *szBaseName, AptAuxLayerInfo *pLevel)
{
    char szFilename[APTAUX_MAX_PATH];
    snprintf(szFilename, sizeof(szFilename), "%s.dat", szBaseName);

    char *pData = (char *)_loadFile(szFilename);
    if (!pData)
    {
        AptGetUserFuncs().pfnDebugPrint("Couldn't find .dat file: '%s'\n", szFilename);
        return false;
    }

    char *pSave = nullptr;
    for (char *line = strtok_r(pData, "\r\n", &pSave); line != nullptr; line = strtok_r(nullptr, "\r\n", &pSave))
    {
        if (strlen(line) == 0 || line[0] == ';')
        {
            continue;
        }
        AptAuxBitmapMapping *pMapping = new AptAuxBitmapMapping;
        if (strstr(line, "->") != nullptr)
        {
            pMapping->bMapped = true;
            pMapping->nBitmapCharID = atoi(strsep_m(&line, "->"));
            pMapping->nTextureID = atoi(strsep_m(&line, "->"));
        }
        else if (strstr(line, "=") != nullptr)
        {
            pMapping->bMapped = false;
            pMapping->nBitmapCharID = atoi(strsep_m(&line, "="));
            pMapping->nTextureID = (int)pMapping->nBitmapCharID;
        }
        else
        {
            delete pMapping;
            continue;
        }
        assert(pMapping->nBitmapCharID < APTAUX_MAXCHARSPERLEVEL);
        pLevel->aIDToChar[pMapping->nBitmapCharID].eType = AptAuxCharacterType_Texture;
        pLevel->aIDToChar[pMapping->nBitmapCharID].pData = pMapping;
    }
    free(pData);
    return true;
}

static void loadIdentityMatrix(float mat[16])
{
    memset(mat, 0, sizeof(float) * 16);
    mat[0] = mat[5] = mat[10] = mat[15] = 1.0f;
}

static void finishUnit(AptAuxShape *pShape, AptAuxUnit *pUnit, std::deque<float> &prims)
{
    if (!pUnit)
    {
        return;
    }
    pUnit->nVertices = (int)prims.size() / 2;
    if (pUnit->nVertices > 0)
    {
        float *pData = new float[prims.size()];
        for (size_t i = 0; i < prims.size(); i++)
        {
            pData[i] = prims[i];
        }
        pUnit->pBuffer = gpRenderer->CreateVertexBuffer(pData, (int)prims.size());
        delete[] pData;
    }
    prims.clear();
    assert(pShape->nUnits < (int)(sizeof(pShape->apUnits) / sizeof(pShape->apUnits[0])));
    pShape->apUnits[pShape->nUnits++] = pUnit;
}

static AptAssetRenderingUnit loadRenderingUnit(void *pUserData, int nID)
{
    AptAuxLayerInfo *pLevel = (AptAuxLayerInfo *)pUserData;

    char szFilename[APTAUX_MAX_PATH];
    snprintf(szFilename, sizeof(szFilename), "%s_geometry/%i.ru", pLevel->szBaseFilename, nID);
    char *pData = (char *)_loadFile(szFilename);
    if (!pData)
    {
        AptGetUserFuncs().pfnDebugPrint("Couldn't open rendering unit: '%s'\n", szFilename);
        return nullptr;
    }

    AptAuxShape *pShape = new AptAuxShape;
    pShape->pLevel = pLevel;

    std::deque<float> prims;
    AptAuxUnit *pUnit = nullptr;
    unsigned char aColour[4] = {};
    char *pSave = nullptr;
    for (char *line = strtok_r(pData, "\r\n", &pSave); line != nullptr; line = strtok_r(nullptr, "\r\n", &pSave))
    {
        char *token;
        switch (line[0])
        {
            case 'c': // new unit
            {
                finishUnit(pShape, pUnit, prims);
                pUnit = new AptAuxUnit;
                break;
            }
            case 's': // style
            {
                if (!pUnit)
                {
                    break;
                }
                line += 2;
                if (strncmp(line, "tc", 2) == 0)
                {
                    line += 3;
                    pUnit->eType = AptAuxUnitType_TriClipped;
                    int nColourIndex = 0;
                    while (nColourIndex < 4 && (token = strsep(&line, ":")) != nullptr)
                    {
                        aColour[nColourIndex++] = (unsigned char)atoi(token);
                    }
                    pUnit->nColour = aColour[0] | (aColour[1] << 8) | (aColour[2] << 16) | (aColour[3] << 24);
                    int nBitmapCharID = atoi(strsep(&line, ":"));
                    AptAuxBitmapMapping *pMapping = (AptAuxBitmapMapping *)pLevel->aIDToChar[nBitmapCharID].pData;
                    pUnit->nTextureID = pMapping ? pMapping->nTextureID : nBitmapCharID;
                    if (getenv("APTVIEWER_DEBUG"))
                    {
                        printf("ru %d: tc unit char %d -> tex %d (mapping %p)\n",
                               nID, nBitmapCharID, pUnit->nTextureID, (void *)pMapping);
                    }
                    loadIdentityMatrix(pUnit->afMatrix);
                    pUnit->afMatrix[0] = (float)atof(strsep(&line, ":"));
                    pUnit->afMatrix[4] = (float)atof(strsep(&line, ":"));
                    pUnit->afMatrix[1] = (float)atof(strsep(&line, ":"));
                    pUnit->afMatrix[5] = (float)atof(strsep(&line, ":"));
                    pUnit->afMatrix[12] = (float)atof(strsep(&line, ":"));
                    pUnit->afMatrix[13] = (float)atof(strsep(&line, ":"));
                }
                else if (strncmp(line, "s", 1) == 0)
                {
                    line += 2;
                    pUnit->eType = AptAuxUnitType_TriSolid;
                    int nColourIndex = 0;
                    while (nColourIndex < 4 && (token = strsep(&line, ":")) != nullptr)
                    {
                        aColour[nColourIndex++] = (unsigned char)atoi(token);
                    }
                    pUnit->nColour = aColour[0] | (aColour[1] << 8) | (aColour[2] << 16) | (aColour[3] << 24);
                }
                break;
            }
            case 't': // triangle vertices
            {
                line += 2;
                while ((token = strsep(&line, ":")) != nullptr)
                {
                    prims.emplace_back((float)atof(token));
                }
                break;
            }
            default:
                break;
        }
    }
    finishUnit(pShape, pUnit, prims);
    free(pData);

    assert(nID < APTAUX_MAXCHARSPERLEVEL);
    pLevel->aIDToChar[nID].pData = pShape;
    pLevel->aIDToChar[nID].eType = AptAuxCharacterType_Geometry;
    return (AptAssetRenderingUnit)pShape;
}

static void freeRenderingUnit(AptAssetRenderingUnit renderingUnit)
{
    // shapes stay cached in the level tables; freed wholesale on level unload
}

// ---------------------------------------------------------------------------
// drawing
// ---------------------------------------------------------------------------

static void applyColourTransform(unsigned int nColour, bool bTranslate, float afOut[4])
{
    float a = (nColour >> 24) / 255.f;
    float r = ((nColour >> 16) & 0xff) / 255.f;
    float g = ((nColour >> 8) & 0xff) / 255.f;
    float b = (nColour & 0xff) / 255.f;

    a *= gafColourScale[0];
    r *= gafColourScale[1];
    g *= gafColourScale[2];
    b *= gafColourScale[3];
    if (bTranslate)
    {
        a += gafColourTranslate[0];
        r += gafColourTranslate[1];
        g += gafColourTranslate[2];
        b += gafColourTranslate[3];
    }
    afOut[0] = r;
    afOut[1] = g;
    afOut[2] = b;
    afOut[3] = a;
}

static AptAuxTexture *unitTexture(const AptAuxShape *pShape, const AptAuxUnit *pUnit)
{
    if (pUnit->nTextureID < 0 || pUnit->nTextureID >= APTAUX_MAXTEXPERLEVEL)
    {
        if (getenv("APTVIEWER_DEBUG"))
        {
            printf("unitTexture: id %d out of range\n", pUnit->nTextureID);
        }
        return nullptr;
    }
    AptAuxTexture *pTexture = pShape->pLevel->aIDToTexture[pUnit->nTextureID];
    if (!pTexture || !pTexture->pTexture)
    {
        if (getenv("APTVIEWER_DEBUG"))
        {
            printf("unitTexture: id %d not loaded\n", pUnit->nTextureID);
        }
        return nullptr;
    }
    return pTexture;
}

static void drawRenderingUnit(AptAssetRenderingUnit renderingUnit, const AptRenderInfo *pRenderInfo)
{
    assert(renderingUnit);
    AptAuxShape *pShape = (AptAuxShape *)renderingUnit;
    applyRenderInfo(pRenderInfo);

    static bool sbTrace = getenv("APTVIEWER_DEBUG") != nullptr;
    if (sbTrace)
    {
        printf("draw shape=%p units=%d type0=%d colour0=%08x mask=%d mul=[%.2f %.2f %.2f %.2f] add=[%.2f %.2f %.2f %.2f]\n",
               (void *)pShape, pShape->nUnits,
               pShape->nUnits ? pShape->apUnits[0]->eType : -1,
               pShape->nUnits ? pShape->apUnits[0]->nColour : 0,
               (int)pRenderInfo->meMaskOper,
               gafColourScale[0], gafColourScale[1], gafColourScale[2], gafColourScale[3],
               gafColourTranslate[0], gafColourTranslate[1], gafColourTranslate[2], gafColourTranslate[3]);
    }

    if (pRenderInfo->meMaskOper == AptMaskRenderOperation_None)
    {
        for (int iUnit = 0; iUnit < pShape->nUnits; iUnit++)
        {
            AptAuxUnit *pUnit = pShape->apUnits[iUnit];
            switch (pUnit->eType)
            {
                case AptAuxUnitType_TriSolid:
                case AptAuxUnitType_LineSolid:
                {
                    float afColor[4];
                    applyColourTransform(pUnit->nColour, true, afColor);
                    gpRenderer->DrawSolid(pUnit->pBuffer, pUnit->nVertices,
                                          pUnit->eType == AptAuxUnitType_LineSolid,
                                          gafVertexMatrix, afColor);
                    break;
                }
                case AptAuxUnitType_TriTiled:
                case AptAuxUnitType_TriClipped:
                {
                    AptAuxTexture *pTexture = unitTexture(pShape, pUnit);
                    if (!pTexture)
                    {
                        break;
                    }
                    // colour scale is applied via the multiplier, translate via the add
                    // stage (matching the two-stage texture combiner of the GL aux)
                    float afColorMul[4];
                    applyColourTransform(pUnit->nColour, false, afColorMul);
                    float afColorAdd[4] = { gafColourTranslate[1], gafColourTranslate[2],
                                            gafColourTranslate[3], gafColourTranslate[0] };

                    // uv = 1/texSize * unit matrix, applied to the vertex position
                    float afUVMatrix[16];
                    memcpy(afUVMatrix, pUnit->afMatrix, sizeof(afUVMatrix));
                    for (int i = 0; i < 16; i += 4)
                    {
                        afUVMatrix[i + 0] /= (float)pTexture->nWidth;
                        afUVMatrix[i + 1] /= (float)pTexture->nHeight;
                    }
                    gpRenderer->DrawTextured(pUnit->pBuffer, pUnit->nVertices,
                                             gafVertexMatrix, afUVMatrix, afColorMul, afColorAdd,
                                             pTexture->pTexture,
                                             pUnit->eType == AptAuxUnitType_TriTiled);
                    break;
                }
                default:
                    break;
            }
        }
    }
    else
    {
        // mask operation: write the shape into the stencil buffer
        const bool bAdd = (pRenderInfo->meMaskOper == AptMaskRenderOperation_Add);
        if (bAdd)
        {
            gnActiveMasks++;
        }
        else
        {
            assert(gnActiveMasks > 0);
            gnActiveMasks--;
        }

        for (int iUnit = 0; iUnit < pShape->nUnits; iUnit++)
        {
            AptAuxUnit *pUnit = pShape->apUnits[iUnit];
            if (pUnit->eType == AptAuxUnitType_None)
            {
                continue;
            }
            gpRenderer->DrawMask(pUnit->pBuffer, pUnit->nVertices,
                                 pUnit->eType == AptAuxUnitType_LineSolid,
                                 gafVertexMatrix, bAdd);
        }

        gpRenderer->SetMaskingEnabled(gnActiveMasks > 0);
    }
}

static void customControlRender(char *szType, char *szTarget, AptAssetRenderingUnit renderingUnit,
                                const char *szCustomProperties, const AptRenderInfo *pRenderInfo)
{
    applyRenderInfo(pRenderInfo);
}

static bool customControlUpdate(AptAssetRenderingUnit renderingUnit)
{
    return false;
}

// ---------------------------------------------------------------------------
// text (FreeType via c_aptft)
// ---------------------------------------------------------------------------

static CFreeType gFreeType;

static void UCS2ToUTF8(const unsigned short *src, int buflen, char *dst)
{
    if (buflen == -1)
    {
        const unsigned short *p = src;
        buflen = 0;
        while (*p != 0)
        {
            p++;
            buflen++;
        }
        buflen++;
    }
    for (int i = 0; i < buflen; i++)
    {
        if (src[i] <= 0x7f)
        {
            *dst++ = (char)src[i];
        }
        else if (src[i] <= 0x7ff)
        {
            *dst++ = (char)(0xc0 | ((src[i] & 0x07c0) >> 6));
            *dst++ = (char)(0x80 | (src[i] & 0x003f));
        }
        else
        {
            *dst++ = (char)(0xe0 | ((src[i] & 0xf000) >> 12));
            *dst++ = (char)(0x80 | ((src[i] & 0x0fc0) >> 6));
            *dst++ = (char)(0x80 | (src[i] & 0x003f));
        }
    }
}

static void UTF8ToUCS2(const char *src, int len, uint16_t *dst)
{
    const unsigned char *pSrc = (const unsigned char *)src;
    while ((const char *)pSrc < src + len)
    {
        if (*pSrc <= 127)
        {
            *dst++ = *pSrc++;
        }
        else if ((*pSrc & 0xe0) == 0xc0)
        {
            *dst++ = (uint16_t)(((pSrc[0] & 0x1f) << 6) | (pSrc[1] & 0x3f));
            pSrc += 2;
        }
        else if ((*pSrc & 0xf0) == 0xe0)
        {
            *dst++ = (uint16_t)(((pSrc[0] & 0x0f) << 12) | ((pSrc[1] & 0x3f) << 6) | (pSrc[2] & 0x3f));
            pSrc += 3;
        }
        else
        {
            assert(0 && "UCS4 is not supported");
            pSrc++;
        }
    }
}

const int FFB_MAXSTRINGS = 200;
struct AllocatedString
{
    int bUsed;
    SDL_GPUTexture *pTexture;
    unsigned int nColour;
    float fLeft, fTop, fRight, fBottom;
    float fTexWidth, fTexHeight;
    float fTextWidth, fTextHeight;
    float fYAscender;
    AptStringAlignment eAlignment;
    AptStringAlignment eBoxAlignment;
    unsigned int nBackColor;
    unsigned int nBorderColor;
    int bBackground;
    int bBorder;
    int nMaxScroll;
};

static AllocatedString gAllocStrings[FFB_MAXSTRINGS];
static int gnAllocStrings = 0;

static AptAssetString allocateString(AptAllocateStringParameters *pASP)
{
    static bool sbTrace = getenv("APTVIEWER_DEBUG") != nullptr;
    if (sbTrace)
    {
        printf("allocateString: '%s' font='%s' flags=%x rect=(%.0f,%.0f)-(%.0f,%.0f) curr=%p\n",
               pASP->szString ? pASP->szString : "(null)",
               pASP->szFontName ? pASP->szFontName : "(null)",
               pASP->eFlags, pASP->x0, pASP->y0, pASP->x1, pASP->y1, pASP->pCurrString);
    }
    if (gpFonts->nFonts == 0)
    {
        return 0;
    }

    if (!(pASP->eFlags & (APT_TEXTFIELD_FUPDATE | APT_TEXTFIELD_DIRTY | APT_TEXTFIELD_TEXTCOLOR)))
    {
        AllocatedString *pString = (AllocatedString *)pASP->pCurrString;
        if (!(pASP->eFlags & APT_TEXTFIELD_ASCHANGE))
        {
            pString->bUsed = 1;
            pString->bBackground = pASP->bBackground;
            pString->bBorder = pASP->bBorder;
            pString->nBackColor = pASP->nBackColor;
            pString->nBorderColor = pASP->nBorderColor;
        }
        pASP->nMaxScroll = pString->nMaxScroll;
        pASP->fTextWidth = pString->fTextWidth;
        pASP->fTextHeight = pString->fTextHeight;
        pASP->fStrLen = 1000;
        return pASP->pCurrString;
    }

    float fWidth = pASP->x1 - pASP->x0;
    float fHeight = pASP->y1 - pASP->y0;
    int nWidth = (int)fWidth;
    int nHeight = (int)fHeight;

    // find the font by name
    int nFontIndex = 0;
    if (pASP->szFontName)
    {
        char szUTF8Name[256];
        for (int i = 0; i < gpFonts->nFonts; i++)
        {
            UCS2ToUTF8(gpFonts->aszFontNamesEnglish[i], -1, szUTF8Name);
            if (strcmp(szUTF8Name, pASP->szFontName) == 0)
            {
                nFontIndex = i;
                break;
            }
            UCS2ToUTF8(gpFonts->aszFontNamesJapanese[i], -1, szUTF8Name);
            if (strcmp(szUTF8Name, pASP->szFontName) == 0)
            {
                nFontIndex = i;
                break;
            }
        }
    }

    static uint16_t aUCS2[64 * 1024];
    int nLen = (int)strlen(pASP->szString);
    memset(aUCS2, 0, sizeof(uint16_t) * (nLen + 1));
    UTF8ToUCS2(pASP->szString, nLen + 1, aUCS2);

    if (pASP->eBoxAlignment != AptStringAlignment_None)
    {
        gFreeType.calcRenderStringSize(nFontIndex, aUCS2, (int)pASP->fFontHeight, nWidth, nHeight,
                                       2, 0, 0, pASP->bWordWrap, pASP->nFontStyle, (int)fWidth,
                                       (CFreeType::Alignment)pASP->eAlignment, pASP->nLineOffset);
        pASP->y1 = (float)nHeight;
        fWidth = (float)nWidth;
        fHeight = (float)nHeight;
    }

    int nTexWidth = 2 << (int)(logf(fWidth - 1) / logf(2.0f));
    int nTexHeight = 2 << (int)(logf(fHeight - 1) / logf(2.0f));
    if (nTexWidth <= 0 || nTexHeight <= 0)
    {
        return 0;
    }
    unsigned char *pCoverage = new unsigned char[nTexWidth * nTexHeight];
    memset(pCoverage, 0, (size_t)nTexWidth * nTexHeight);

    AllocatedString *pString = nullptr;
    int nString;
    assert(gnAllocStrings < FFB_MAXSTRINGS);
    for (nString = 0; nString < FFB_MAXSTRINGS; nString++)
    {
        if (!gAllocStrings[nString].bUsed)
        {
            pString = &gAllocStrings[nString];
            break;
        }
    }
    if (!pString)
    {
        delete[] pCoverage;
        return 0;
    }
    gnAllocStrings++;

    unsigned int nTextWidth, nTextHeight;
    float fYAscender = 0.f;
    pASP->nMaxScroll = gFreeType.renderString(nFontIndex, aUCS2, (int)pASP->fFontHeight,
                                              nTexWidth, nTexHeight, pCoverage,
                                              &nTextWidth, &nTextHeight, &fYAscender,
                                              2, 0, 0, pASP->bWordWrap, pASP->nFontStyle,
                                              nWidth, nHeight,
                                              (CFreeType::Alignment)pASP->eAlignment, pASP->nLineOffset);
    if (sbTrace)
    {
        int nMaxCoverage = 0;
        for (int i = 0; i < nTexWidth * nTexHeight; i++)
        {
            if (pCoverage[i] > nMaxCoverage)
            {
                nMaxCoverage = pCoverage[i];
            }
        }
        printf("renderString: font=%d tex=%dx%d text=%ux%u maxCoverage=%d\n",
               nFontIndex, nTexWidth, nTexHeight, nTextWidth, nTextHeight, nMaxCoverage);
    }
    pASP->fTextWidth = (float)nTextWidth;
    pASP->fTextHeight = (float)nTextHeight;
    pASP->fStrLen = 1000;

    pString->fLeft = pASP->x0;
    pString->fRight = pASP->x1;
    switch (pASP->eAlignment)
    {
        case AptStringAlignment_Center:
            pString->fLeft = (pASP->x0 + pASP->x1) / 2 - fWidth / 2;
            pString->fRight = (pASP->x0 + pASP->x1) / 2 + fWidth / 2;
            if (!pASP->bWordWrap)
            {
                pASP->x0 = pString->fLeft;
                pASP->x1 = pString->fRight;
            }
            break;
        case AptStringAlignment_Right:
            pString->fLeft = pASP->x1 - fWidth;
            pASP->x0 = pString->fLeft;
            break;
        case AptStringAlignment_Left:
            pString->fRight = pASP->x0 + fWidth;
            pASP->x1 = pString->fRight;
            break;
        default:
            break;
    }

    pString->bUsed = 1;
    pString->nColour = pASP->nColour;
    pString->fTop = pASP->y0 + 2;
    pString->fBottom = pASP->y1;
    pString->fTexWidth = (float)nTexWidth;
    pString->fTexHeight = (float)nTexHeight;
    pString->eAlignment = pASP->eAlignment;
    pString->eBoxAlignment = pASP->eBoxAlignment;
    pString->bBackground = pASP->bBackground;
    pString->bBorder = pASP->bBorder;
    pString->nBackColor = pASP->nBackColor;
    pString->nBorderColor = pASP->nBorderColor;
    pString->fTextWidth = (float)nTextWidth;
    pString->fTextHeight = (float)nTextHeight;
    pString->fYAscender = fYAscender;
    pString->nMaxScroll = (int)pASP->nMaxScroll;

    // expand coverage to white RGBA (FreeType coverage is already full 0..255 range)
    unsigned char *pRGBA = new unsigned char[nTexWidth * nTexHeight * 4];
    for (int i = 0; i < nTexWidth * nTexHeight; i++)
    {
        pRGBA[i * 4 + 0] = 255;
        pRGBA[i * 4 + 1] = 255;
        pRGBA[i * 4 + 2] = 255;
        pRGBA[i * 4 + 3] = pCoverage[i];
    }
    pString->pTexture = gpRenderer->CreateTextureRGBA(pRGBA, nTexWidth, nTexHeight);
    delete[] pCoverage;
    delete[] pRGBA;

    return pString;
}

static void makeRectMatrix(float afOut[16], float fLeft, float fTop, float fRight, float fBottom)
{
    // maps the unit quad onto the rectangle
    memset(afOut, 0, sizeof(float) * 16);
    afOut[0] = fRight - fLeft;
    afOut[5] = fBottom - fTop;
    afOut[10] = 1.f;
    afOut[12] = fLeft;
    afOut[13] = fTop;
    afOut[15] = 1.f;
}

static void drawString(AptAssetString string, const AptRenderInfo *pRenderInfo)
{
    static bool sbTrace = getenv("APTVIEWER_DEBUG") != nullptr;
    if (sbTrace)
    {
        printf("drawString: %p\n", string);
    }
    AllocatedString *pString = (AllocatedString *)string;
    if (!pString)
    {
        return;
    }
    assert(pString->bUsed);
    applyRenderInfo(pRenderInfo);

    const float w = pString->fRight - pString->fLeft;
    const float h = pString->fBottom - pString->fTop;
    const float nBorderBuff = 2.f;

    // text box rect for background/border, depending on the auto-size alignment
    float fBoxLeft = pString->fLeft;
    float fBoxTop = pString->fTop;
    float fBoxRight = pString->fRight;
    float fBoxBottom = pString->fBottom;
    if (pString->bBackground || pString->bBorder)
    {
        if (pString->eBoxAlignment == AptStringAlignment_Center)
        {
            fBoxLeft = pString->fLeft + (w - pString->fTextWidth) / 2.f - nBorderBuff;
            fBoxRight = pString->fRight - (w - pString->fTextWidth) / 2.f + nBorderBuff;
            fBoxBottom = pString->fTop + pString->fTextHeight;
        }
        else if (pString->eBoxAlignment == AptStringAlignment_Right)
        {
            fBoxLeft = pString->fLeft + (w - pString->fTextWidth) - nBorderBuff;
            fBoxRight = pString->fRight + nBorderBuff;
            fBoxBottom = pString->fTop + pString->fTextHeight;
        }
        else if (pString->eBoxAlignment == AptStringAlignment_Left)
        {
            fBoxLeft = pString->fLeft - nBorderBuff;
            fBoxRight = pString->fRight - (w - pString->fTextWidth) + nBorderBuff;
            fBoxBottom = pString->fTop + pString->fTextHeight;
        }
    }

    float afRect[16];
    if (pString->bBackground)
    {
        float afColor[4];
        applyColourTransform(0xFF000000 | pString->nBackColor, true, afColor);
        makeRectMatrix(afRect, fBoxLeft, fBoxTop, fBoxRight, fBoxBottom);
        float afModel[16];
        // model = vertexMatrix * rect
        for (int col = 0; col < 4; col++)
        {
            for (int row = 0; row < 4; row++)
            {
                afModel[col * 4 + row] = gafVertexMatrix[0 * 4 + row] * afRect[col * 4 + 0] +
                                         gafVertexMatrix[1 * 4 + row] * afRect[col * 4 + 1] +
                                         gafVertexMatrix[2 * 4 + row] * afRect[col * 4 + 2] +
                                         gafVertexMatrix[3 * 4 + row] * afRect[col * 4 + 3];
            }
        }
        gpRenderer->DrawSolid(gpRenderer->GetUnitQuadTriangles(), 6, false, afModel, afColor);
    }

    // the text quad
    {
        float afColorMul[4];
        applyColourTransform(0xFF000000 | pString->nColour, true, afColorMul);
        makeRectMatrix(afRect, pString->fLeft, pString->fTop, pString->fRight, pString->fBottom);
        float afModel[16];
        for (int col = 0; col < 4; col++)
        {
            for (int row = 0; row < 4; row++)
            {
                afModel[col * 4 + row] = gafVertexMatrix[0 * 4 + row] * afRect[col * 4 + 0] +
                                         gafVertexMatrix[1 * 4 + row] * afRect[col * 4 + 1] +
                                         gafVertexMatrix[2 * 4 + row] * afRect[col * 4 + 2] +
                                         gafVertexMatrix[3 * 4 + row] * afRect[col * 4 + 3];
            }
        }
        // uv: unit quad -> [0, w/texW] x [0, h/texH]
        float afUVMatrix[16];
        loadIdentityMatrix(afUVMatrix);
        afUVMatrix[0] = w / pString->fTexWidth;
        afUVMatrix[5] = h / pString->fTexHeight;
        static const float afNoAdd[4] = { 0, 0, 0, 0 };
        gpRenderer->DrawTextured(gpRenderer->GetUnitQuadTriangles(), 6, afModel, afUVMatrix,
                                 afColorMul, afNoAdd, pString->pTexture, false);
    }

    if (pString->bBorder)
    {
        float afColor[4];
        applyColourTransform(0xFF000000 | pString->nBorderColor, true, afColor);
        makeRectMatrix(afRect, fBoxLeft, fBoxTop, fBoxRight, fBoxBottom);
        float afModel[16];
        for (int col = 0; col < 4; col++)
        {
            for (int row = 0; row < 4; row++)
            {
                afModel[col * 4 + row] = gafVertexMatrix[0 * 4 + row] * afRect[col * 4 + 0] +
                                         gafVertexMatrix[1 * 4 + row] * afRect[col * 4 + 1] +
                                         gafVertexMatrix[2 * 4 + row] * afRect[col * 4 + 2] +
                                         gafVertexMatrix[3 * 4 + row] * afRect[col * 4 + 3];
            }
        }
        gpRenderer->DrawSolid(gpRenderer->GetUnitQuadOutline(), 8, true, afModel, afColor);
    }
}

static void deallocateString(AptAssetString string, unsigned int eFlags)
{
    if (!(eFlags & (APT_TEXTFIELD_FUPDATE | APT_TEXTFIELD_DIRTY | APT_TEXTFIELD_TEXTCOLOR)))
    {
        return;
    }
    AllocatedString *pString = (AllocatedString *)string;
    if (!pString || !pString->bUsed)
    {
        return;
    }
    pString->bUsed = 0;
    gpRenderer->DestroyTexture(pString->pTexture);
    pString->pTexture = nullptr;
    gnAllocStrings--;
}

// ---------------------------------------------------------------------------
// sound stubs (the viewer is silent)
// ---------------------------------------------------------------------------

#if defined(APT_USE_SOUND_OBJECT)
static AptAssetSound loadSound(void *pUserData, int nID, const char *szName)
{
    return nullptr;
}

static void freeSound(AptAssetSound sound)
{
}

static void startSound(AptAssetSound sound, const char *szName)
{
}

static void startSoundStream(void *pUserData, int nID)
{
}
#endif

// ---------------------------------------------------------------------------
// effects / render flags stubs
// ---------------------------------------------------------------------------

static AptAssetEffect createEffect(const AptEffectData &uiEffectData, AptValue *movieClip)
{
    return nullptr;
}

static void destroyEffect(AptAssetEffect uiEffect)
{
}

static void pushEffect(const AptRenderInfo &renderInfo)
{
}

static void popEffect(const AptRenderInfo &renderInfo)
{
}

// ---------------------------------------------------------------------------
// animation loading (loose files, synchronous)
// ---------------------------------------------------------------------------

static void loadAnimation(const char *szBaseName, AptFilePtr pAsyncLoadContext)
{
    LoadParams *pLP = new LoadParams();
    strncpy(pLP->szName, szBaseName, sizeof(pLP->szName) - 1);
    pLP->szName[sizeof(pLP->szName) - 1] = 0;
    pLP->pAsyncLoadContext = pAsyncLoadContext;
    pLP->pLevel = nullptr;
    pLP->pMainData = nullptr;
    pLP->pConstTable = nullptr;

    for (int i = 0; i < APTAUX_MAXLEVELS; i++)
    {
        if (!gLevelInfo[i].bInUse)
        {
            pLP->pLevel = &gLevelInfo[i];
            break;
        }
    }
    assert(pLP->pLevel);

    // strip any directory component: all assets live relative to the working directory
    const char *pSlash = strrchr(pLP->szName, '/');
    const char *pBaseName = pSlash ? pSlash + 1 : pLP->szName;

    char szFilename[APTAUX_MAX_PATH];
    snprintf(szFilename, sizeof(szFilename), "%s.apt", pBaseName);
    void *pAptData = _loadFile(szFilename);
    if (!pAptData)
    {
        AptGetUserFuncs().pfnDebugPrint("Couldn't open file: '%s'\n", szFilename);
        assert(gnFinishedLoads < (int)(sizeof(gaFinishedLoads) / sizeof(gaFinishedLoads[0])));
        gaFinishedLoads[gnFinishedLoads++] = pLP; // completed with pMainData == NULL -> discarded
        return;
    }

    snprintf(szFilename, sizeof(szFilename), "%s.const", pBaseName);
    void *pConstData = _loadFile(szFilename);
    if (!pConstData)
    {
        AptGetUserFuncs().pfnDebugPrint("Couldn't open file: '%s'\n", szFilename);
        free(pAptData);
        assert(gnFinishedLoads < (int)(sizeof(gaFinishedLoads) / sizeof(gaFinishedLoads[0])));
        gaFinishedLoads[gnFinishedLoads++] = pLP;
        return;
    }

    AptAuxLayerInfo *pLevel = pLP->pLevel;
    pLevel->bInUse = 1;
    pLevel->pAptData = pAptData;
    strncpy(pLevel->szBaseFilename, pBaseName, sizeof(pLevel->szBaseFilename) - 1);
    pLevel->szBaseFilename[sizeof(pLevel->szBaseFilename) - 1] = 0;
    memset(pLevel->aIDToChar, 0, sizeof(pLevel->aIDToChar));
    memset(pLevel->aIDToTexture, 0, sizeof(pLevel->aIDToTexture));

    loadMappings(pBaseName, pLevel);

    pLP->pMainData = pAptData;
    pLP->pConstTable = pConstData;

    assert(gnFinishedLoads < (int)(sizeof(gaFinishedLoads) / sizeof(gaFinishedLoads[0])));
    gaFinishedLoads[gnFinishedLoads++] = pLP;
}

void AptAuxSDLGPU_FinishAsyncLoads()
{
    for (int i = 0; i < gnFinishedLoads; i++)
    {
        if (gaFinishedLoads[i]->pMainData)
        {
            AptCompleteAnimationAsyncLoad(gaFinishedLoads[i]->pAsyncLoadContext,
                                          gaFinishedLoads[i]->pMainData,
                                          gaFinishedLoads[i]->pConstTable,
                                          gaFinishedLoads[i]->pLevel);
        }
        delete gaFinishedLoads[i];
    }
    gnFinishedLoads = 0;
}

static void loadAnimationCompleted(const char *szBaseName, const char *szTargetName)
{
}

static void freeConstantTable(void *pConstantTable)
{
    free(pConstantTable);
}

static void freeAnimation(void *pUserHandle)
{
    AptAuxLayerInfo *pLevel = (AptAuxLayerInfo *)pUserHandle;
    if (!pLevel)
    {
        return;
    }

    for (int i = 0; i < APTAUX_MAXCHARSPERLEVEL; i++)
    {
        if (pLevel->aIDToChar[i].eType == AptAuxCharacterType_Geometry)
        {
            AptAuxShape *pShape = (AptAuxShape *)pLevel->aIDToChar[i].pData;
            for (int j = 0; j < pShape->nUnits; j++)
            {
                gpRenderer->DestroyVertexBuffer(pShape->apUnits[j]->pBuffer);
                delete pShape->apUnits[j];
            }
            delete pShape;
        }
        else if (pLevel->aIDToChar[i].eType == AptAuxCharacterType_Texture &&
                 pLevel->aIDToChar[i].pData)
        {
            delete (AptAuxBitmapMapping *)pLevel->aIDToChar[i].pData;
        }
        pLevel->aIDToChar[i].eType = AptAuxCharacterType_None;
        pLevel->aIDToChar[i].pData = nullptr;
    }
    for (int i = 0; i < APTAUX_MAXTEXPERLEVEL; i++)
    {
        if (pLevel->aIDToTexture[i])
        {
            // the SDL texture itself was released through pfnFreeTexture
            delete pLevel->aIDToTexture[i];
            pLevel->aIDToTexture[i] = nullptr;
        }
    }

    free(pLevel->pAptData);
    pLevel->pAptData = nullptr;
    pLevel->bInUse = 0;
}

// ---------------------------------------------------------------------------
// initialization
// ---------------------------------------------------------------------------

void AptAuxSDLGPU_Initialize(AptUserFunctions &funcs, AptGpuRenderer *pRenderer, AptAuxFontList *pFonts)
{
    gpRenderer = pRenderer;

    funcs.pfnMemAlloc = auxMalloc;
    funcs.pfnMemFree = free;
    funcs.pfnLoadAnimation = loadAnimation;
    funcs.pfnLoadAnimationCompleted = loadAnimationCompleted;
    funcs.pfnFreeConstantTable = freeConstantTable;
    funcs.pfnFreeAnimation = freeAnimation;
    funcs.pfnAssertFail = auxAssert;
    funcs.pfnAssertFailMsg = auxAssertMsg;
    funcs.pfnCommand = command;
    funcs.pfnSetExternVariable = command;
    funcs.pfnGetExternVariable = getVariableDefault;
    funcs.pfnLoadVariables = loadVariables;
    funcs.pfnLoadVariablesNULL = loadVariablesNULL;
    funcs.pfnSendVariables = sendVariables;
    funcs.pfnGetBytesTotal = getBytesTotal;
    funcs.pfnGetBytesLoaded = getBytesLoaded;
    funcs.pfnDebugPrint = debugPrint;
    funcs.pfnDebugSetScreenGrabPending = debugSetScreenGrabPending;
    funcs.pfnDebugAddSavedInput = debugAddSavedInput;
    funcs.pfnLoadTexture = loadTexture;
    funcs.pfnFreeTexture = freeTexture;
    funcs.pfnBindTexture = bindTexture;
    funcs.pfnDrawImage = drawImage;
#if defined(APT_USE_SOUND_OBJECT)
    funcs.pfnLoadSound = loadSound;
    funcs.pfnFreeSound = freeSound;
    funcs.pfnStartSound = startSound;
    funcs.pfnStartSoundStream = startSoundStream;
#endif
    funcs.pfnLoadRenderingUnit = loadRenderingUnit;
    funcs.pfnDrawRenderingUnit = drawRenderingUnit;
    funcs.pfnFreeRenderingUnit = freeRenderingUnit;
    funcs.pfnSetColourTransform = setColourTransform;
    funcs.pfnSetVertexMatrix = setVertexMatrix;
    funcs.pfnSetBackgroundColour = setBackgroundColour;
    funcs.pfnCustomControlRender = customControlRender;
    funcs.pfnCustomControlUpdate = customControlUpdate;
    funcs.pfnAllocateString = allocateString;
    funcs.pfnDrawString = drawString;
    funcs.pfnDeallocateString = deallocateString;
    funcs.pfnPointHitTest = NULL;
    funcs.pfnGetRealTimeClock = getRealTimeClock;
    funcs.pfnGetStageWidth = getStageWidth;
    funcs.pfnGetStageHeight = getStageHeight;
    funcs.pfnCreateEffect = createEffect;
    funcs.pfnDestroyEffect = destroyEffect;
    funcs.pfnPushEffect = pushEffect;
    funcs.pfnPopEffect = popEffect;

    gpFonts = pFonts;
    gFreeType.init(gpFonts->nFonts, (void **)gpFonts->pData, (int *)gpFonts->nDataSize);
    for (int i = 0; i < gpFonts->nFonts; i++)
    {
        pFonts->aszFontNamesEnglish[i] = (unsigned short *)funcs.pfnMemAlloc(256);
        pFonts->aszFontNamesJapanese[i] = (unsigned short *)funcs.pfnMemAlloc(256);
        gFreeType.getName(i, pFonts->aszFontNamesEnglish[i], 1033);
        gFreeType.getName(i, pFonts->aszFontNamesJapanese[i], 1041);
    }

    memset(gafVertexMatrix, 0, sizeof(gafVertexMatrix));
    gafVertexMatrix[0] = gafVertexMatrix[5] = gafVertexMatrix[10] = gafVertexMatrix[15] = 1.f;
    gnActiveMasks = 0;
}

void AptAuxSDLGPU_Shutdown()
{
    gFreeType.purge();
    if (gpSavedInputsFile)
    {
        fclose(gpSavedInputsFile);
        gpSavedInputsFile = nullptr;
    }
}
