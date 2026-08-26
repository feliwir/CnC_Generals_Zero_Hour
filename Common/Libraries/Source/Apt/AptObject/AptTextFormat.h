#pragma once

#include "AptObject/AptObject.h"
#include "TextFormat.h"

class AptTextFormat : public AptObject
{
  public:
    APT_VALUE_GC_NEW_DELETE_OPERATORS
    // Added Metric defines

    AptTextFormat(AptValue *pFName, float fFonstSize, unsigned int nFontColor, int isBold, int isItalic, int isUnderline, int nUrl, int nTarget, AptValue *pStringAlignment, int nLMargin, int nRMargin, int nIndentation, int nLeading, int nTracking)
        : AptObject(AptVFT_TextFormat),
          mTextFormat(pFName, fFonstSize, nFontColor, isBold, isItalic, isUnderline, nUrl, nTarget, pStringAlignment, nLMargin, nRMargin, nIndentation, nLeading, nTracking)
    {
        // Do nothing...
    }
    AptTextFormat(const TextFormat *pNewTextObj)
        : AptObject(AptVFT_TextFormat),
          mTextFormat(pNewTextObj)
    {
        // Do nothing...
    }

    virtual AptValue *objectMemberLookup(AptValue *const pContext, const AptNativeString *const pName) const;
    virtual bool objectMemberSet(AptValue *const pContext, const AptNativeString *const pName, AptValue *const pValue);

    TextFormat *GetTextFormat()
    {
        return &mTextFormat;
    }

  protected:
    APT_INLINE
    virtual ~AptTextFormat()
    {
        //  Do nothing...
    }

    TextFormat mTextFormat;
};

/*H*************************************************************************************************
    EOF
*************************************************************************************************H*/
