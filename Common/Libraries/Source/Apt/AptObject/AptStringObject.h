/**
 * Wraps AptString in an AptObject so it can be used as a script object.
 */

#pragma once

#include "AptValue/AptValue.h"
#include "AptValue/AptString.h"
#include "AptObject/AptObject.h"

class AptStringObject : public AptObject
{
  public:
    APT_VALUE_GC_NEW_DELETE_OPERATORS

    AptStringObject() : AptObject(AptVFT_StringObject), mpStringObject(NULL)
    {
    }
    AptStringObject(AptString *pString) : AptObject(AptVFT_StringObject), mpStringObject(NULL)
    {
        setString(pString);
    }

    ~AptStringObject();
    void setString(AptString *pString);

    virtual AptValue *objectMemberLookup(AptValue *const pContext, const AptNativeString *const pName) const;
    AptString *mpStringObject;
};
