#pragma once
#include "AptObject/AptObject.h"

using AptNativeFunctionPointer = AptValue *(*)(AptValue * pThis, int nParams);
class AptNativeFunction : public AptObject
{
  public:
    APT_VALUE_GC_NEW_DELETE_OPERATORS
    // Added Metric defines

    AptNativeFunction(AptNativeFunctionPointer _pFunc) : AptObject(AptVFT_NativeFunction), mFunc(_pFunc)
    {
        // Native Functions don't need to be put on the deferred vector.
        SetAllowDelayedDeletion(false);
    }

    virtual AptValue *Call(AptValue *pContext, int nParams)
    {
        return mFunc(pContext, nParams);
    }

  protected:
    AptNativeFunctionPointer mFunc;

    virtual ~AptNativeFunction()
    {
        //  Do nothing...
    }
};
