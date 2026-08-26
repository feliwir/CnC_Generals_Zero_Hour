#pragma once
#include "AptValue/AptBoolean.h"

/** @return the boolean value held by this object. */
bool AptBoolean::GetBool() const
{
    return (mbValue);
}
