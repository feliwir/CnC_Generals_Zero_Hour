/**
 * Generates various AptValues (String, Integer, Float, Boolean, Array, formatted String), which
 * are garbage-collected.
 */

#pragma once

#include "AptValue/AptValue.h"

class AptValueFactory
{
  public:
    static AptValue *CreateString(const char *szValue);
    static AptValue *CreateInteger(int nValue);
    static AptValue *CreateFloat(float fValue);
    static AptValue *CreateBoolean(bool bValue);
    static AptValue *CreateArray(int nElements, AptValue **pAptValue);
    static AptValue *CreateArray();
    static AptValue *CreateObject();
    static AptValue *CreateStringFormatted(const char *szFormat, ...);
    static AptValue *CreateUndefined();
};
