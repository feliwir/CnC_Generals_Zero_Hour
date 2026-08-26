#pragma once

typedef unsigned short uint16;

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_SFNT_NAMES_H

class CFreeType
{
public:
    CFreeType();
    ~CFreeType();

    typedef void *(*pfnLoadFile)(const char *szName, int *pnSize);
    typedef void (*pfnUnloadFile)(void *pData);

    void init(int nFonts, void **pData, int  *pSize);
    void purge();

    void getName(int nFont, unsigned short *szString, int nLanguageID);
    void setStrokeFontScale(int nFont, float fScale); // fScale == [0..1]

    enum Alignment { Left, Right, Centre};

    // $FR#598 CSB - Adding extra parameter for Font Style 3/36
    int renderString(   int nFont,
                        const uint16 *wszString,
                        int nPointSize,
                        int nTextureWidth,
                        int nTextureHeight,
                        unsigned char *pData,
                        unsigned int *pnTextWidth,
                        unsigned int *pnTextHeight,
                        float *pfYAscender,
                        int nBorderSize,
                        int nX,
                        int nY,
                        int bWrap,
                        unsigned int nFontStyle,
                        int nBoxWidth,
                        int nBoxHeight,
                        Alignment eAlignment,
                        int nLineOffset );

    // $FR#598 CSB - Adding extra parameter for Font Style 4/36
    void calcRenderStringSize(int nFont, uint16 *wszString, int nPointSize, int& nMaxWidth, int& nMaxHeight, int nBorderSize = 0, int nX = 0, int nY = 0, int bWrap = 0, unsigned int nFontStyle = 0, int nBoxWidth = 0,
                      Alignment eAlignment = Left, int nLineOffset = 0);
    void flushCache();

    const uint16 * const AnalyzeLine( const uint16 *wszLineStart, bool bWrapAtBounding, int nWrapWidth, int *nOutLineChars, int *nOutTextWidth, short anKerning[], int nFont);
    int HAK_MeasureTextInX(int nFont, const unsigned short *text, short *xKernValuesInFUnits, int numChars );
    int HAK_MeasureTextInY(int nFont, const unsigned short *text, int xYBase, int numChars );

private:
    CFreeType(CFreeType &x) { (void) x; }
    void operator=(CFreeType &x) { (void) x; }
    FT_Library mLibrary;
    int bInitialized;
    int nFonts;
    int m_nLastFont ;       // added for not calling FF_FM_selectFont.
    // $FR#598 CSB - Adding global data member to check for the Last Font Style 5/36
    unsigned int nLastFontStyle;

    struct fontData
    {
        bool bUsed;
        FT_Face mFace;
        unsigned char *pData;
        int nCode;
    };

#define FT_MAX_FONTS 16
    fontData fonts[FT_MAX_FONTS];
    float afScales[FT_MAX_FONTS];
};

extern CFreeType FT;
