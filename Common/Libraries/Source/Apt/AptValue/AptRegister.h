#pragma once
#include "AptValue/AptValue.h"
#include "_AptInternalDefines.h"

class AptRegister : public AptValueNoGC
{
  public:
    APT_VALUE_NEW_DELETE_OPERATORS


    int nVal;

    static AptRegister *Create(int32_t nVal);

    static void Initialize();

    static void Shutdown();

    // Now we are creating global lookup objects so do not do anything in Add/Release
#if defined(APT_INC_DEC_MESSAGES)
    virtual void AddRef(const char *szFuncName, const char *szFileName, int nLineNumber) {}  //  Do nothing...
    virtual void Release(const char *szFuncName, const char *szFileName, int nLineNumber) {} //  Do nothing...
#else
    virtual void AddRef() {}  //  Do nothing...
    virtual void Release() {} //  Do nothing...
#endif

    static AptRegister *s_RegisterArray;
    static int32_t s_nMaxRegisters;

  protected:
    virtual ~AptRegister()
    {
        //  Do nothing...
    }
    AptRegister(int _nVal) : AptValueNoGC(AptVFT_Register), nVal(_nVal)
    {
        setIsDefined(1);
        setRefCount(MAX_REFCOUNT);
    }
    AptRegister() : AptValueNoGC(AptVFT_Register), nVal(0)
    {
        setIsDefined(1);
        setRefCount(MAX_REFCOUNT);
    }
};
