#pragma once

/**
    This file defines a the actionscript "Mouse" object handlers.
*/

#include "AptObject/AptObject.h"

// Made the class itself visible for random purposes, but conditionally
// removed its contents so that it doesn't take up any space in the final build.
class AptMouse : public AptObject
{

#if defined(APT_USE_MOUSE)

  public:
    APT_VALUE_GC_NEW_DELETE_OPERATORS

    AptMouse() : AptObject(AptVFT_Mouse, APT_OBJECTHASHSIZE)
    {
        setGCRoot(1);
    }

    NATIVE_MEMBER_FUNCTION_DECL(addListener);
    NATIVE_MEMBER_FUNCTION_DECL(removeListener);

    static void CleanNativeFunctions();

    virtual AptValue *objectMemberLookup(AptValue *const pContext, const AptNativeString *const pName) const;

  protected:
    APT_INLINE
    virtual ~AptMouse()
    {
        //  Do nothing...
    }

  private:
#else
    AptMouse() : AptObject(AptVFT_Mouse, APT_OBJECTHASHSIZE)
    {
        APT_ASSERT(false && "Apt Mouse is not built into this library. To use the mouse, please compile with APT_USE_MOUSE defined");
    }
#endif
};
