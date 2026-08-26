#pragma once
#include "AptDefine.h"
#include "Apt.h"
#include "_AptInternalDefines.h"

class AptValue;

class AptValueVector
{
  public:
    AptValueVector(const int32_t iSize);
    ~AptValueVector();

    void PushValue(AptValue *const pValue);
    APT_INLINE
    AptValue *PopValue();
    APT_INLINE
    int32_t GetNumValues() const;

#if defined(APT_DEBUG) || defined(APT_ENABLE_ZOMBIE_OUTPUT)
    APT_INLINE
    int32_t GetHighWaterValues() const;
#endif

    void ReleaseValues();
    APT_INLINE
    int32_t IsVectorFull() const;
    APT_INLINE
    AptValue *GetAt(const int32_t iPos)
    {

        APT_ASSERT(iPos >= 0 && iPos < mCurrentNum);
        return mpValues[iPos];
    }
    APT_INLINE
    void SetAt(const int32_t iPos, AptValue *pVal)
    {

        APT_ASSERT(iPos >= 0 && iPos < mCurrentNum);
        mpValues[iPos] = pVal;
    }
    APT_INLINE
    void RemoveAt(const int32_t iPos)
    {

        APT_ASSERT(iPos >= 0 && iPos < mCurrentNum);
        mCurrentNum--;
        if (mCurrentNum && iPos != mCurrentNum)
        {
            memmove(&mpValues[iPos], &mpValues[iPos + 1], sizeof(AptValue *) * (mCurrentNum - iPos));
        }
        mpValues[mCurrentNum] = NULL;
    }

  private:
    AptValueVector();
    explicit AptValueVector(const AptValueVector &Other);
    AptValueVector &operator=(const AptValueVector &Other);

    int32_t mCapacity;
    int32_t mCurrentNum;
    AptValue **mpValues;
#if defined(APT_DEBUG) || defined(APT_ENABLE_ZOMBIE_OUTPUT)
    int32_t mHighWaterNum;
#endif

    APT_NEW_DELETE_OPERATORS
};
