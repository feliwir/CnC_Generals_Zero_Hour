#pragma once
#include "AptValue/AptValue.h"
#include "_AptInternalDefines.h"

class AptLookup : public AptValueNoGC
{
  public:
    APT_VALUE_NEW_DELETE_OPERATORS


    int nLookup;
    static AptLookup *Create(int32_t nVal);

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

    static AptLookup *s_LookupArray;
    static int32_t s_nMaxLookups;

  private:
    virtual ~AptLookup()
    {
        //  Do nothing...
    }

    AptLookup(int32_t _nLookup) : AptValueNoGC(AptVFT_Lookup), nLookup(_nLookup)
    {
        setIsDefined(1);
        setRefCount(MAX_REFCOUNT);
    }

    AptLookup() : AptValueNoGC(AptVFT_Lookup), nLookup(0)
    {
        setIsDefined(1);
        setRefCount(MAX_REFCOUNT);
    }
};
