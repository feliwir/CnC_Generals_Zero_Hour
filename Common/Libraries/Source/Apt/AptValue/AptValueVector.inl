#pragma once
#include "AptValue/AptValueVector.h"

/** Pops and returns the top value, decrementing the count. */
AptValue *AptValueVector::PopValue()
{
    APT_ASSERT(mCurrentNum > 0);
    return (mpValues[--mCurrentNum]);
}

/** Returns the number of values currently in the vector. */
int32_t AptValueVector::GetNumValues() const
{
    return mCurrentNum;
}

/** Returns whether the vector is full. */
int32_t AptValueVector::IsVectorFull() const
{
    if (mCurrentNum >= mCapacity)
    {
        return (true);
    }
    return (false);
}

#if defined(APT_DEBUG) || defined(APT_ENABLE_ZOMBIE_OUTPUT)
/** Returns the highest number of values the vector has held at once. */
int32_t AptValueVector::GetHighWaterValues() const
{
    return mHighWaterNum;
}
#endif
