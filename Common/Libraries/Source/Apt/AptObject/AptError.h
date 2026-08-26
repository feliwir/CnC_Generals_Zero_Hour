/**
    This file defines a the actionscript "Error" object handlers.
*/

#pragma once

#include "AptObject/AptObject.h"

/**
 * @brief Implements Native Error Class in Actionscript.
 * Added for Flash Player 7 / AS 2.0 support. (Release 17.0)
 */
class AptError : public AptObject
{
  public:
    APT_VALUE_GC_NEW_DELETE_OPERATORS
    AptNativeString msMessage;
    AptNativeString msName;

    AptError() : AptObject(AptVFT_Error), msMessage("Error"), msName("Error")
    {
    }

    AptError(AptNativeString sMessage) : AptObject(AptVFT_Error), msMessage(sMessage), msName("Error")
    {
    }

    NATIVE_MEMBER_FUNCTION_DECL(toString);

    virtual AptValue *objectMemberLookup(AptValue *const pContext, const AptNativeString *const pName) const;
    virtual bool objectMemberSet(AptValue *const pContext, const AptNativeString *const pName, AptValue *const pValue);

    static void CleanNativeFunctions();

  protected:
    virtual ~AptError();
};
