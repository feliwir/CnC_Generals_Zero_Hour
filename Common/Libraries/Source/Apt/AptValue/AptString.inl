#include "AptString.h"

APT_INLINE bool AptString::IsConstantString()
{
    return str.IsConstantString();
}
