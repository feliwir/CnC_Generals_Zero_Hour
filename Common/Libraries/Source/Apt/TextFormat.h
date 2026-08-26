#pragma once

/*** Include files ********************************************************************************/

/*** Defines **************************************************************************************/

/*** Macros ***************************************************************************************/

/*** Type Definitions *****************************************************************************/

struct TextFormat
{
    static const int UNDEFINED_LEADING_VALUE  = 0x7fffff;
    static const int UNDEFINED_TRACKING_VALUE = 0x7fffff;
    APT_NEW_DELETE_OPERATORS
    // Added Metric defines

        AptNativeString pFontName;
    float fSize;
    int nColor;
    AptStringAlignment eAlignment;
    unsigned int nFontStyle; // Adding extra data member eFontStyle for Font Style, Indent, LeftMargin, RightMargin
    int nIndent;
    int nLeftMargin;
    int nRightMargin;
    int nLeading;
    int nTracking;

    // Adding extra parameter for Font Style, Indent, LeftMargin, RightMargin
    TextFormat(AptValue *pFName, float fFonstSize, unsigned int nFontColor, int isBold, int isItalic, int isUnderline, int nUrl, int nTarget,
               AptValue *pStringAlignment, int nLMargin, int nRMargin, int nIndentation, int nLeadingArgument, int nTrackingArgument);
    TextFormat(const TextFormat *pNewTextObj);

    void copyTextFormatObj(const TextFormat *pNewTextObj);
};

/*** Variables ************************************************************************************/

/*** Functions ************************************************************************************/

