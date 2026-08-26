/**
 * In Apt 0.15.x, AptValueVector is introduced in implementation of actioninterpreter. Previously every action tag use to push/pop elements and while doing that it used to reference count those objects to decide their life time. With new vector, instead of popping and decrementing the reference count on AptValue, it just pops from actioninterpreter.stack and pushes it to this DeferedVector. At the end of script function, or end of actionscript in a frame, or some other occasions, Apt decrements reference count on all these objects that were pushed in DeferedVector, and then in one shot, deletes them whose reference count reaches zero, this improves the speed of deletion a lot because of less cache misses.
 */

#include "Apt.h"
#include "AptValue/AptValueVector.h"
#include "AptValue/AptValue.h"
#include "_AptInternalDefines.h"
#include "MainInline.h"

#if !defined(APT_ENABLE_INLINE)
#include "AptValue/AptValueVector.inl"
#endif


/**
 * @param iSize @note
 */
AptValueVector::AptValueVector(const int32_t iSize)
    : mCapacity(iSize),
      mCurrentNum(0)
#if defined(APT_DEBUG) || defined(APT_ENABLE_ZOMBIE_OUTPUT)
      ,
      mHighWaterNum(0)
#endif
{
    mpValues = APT_MALLOC_ARRAY(AptValue *, mCapacity);
    APT_ASSERT(mpValues != NULL); //&& "Allocation Returned NULL!!!!");
}

/**
 */
AptValueVector::~AptValueVector()
{
    APT_ASSERT(GetNumValues() == 0);
    APT_FREE_ARRAY(mpValues, AptValue *, mCapacity);
    mpValues = NULL;
}

/**
 * This pushes a value to deferred vector and it is deleted later.
 * When the vector is full, we just go the old way of INC/DEC in the actual stack.push and stack.pop
 * operations. By this way, users can determine their own vector size depending on the
 * performance versus memory usage.
 * @param pValue AptValue *
 */
void AptValueVector::PushValue(AptValue *const pValue)
{

    APT_ASSERT(pValue->IsReleaseAtEnd()); //  Check that the upper level set the release flag
                                          //  So we push it only by using the standard process
    // APT_ASSERT(mCurrentNum < mCapacity);    //  If this ASSERT fires you need to increase
    //   AptInitParams.iDeferedVectorSize
    // commented the above assert as user community is not happy with this assert.
    // when we reach the capacity we just start the old way of INC/DEC.
    if (mCurrentNum >= mCapacity)
    {
        // its time not to push any more items inside this deferred vector.
        // so just clear the flag on to AptValue that is set to indicate whether to clear it at end or immediately.
        pValue->ClearReleaseAtEnd();
        return;
    }

    APT_ASSERT(pValue != NULL);
    mpValues[mCurrentNum++] = pValue;
#if defined(APT_DEBUG) || defined(APT_ENABLE_ZOMBIE_OUTPUT)
    if (mCurrentNum > mHighWaterNum)
    {
        mHighWaterNum = mCurrentNum;
    }
#endif
}

/**
 * @param None .
 */
void AptValueVector::ReleaseValues()
{

    AptValue *pValue;
    uint32_t uRefCount;

    while (mCurrentNum)
    {
        pValue    = PopValue();
        uRefCount = pValue->getRefCount();
        if (uRefCount > 0)
        {
            //  At the end, the object contained a non-null reference counting
            pValue->ClearReleaseAtEnd();
            continue;
        }

        //  The reference counting is NULL at the end, we need to delete it
        pValue->ForceDelete();
    }

    APT_ASSERT(mCurrentNum == 0);
}
/*H*************************************************************************************************
    EOF
*************************************************************************************************H*/
