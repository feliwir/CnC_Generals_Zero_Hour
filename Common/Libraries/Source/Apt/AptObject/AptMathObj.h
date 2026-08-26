#pragma once

#include "AptObject/AptObject.h"

class AptMathObj : public AptObject
{
  public:
#if APT_USE_MATH_OBJECT
    APT_VALUE_GC_NEW_DELETE_OPERATORS

    AptMathObj() : AptObject(AptVFT_Math, APT_OBJECTHASHSIZE)
    {
        setGCRoot(1);
    }

    virtual AptValue *objectMemberLookup(AptValue *const pContext, const AptNativeString *const pName) const;

    NATIVE_MEMBER_FUNCTION_DECL(sin);
    NATIVE_MEMBER_FUNCTION_DECL(cos);
    NATIVE_MEMBER_FUNCTION_DECL(atan2);
    NATIVE_MEMBER_FUNCTION_DECL(round);
    NATIVE_MEMBER_FUNCTION_DECL(min);
    NATIVE_MEMBER_FUNCTION_DECL(max);

    NATIVE_MEMBER_FUNCTION_DECL(abs);
    NATIVE_MEMBER_FUNCTION_DECL(acos);
    NATIVE_MEMBER_FUNCTION_DECL(asin);
    NATIVE_MEMBER_FUNCTION_DECL(atan);
    NATIVE_MEMBER_FUNCTION_DECL(ceil);
    NATIVE_MEMBER_FUNCTION_DECL(exp);
    NATIVE_MEMBER_FUNCTION_DECL(floor);
    NATIVE_MEMBER_FUNCTION_DECL(log);
    NATIVE_MEMBER_FUNCTION_DECL(pow);
    NATIVE_MEMBER_FUNCTION_DECL(random);
    NATIVE_MEMBER_FUNCTION_DECL(sqrt);
    NATIVE_MEMBER_FUNCTION_DECL(tan);
#else
    AptMathObj() : AptObject(AptVFT_Math, APT_OBJECTHASHSIZE)
    {
        APT_ASSERT(0 && "APT-Warning-Math object cannot be used as it is compiled out, make sure to compile with APT_USE_MATH_OBJECT defined as 1");
    }
#endif

    static void CleanNativeFunctions();

  protected:
    APT_INLINE
    virtual ~AptMathObj()
    {
        //  Do nothing...
    }

  private:
};
