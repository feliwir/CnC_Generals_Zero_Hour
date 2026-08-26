/*H************************************************************************************************/
/**
 * @file ftapt.cpp
 *
 * @brief FreeType-based text layout and rendering backend used by Apt Aux to render fonts. This
 * replaces the original Font Fusion backend (formerly ffapt.cpp); some functions still mirror its
 * layout behavior on purpose for output parity, as noted where relevant.
 *
 * @copyright (c) 2025 Stephan Vedder
 *
 * @version 2.0     2025 (SV) Rewritten on top of FreeType, replacing the original Font Fusion backend.
 */
/************************************************************************************************H*/
#include "ftapt.h"
#include "Apt.h"

#include "math.h"

   float gAptFFAdditionalScaleFactorX = 1.f;

//
// (1.2:rrv:1/1)
// --- these values may need to be adjusted depending on the font being used
// ---  fonts differences affect the scaling, so a balance needs to be struck
// ---  between the amount of memory needed and the resolution achieved.
float gAptFFScaleFactorX    = 0.7071f;
float gAptFFScaleFactorY    = 0.7071f;
int gAptFFScaleAtThisPtSize = 15;
//

CFreeType FT;

// $FR#598 CSB - Setting data member Last Font Style 27/36
CFreeType::CFreeType() : bInitialized(0), nFonts(0), m_nLastFont(-1), nLastFontStyle(0xFFFFFFFF) {}

/*F************************************************************************************************/
/**
 * @brief Flush the font cache (duh!)
 */
/************************************************************************************************F*/
void CFreeType::flushCache()
{
    int nErr = 0;
    APT_ASSERT(!nErr);
}


/*F************************************************************************************************/
/**
 * @brief initialize FreeType.
 *
 * @param nFonts number of fonts
 * @param pData Array of buffers, one for each font
 * @param pSize Array of buffer sizes, one for each font.
 */
/************************************************************************************************F*/
void CFreeType::init(int nFonts, void **pData, int  *pSize)
{
    APT_ASSERT(!bInitialized);
    FT_Error nError = 0;

    // make a font manager
    nError = FT_Init_FreeType(&mLibrary);
    APT_ASSERT(nError == 0);

    APT_ASSERT(nFonts < FT_MAX_FONTS);
    this->nFonts = nFonts;

    for (int i = 0; i < nFonts; i++)
    {
        // Whip up the required data.
        fontData *pFont = &fonts[i];

        pFont->pData = (unsigned char *)pData[i];
        APT_ASSERT(pFont->pData);

        // Get a new font memory handler
        nError = FT_New_Memory_Face(mLibrary, pFont->pData, pSize[i], 0, &pFont->mFace);
        APT_ASSERT(nError == 0);
    }

    bInitialized = 1;
}


/*F************************************************************************************************/
/**
 * @brief deletes all font data.
 */
/************************************************************************************************F*/
void CFreeType::purge()
{
    APT_ASSERT(bInitialized);
    FT_Error nError = 0;

    // 2004/05/08 -- LH -- Fixed minor issue where m_nLastFont was not cleared on purge
    m_nLastFont = -1;

    // $FR#598 CSB - Setting Last Font Style 28/36
    nLastFontStyle = 0xFFFFFFFF;

    for (int i = 0; i < nFonts; i++)
    {
        fontData *pFont = &fonts[i];
        nError = FT_Done_Face(pFont->mFace);
        APT_ASSERT(!nError);
    }

    nError = FT_Done_FreeType(mLibrary);
    APT_ASSERT(!nError);
    mLibrary = 0;

    bInitialized = 0;
}

CFreeType::~CFreeType()
{
}


/*F************************************************************************************************/
/**
 * @brief This function sets the font scale (makes it bigger or smaller).
 */
/************************************************************************************************F*/
void CFreeType::setStrokeFontScale(int nFont, float fScale)
{
    APT_ASSERT(nFont >= 0 && nFont < nFonts);
    afScales[nFont] = fScale;
}

/*F************************************************************************************************/
/**
 * @brief This function gives you the name of the indexed font.
 *
 * @note for language id, refer to http://support.microsoft.com/?kbid=221435.
 * English US: 1033
 * Japanese: 1041
 */
/************************************************************************************************F*/
void CFreeType::getName(int nFont, unsigned short *szFontName, int nLanguageID)
{
    APT_ASSERT(nFont >= 0 && nFont < nFonts);

    int nError = 0;

    FT_Face face = fonts[nFont].mFace;
    FT_SfntName name;
    FT_Get_Sfnt_Name(face, 0, &name);
    unsigned char *pSrc = name.string;
   
    if (pSrc)
    {
        for (;;)
        {
            if (*pSrc == 0)
            {
                break;
            }
            *szFontName++ = *pSrc++;
        }
    }
    *szFontName = 0;
}

/*F************************************************************************************************/
/**
 * @brief This function calculates the width in pixels of a string that is passed in. This is useful for
 * getting the required size of textures and such.
 *
 * @param nFont Index of desired font in font list
 * @param text string to use
 * @param xKernValuesInFUnits kerning values, unused.
 * @param numChars number of chars to include in the calculation. This must not exceed strlen of string!
 *
 * @return int - Width in pixels the string will need to render in.
 */
/************************************************************************************************F*/
int CFreeType::HAK_MeasureTextInX(int nFont, const unsigned short *text, short *xKernValuesInFUnits, int numChars )
{
    // Mirrors the Font Fusion implementation (ffapt.cpp): the sum of the advances,
    // compensated for a negative left side bearing on the first character, taking
    // the rightmost pixel extent of every glyph. No kerning - Flash doesn't kern.
    (void)xKernValuesInFUnits;
    FT_Face pFace = fonts[nFont].mFace;
    int nPenLocation = 0;
    int nMaxWidth = 0;

    if (numChars < 1)
    {
        return 0;
    }

    if (FT_Load_Char(pFace, text[0], FT_LOAD_DEFAULT) == 0)
    {
        const int nLeft = (int)(pFace->glyph->metrics.horiBearingX >> 6);
        if (nLeft < 0)
        {
            nPenLocation -= nLeft;
        }
    }

    for (int i = 0; i < numChars; i++)
    {
        if (FT_Load_Char(pFace, text[i], FT_LOAD_DEFAULT) != 0)
        {
            continue;
        }
        const int nAdvance = (int)(pFace->glyph->advance.x >> 6);
        const int nLeft = (int)(pFace->glyph->metrics.horiBearingX >> 6);
        const int nWidth = (int)(pFace->glyph->metrics.width >> 6);

        nPenLocation += nAdvance;

        const int nTempMaxWidth = nPenLocation + (nWidth - (nAdvance - nLeft));
        if (nTempMaxWidth > nMaxWidth)
        {
            nMaxWidth = nTempMaxWidth;
        }
    }

    return nMaxWidth;
}

/*F************************************************************************************************/
/**
 * @brief This function finds the largest character in a line so the difference can be added to the rest of the characters at render time.
 * This is needed to align all of the characters to their baseline.
 *
 * @param nFont Index of desired font in font list
 * @param text string to use
 * @param xYBase
 * @param numChars number of chars to include in the calculation. This must not exceed strlen of string!
 *
 * @return int - Y Adjustment value.
 */
/************************************************************************************************F*/
int CFreeType::HAK_MeasureTextInY(int nFont, const unsigned short *text, int xYBase, int numChars )
{
    return 0;
}

/*F************************************************************************************************/
/**
 * @brief This function basically takes in a pointer to the base of a line in a string and returns the number
 * of characters in the line, and the width of the rendered line. Lines are defined by \r \n or
 * optionally by a wrap width (more time consuming). This returns the pointer to the base of the next
 * line in the string. (or a pointer to the null character in the case of the end of a string.)
 *
 * @param wszLineStart pointer to base of line to be analyzed
 * @param bWrapAtBounding true = wrap at bounding box (note, can be time consuming)
 * @param nWrapWidth if wrapping, this is width to wrap at.
 * @param nOutLineChars [OUT] - characters in the current line.
 * @param nOutTextWidth [OUT] - width of rendered line.
 * @param anKerning [IN/OUT] - kerning values, not sure if input or output...
 *
 * @return const uint16 * - pointer to base of next line.
 *
 * @note This can be greatly optimized for wrapping case by not calculating the width of the whole string for
 * every length. To do this I would recommend guessing the length of the string using the wraping width
 * and the average character width (generally defined by the width of the letter 'x' no jokes!).
 */
/************************************************************************************************F*/
// Selects the pixel size and style transform on the face, mirroring the scaling
// hacks of the Font Fusion implementation (ffapt.cpp): fonts at or above
// gAptFFScaleAtThisPtSize points are scaled by gAptFFScaleFactorX/Y, and italic
// is emulated with a shear transform.
static void _ftSelectSize(FT_Face pFace, int nPointSize, unsigned int nFontStyle)
{
    const float fScaleX = nPointSize < gAptFFScaleAtThisPtSize ? 1.0f : gAptFFScaleFactorX;
    const float fScaleY = nPointSize < gAptFFScaleAtThisPtSize ? 1.0f : gAptFFScaleFactorY;
    FT_Set_Pixel_Sizes(pFace,
                       (FT_UInt)(nPointSize * fScaleX * gAptFFAdditionalScaleFactorX + 0.5f),
                       (FT_UInt)(nPointSize * fScaleY + 0.5f));

    FT_Matrix matrix;
    matrix.xx = 0x10000;
    // sin(15.0f) matches ffapt.cpp verbatim (which passed radians despite the
    // "15 degree" comment); keep it for identical output
    matrix.xy = (nFontStyle & AptFontStyle_Italic) ? (FT_Fixed)(sin(15.0f) * 0x10000) : 0;
    matrix.yx = 0;
    matrix.yy = 0x10000;
    FT_Set_Transform(pFace, &matrix, NULL);
}


const uint16 * const CFreeType::AnalyzeLine(const uint16 *wszLineStart,
                                              bool          bWrapAtBounding,
                                              int           nWrapWidth,
                                              int          *nOutLineChars,  /* OUT Parameter */
                                              int          *nOutTextWidth,  /* OUT Parameter */
                                              short         anKerning[],
                                              int           nFont)
{
    // Structure ported from the Font Fusion implementation (ffapt.cpp).
    // Assumes the face's pixel size has been selected by the caller.
    const uint16 *pCurrentChar = wszLineStart;
    uint16 wThisChar;
    int nDeadChars = 0; // chars not rendered at the end of the line

    // Handle the optimal case of no wrapping (this makes things pretty quick).
    if (!bWrapAtBounding)
    {
        while ((wThisChar = *pCurrentChar) != '\0')
        {
            // Handle explicit line separators here.
            if (wThisChar == '\r')
            {
                nDeadChars++;
                if (pCurrentChar[1] == '\n')
                {
                    nDeadChars++;
                }
                break;
            }
            if (wThisChar == '\n')
            {
                nDeadChars++;
                break;
            }
            pCurrentChar++;
        }
        *nOutLineChars = (int)(pCurrentChar - wszLineStart);
        *nOutTextWidth = HAK_MeasureTextInX(nFont, wszLineStart, anKerning, *nOutLineChars);
    }
    else // if wrapping, lots more to do: advance one word at a time until we
    {    // overflow nWrapWidth, then go back to the last word separator
        int nCurrentWidth = 0;  // running length as we go
        int nPrevCharWidth = 0; // width of the line at the end of the previous character
        int nPrevWordWidth = 0; // width of the line at the end of the previous word separator
        const uint16 *pPrevWordSeparator = wszLineStart; // pointer to end of previous word

        while ((wThisChar = *pCurrentChar) != '\0')
        {
            // Handle explicit line separators here.
            if (wThisChar == '\r')
            {
                nDeadChars++;
                if (pCurrentChar[1] == '\n')
                {
                    nDeadChars++;
                }
                break;
            }
            if (wThisChar == '\n')
            {
                nDeadChars++;
                break;
            }

            // We are wrapping: look for word separators and see when we overflow the box.
            if (wThisChar == ' ' || wThisChar == '\t')
            {
                nPrevWordWidth = nCurrentWidth;
                pPrevWordSeparator = pCurrentChar;
            }

            nPrevCharWidth = nCurrentWidth;
            nCurrentWidth = HAK_MeasureTextInX(nFont, wszLineStart, anKerning, (int)(pCurrentChar - wszLineStart) + 1);

            // Note: do NOT equal or exceed the wrap width, or else we overflow our texture.
            if (nCurrentWidth >= nWrapWidth)
            {
                if (pPrevWordSeparator == wszLineStart)
                {
                    if (wszLineStart == pCurrentChar)
                    {
                        // if one letter doesn't fit just let it go
                        pCurrentChar++;
                    }
                    else
                    {
                        // if one word does not fit, wrap in-word
                        nCurrentWidth = nPrevCharWidth;
                    }
                }
                else
                {
                    // we have seen a word separator; it becomes a dead char
                    nDeadChars++;
                    pCurrentChar = pPrevWordSeparator;
                    nCurrentWidth = nPrevWordWidth;
                }
                break;
            }
            pCurrentChar++;
        }
        *nOutLineChars = (int)(pCurrentChar - wszLineStart);
        *nOutTextWidth = nCurrentWidth;
    }

    return wszLineStart + *nOutLineChars + nDeadChars;
}

/*F************************************************************************************************/
/**
 * @brief This monstrosity some or all of a string. This will render only what is visible within the bounding
 * box. We still do some parsing outside of the box inorder to get the total number of lines and such,
 * but they are not rendered. This is not optimized to say the least but is definitely not as wastefull
 * as it has been in the past.
 *
 * @param nFont Font Index
 * @param wszString string to render (in UCS2 format)
 * @param nPointSize Font Size
 * @param nTextureWidth (IN) Width of texture created (>= size of text field)
 * @param nTextureHeight (IN) Height of texture created (>= size of text field)
 * @param pData texture Buffer. (i.e. from AptAuxEAGLREAL_CreateFontTexture())
 * @param pnTextWidth OUT Text Width, can be used for auto size.
 * @param pnTextHeight OUT Text Height, can be used for auto size.
 * @param pfYAscender value we ascend each line I think.
 * @param nBorderSize Size of Border Drawn
 * @param nX starting x coordinate
 * @param nY starting y coordinate
 * @param bWrap does text wrap.
 * @param nBoxWidth Width of bounding box (used for wrap)
 * @param eAlignment alignment types
 * @param nLineOffset line to start rendering (for scrolling)
 *
 * @return int - number of lines that can be scrolled (Max useable nLineOffset value)
 *
 * @note Previous comment states that
 * "we don't do kerning because flash doesn't.. not sure if that's a great thing,
 *  but there you have it"
 * I am not sure if this comment is/was correct.
 * Also, this function has SO MANY arguments! I'm sure there is a better way!
 */
/************************************************************************************************F*/
int CFreeType::renderString( int           nFont,
                               const uint16 *wszString,
                               int           nPointSize,
                               int           nTextureWidth,
                               int           nTextureHeight,
                               unsigned char *pData,
                               unsigned int  *pnTextWidth,
                               unsigned int  *pnTextHeight,
                               float         *pfYAscender,
                               int           nBorderSize,
                               int           nX,
                               int           nY,
                               int           bWrap,
                               // $FR#598 CSB - Adding extra parameter for Font Style 29/36
                               unsigned int  nFontStyle,
                               int           nBoxWidth,
                               int           nBoxHeight,
                               Alignment     eAlignment,
                               int           nLineOffset )
{
    APT_ASSERT(bInitialized);
    APT_ASSERT(nFont >= 0 && nFont < nFonts);
    APT_ASSERT(nPointSize > 0);
    const int MAX_STRING = 512;
    short anKerning[MAX_STRING];

    *pnTextWidth = 0;
    *pnTextHeight = 0;
    if (pfYAscender)
    {
        *pfYAscender = 0.f;
    }

    FT_Face pFace = fonts[nFont].mFace;
    _ftSelectSize(pFace, nPointSize, nFontStyle);

    // font metrics in pixels (FreeType 26.6 fixed point); the line advance
    // corresponds to Font Fusion's ascender - descender + line gap
    const int nAscender = (int)((pFace->size->metrics.ascender + 32) >> 6);
    const int nDescender = (int)(pFace->size->metrics.descender >> 6); // negative
    int nLineAdvance = (int)(pFace->size->metrics.height >> 6);
    if (nLineAdvance <= 0)
    {
        nLineAdvance = nPointSize;
    }
    const int nLineGap = nLineAdvance - (nAscender - nDescender);

    const int nBorderSize2 = nBorderSize * 2;
    const int nWrapWidth = nBoxWidth - nBorderSize2;

    // number of lines that fit the box; used to normalize scroll and maxscroll
    int nLinesRenderedCalc = (nBoxHeight - nBorderSize2) / nLineAdvance;
    if (nLinesRenderedCalc < 1)
    {
        nLinesRenderedCalc = 1;
    }

    const uint16 *pLineOffset = wszString;      // current line
    const uint16 *pNextLineOffset = wszString;  // next line start
    const uint16 *pHoldOffset = wszString;      // scroll position place holder

    int nMaxLineWidth = 0;
    int nLinesRendered = 0;
    int nLinesTotal = 0;
    int nLineChars = 0;
    int nLineWidth = 0;
    bool bFilledBox = false;

    // First pass: count the lines, capture the max line width and intercept the
    // line referenced by the scroll value (structure ported from ffapt.cpp).
    {
        const int HOLDARRAYSZ = 128;
        const uint16 *apHoldOffset[HOLDARRAYSZ] = {};
        int nLnIndex = 0;

        while (pLineOffset[0] != '\0')
        {
            pNextLineOffset = AnalyzeLine(pLineOffset, bWrap != 0, nWrapWidth, &nLineChars, &nLineWidth, anKerning, nFont);
            if (nLineWidth > nMaxLineWidth)
            {
                nMaxLineWidth = nLineWidth;
            }
            nLinesTotal++;
            // if we are at the target line offset, save its pointer (scroll is 1-based)
            if (nLinesTotal == nLineOffset)
            {
                pHoldOffset = pLineOffset;
            }
            // round-robin buffer of the last HOLDARRAYSZ line starts
            apHoldOffset[nLnIndex++] = pLineOffset;
            if (nLnIndex == HOLDARRAYSZ)
            {
                nLnIndex = 0;
            }
            if (pNextLineOffset == pLineOffset)
            {
                break; // safety against zero progress
            }
            pLineOffset = pNextLineOffset;
        }

        int nMaxScrollCalc = nLinesTotal - nLinesRenderedCalc + 1;
        if (nMaxScrollCalc < 1)
        {
            nMaxScrollCalc = 1;
        }

        // clamp the scroll so the last page stays filled
        if (nLineOffset > nMaxScrollCalc)
        {
            int nuix = nLnIndex - ((nLinesRenderedCalc < nLinesTotal) ? nLinesRenderedCalc : nLinesTotal);
            if (nuix < 0)
            {
                nuix += HOLDARRAYSZ;
            }
            if (nuix >= 0 && apHoldOffset[nuix])
            {
                pHoldOffset = apHoldOffset[nuix];
            }
        }
    }

    // reposition to the scroll position
    pLineOffset = pHoldOffset;

    // Second pass: render the lines that fit into the box.
    int nBaseY = nAscender + nBorderSize + nLineGap + nY;
    while (pLineOffset[0] != '\0' && !bFilledBox)
    {
        pNextLineOffset = AnalyzeLine(pLineOffset, bWrap != 0, nWrapWidth, &nLineChars, &nLineWidth, anKerning, nFont);

        if (bWrap && (pNextLineOffset - pLineOffset) == 1 && nLineWidth > (nBoxWidth + nBorderSize2 + 1))
        {
            // couldn't even fit one char; well, don't cry over spilled milk, move along
            break;
        }
        APT_ASSERT(nLineChars < MAX_STRING);

        int nAlignmentOffset = 0;
        if (eAlignment == CFreeType::Right)
        {
            nAlignmentOffset = nBoxWidth - nBorderSize2 - nLineWidth;
        }
        else if (eAlignment == CFreeType::Centre)
        {
            nAlignmentOffset = (nBoxWidth - nBorderSize2 - nLineWidth) / 2;
        }
        if (nAlignmentOffset < 0)
        {
            nAlignmentOffset = 0;
        }

        int nPenX = nBorderSize + nX + nAlignmentOffset;
        // compensate a negative left side bearing on the first character, like HAK_MeasureTextInX
        if (nLineChars > 0 && FT_Load_Char(pFace, pLineOffset[0], FT_LOAD_DEFAULT) == 0)
        {
            const int nLeft = (int)(pFace->glyph->metrics.horiBearingX >> 6);
            if (nLeft < 0)
            {
                nPenX -= nLeft;
            }
        }

        for (int nIndex = 0; nIndex < nLineChars; nIndex++)
        {
            if (FT_Load_Char(pFace, pLineOffset[nIndex], FT_LOAD_RENDER) != 0)
            {
                continue;
            }

            const FT_Bitmap *pBitmap = &pFace->glyph->bitmap;
            const int nDstX = nPenX + pFace->glyph->bitmap_left;
            const int nDstY = nBaseY - pFace->glyph->bitmap_top;
            for (unsigned int nRow = 0; nRow < pBitmap->rows; nRow++)
            {
                const int y = nDstY + (int)nRow;
                if (y < 0 || y >= nTextureHeight)
                {
                    continue;
                }
                const unsigned char *pSrc = pBitmap->buffer + nRow * pBitmap->pitch;
                unsigned char *pDst = pData + y * nTextureWidth;
                for (unsigned int nCol = 0; nCol < pBitmap->width; nCol++)
                {
                    const int x = nDstX + (int)nCol;
                    if (x < 0 || x >= nTextureWidth || x >= nBoxWidth)
                    {
                        continue;
                    }
                    // |= instead of a plain store, so overlapping glyphs don't chop
                    // each other off (see $bug491 in ffapt.cpp)
                    pDst[x] |= pSrc[nCol];
                }
            }

            nPenX += (int)(pFace->glyph->advance.x >> 6);
        }

        nLinesRendered++;
        nBaseY += nLineAdvance;
        pLineOffset = pNextLineOffset;

        // If the next line takes us out of the box, stop. The final line does not
        // need a line gap and should not include the descender.
        if (nBaseY + nAscender - nLineGap > nBoxHeight + nBorderSize2)
        {
            bFilledBox = true;
        }
    }

    // The text height property counts all lines, even when not all are rendered.
    nBaseY += (nLinesTotal - nLinesRendered) * nLineAdvance;
    nBaseY += nDescender - nLineGap;

    *pnTextWidth = (unsigned int)nMaxLineWidth;
    *pnTextHeight = (unsigned int)(nBaseY - (nAscender + nLineGap));

    if (pfYAscender)
    {
        *pfYAscender = (float)(nAscender + nBorderSize);
    }

    // +1 because MaxScroll is a base-1 number
    int nReturnMaxScroll = nLinesTotal - nLinesRendered + 1;
    if (nReturnMaxScroll < 1)
    {
        nReturnMaxScroll = 1;
    }
    return nReturnMaxScroll;

}   // end of CFreeType::renderString


/*F************************************************************************************************/
/**
 * @brief returns (via passed int references) the max size of the string when rendered. This can be used to
 * ensure a texture large enough is created. The size this returns does NOT include any borders!!!
 * borders are added by the caller.
 *
 * @param nFont Font Index
 * @param wszString string to render (in UCS2 format)
 * @param nPointSize Font Size
 * @param nMaxWidth (OUT) Width of texture created (>= size of text field)
 * @param nMaxHeight (OUT) Height of texture created (>= size of text field)
 * @param nBorderSize [Not used!!! border sizes are added by caller!]
 * @param nX starting x coordinate
 * @param nY starting y coordinate
 * @param bWrap does text wrap.
 * @param nBoxWidth Width of bounding box (used for wrap)
 * @param eAlignment alignment types
 * @param nLineOffset line to start rendering (for scrolling)
 *
 * @return int nMaxWidth - (OUT) Width of texture created (>= size of text field)
 * @return int nMaxHeight - (OUT) Height of texture created (>= size of text field)
 *
 * @note this function has SO MANY arguments! I'm sure there is a better way!
 */
/************************************************************************************************F*/
void CFreeType::calcRenderStringSize(int       nFont,
                                       uint16   *wszString,
                                       int       nPointSize,
                                       int      &nMaxWidth,     /* OUT Parameter! */
                                       int      &nMaxHeight,    /* OUT Parameter! */
                                       int       nBorderSize,
                                       int       nX,
                                       int       nY,
                                       int       bWrap,
                                       // $FR#598 CSB - Adding extra parameter for Font Style 33/36
                                       unsigned int nFontStyle,
                                       int       nBoxWidth,
                                       Alignment eAlignment,
                                       int       nLineOffset )
{
    APT_ASSERT(bInitialized);
    APT_ASSERT(nFont >= 0 && nFont < nFonts);
    APT_ASSERT(nPointSize > 0);
    const unsigned int MAX_STRING = 512;
    short anKerning[MAX_STRING];

    nMaxWidth = 0;
    nMaxHeight = 0;

    FT_Face pFace = fonts[nFont].mFace;
    _ftSelectSize(pFace, nPointSize, nFontStyle);

    const int nAscender = (int)((pFace->size->metrics.ascender + 32) >> 6);
    const int nDescender = (int)(pFace->size->metrics.descender >> 6); // negative
    int nLineAdvance = (int)(pFace->size->metrics.height >> 6);
    if (nLineAdvance <= 0)
    {
        nLineAdvance = nPointSize;
    }
    const int nLineGap = nLineAdvance - (nAscender - nDescender);

    const int nWrapWidth = nBoxWidth - (nBorderSize * 2);
    int nMaxLineLength = 0;
    int nLineChars = 0;
    int nLineWidth = 0;

    // height accumulation mirrors ffapt.cpp: start at ascender + line gap,
    // advance per line, then remove the trailing gap and add the descender
    int nHeight = nAscender + nLineGap + nY;

    const uint16 *pLineOffset = wszString;
    while (pLineOffset[0] != '\0')
    {
        const uint16 *pNextLineOffset = AnalyzeLine(pLineOffset, bWrap != 0, nWrapWidth, &nLineChars, &nLineWidth, anKerning, nFont);
        if (nLineWidth > nMaxLineLength)
        {
            nMaxLineLength = nLineWidth;
        }
        nHeight += nLineAdvance;
        if (pNextLineOffset == pLineOffset)
        {
            break; // safety against zero progress
        }
        pLineOffset = pNextLineOffset;
    }

    nHeight += nDescender;
    nHeight -= nLineGap;

    nMaxHeight = nHeight;
    nMaxWidth = nMaxLineLength + (nBorderSize * 2) + 1 /* Extra Pixel so that border is not drawn on top */;

}

