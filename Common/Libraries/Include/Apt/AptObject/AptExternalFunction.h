/******************************************************************************/
/**
    Copyright 2012 Electronic Arts Inc.

    @file       AptExternalFunction.h

    @brief      Function class, extendable outside of Apt
*/
/******************************************************************************/

/*** Include guard ************************************************************/

#pragma once

/*** Includes *****************************************************************/

#include "AptObject/AptObject.h"

/*** Forward Declarations *****************************************************/

/*** Macros *******************************************************************/

/*** Namespaces ***************************************************************/

/*** Interfaces ***************************************************************/

class AptExternalFunction : public AptObject
{
  public:
    APT_VALUE_GC_NEW_DELETE_OPERATORS

    AptExternalFunction(uint32_t classSize);
    uint32_t GetClassSize() const;

    virtual AptValue *Call(int stackParams) = 0;

  protected:
    virtual ~AptExternalFunction();

  private:
    uint32_t mClassSize;
};

/*** Implementation ***********************************************************/

/******************************************************************************/

