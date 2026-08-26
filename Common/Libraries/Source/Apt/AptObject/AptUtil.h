/**
    This provides common utilities functions previously defined in ActionScript (by ION/Central UI).
    Moved to C++ to improve ActionScript execution performance.
*/

#pragma once

#include "AptObject/AptObject.h"

class AptUtil : public AptObject
{
  public:
#if APT_USE_UTILITY
    APT_VALUE_GC_NEW_DELETE_OPERATORS
    // Added Metric defines

    AptUtil() : AptObject(AptVFT_AptUtil, APT_OBJECTHASHSIZE)
    {
        setGCRoot(1);
    }

    virtual AptValue *objectMemberLookup(AptValue *const pContext, const AptNativeString *const pName) const;

    NATIVE_MEMBER_FUNCTION_DECL(formatNumberString);
    NATIVE_MEMBER_FUNCTION_DECL(replaceString);
    NATIVE_MEMBER_FUNCTION_DECL(trimString);
    NATIVE_MEMBER_FUNCTION_DECL(trimLeftString);
    NATIVE_MEMBER_FUNCTION_DECL(trimRightString);
    NATIVE_MEMBER_FUNCTION_DECL(searchArray);
    NATIVE_MEMBER_FUNCTION_DECL(reverseSearchArray);
    NATIVE_MEMBER_FUNCTION_DECL(countArray);
    NATIVE_MEMBER_FUNCTION_DECL(getAptVersion);
    NATIVE_MEMBER_FUNCTION_DECL(safeForIn);
    NATIVE_MEMBER_FUNCTION_DECL(convertHsvToColorTransform);
    NATIVE_MEMBER_FUNCTION_DECL(colorMatrixMultiply);

#else
    AptUtil() : AptObject(AptVFT_AptUtil, APT_OBJECTHASHSIZE)
    {
        APT_ASSERT(0 && "APT-Warning-AptUtil object cannot be used as it is compiled out, make sure to compile with APT_USE_UTILITY defined as 1");
    }
#endif

    static void CleanNativeFunctions();

  protected:
    virtual ~AptUtil()
    {
        //  Do nothing...
    }

  private:
    enum Trim
    {
        LEFT,
        RIGHT,
        BOTH
    };

    static AptValue *trim(Trim type, int32_t paramCount);
    static AptValue *search(bool reverse, int32_t paramCount);
};
