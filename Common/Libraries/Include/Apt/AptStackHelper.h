/** Helper class exposing functions to manipulate the AptActionInterpreter stack. */

#pragma once

class AptValue;

class AptStackHelper
{
  public:
    static AptValue *GetStackAt(const int index);
};
