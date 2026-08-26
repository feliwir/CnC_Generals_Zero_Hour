#pragma once

/**
    This file defines a the actionscript "key" object handlers.
*/

#include "AptObject/AptObject.h"

class AptKey : public AptObject
{
  public:
    APT_VALUE_GC_NEW_DELETE_OPERATORS

    AptKey() : AptObject(AptVFT_Key, APT_OBJECTHASHSIZE)
    {
        setGCRoot(1); // This is a global object
    }

    NATIVE_MEMBER_FUNCTION_DECL(isDown);
    NATIVE_MEMBER_FUNCTION_DECL(isToggled);

    NATIVE_MEMBER_FUNCTION_DECL(getCode);
    NATIVE_MEMBER_FUNCTION_DECL(getController);
    NATIVE_MEMBER_FUNCTION_DECL(addListener);
    NATIVE_MEMBER_FUNCTION_DECL(removeListener);
    NATIVE_MEMBER_FUNCTION_DECL(getAnalogStickInfo);
    NATIVE_MEMBER_FUNCTION_DECL(getAnalogTriggerInfo);
    NATIVE_MEMBER_FUNCTION_DECL(getGestureInfo);
    NATIVE_MEMBER_FUNCTION_DECL(getAscii);

    static void CleanNativeFunctions();

    virtual AptValue *objectMemberLookup(AptValue *const pContext, const AptNativeString *const pName) const;

  protected:
    APT_INLINE virtual ~AptKey()
    {
        //  Do nothing...
    }
};

#if defined(APT_ALTERNATE_INPUT)
class AptAlternateInput : public AptObject
{

  public:
    APT_VALUE_GC_NEW_DELETE_OPERATORS

    AptAlternateInput() : AptObject(AptVFT_AltInput, APT_OBJECTHASHSIZE)
    {
        setGCRoot(1);
    }

    NATIVE_MEMBER_FUNCTION_DECL(addListener);
    NATIVE_MEMBER_FUNCTION_DECL(removeListener);

    static void CleanNativeFunctions();

    virtual AptValue *objectMemberLookup(AptValue *const pContext, const AptNativeString *const pName) const;

  protected:
    APT_INLINE virtual ~AptAlternateInput()
    {
        //  Do nothing...
    }

  private:
};
#endif
