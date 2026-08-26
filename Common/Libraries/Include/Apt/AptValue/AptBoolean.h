/**
 * Declares AptBoolean class
 */

#pragma once
#include "AptValue/AptValue.h"

class AptBoolean : public AptValueNoGC
{
  protected:
    //  The user can't call new and delete, need to call Create() and Destroy()
    static void *operator new(size_t size);
    static void operator delete(void *p, size_t size);
    static void *operator new[](size_t size);
    static void operator delete[](void *p);

  public:

    /** @brief Return the boolean value of thie object. */
    APT_INLINE bool GetBool() const;

    static AptBoolean *Create(const bool bValue);

    APT_ACCESS_INTERNAL : AptBoolean() : AptValueNoGC(AptVFT_Boolean)
    {
        mbValue = false;
        setIsDefined(1);
        setRefCount(MAX_REFCOUNT);
    }

    static void Initialize();

    static void Shutdown();

    // This Optimizes global objects by doing nothing on Add/Release
#if defined(APT_INC_DEC_MESSAGES)
    virtual void AddRef(const char *szFuncName, const char *szFileName, int nLineNumber) {}  //  Do nothing...
    virtual void Release(const char *szFuncName, const char *szFileName, int nLineNumber) {} //  Do nothing...
#else
    virtual void AddRef() {}  //  Do nothing...
    virtual void Release() {} //  Do nothing...
#endif

  protected:
    /** @brief Hide the destructor */
    APT_INLINE virtual ~AptBoolean()
    {
        // Do Nothing
    }

  private:
    bool mbValue;
    static AptBoolean *spBooleanFalse;
    static AptBoolean *spBooleanTrue;

    AptBoolean(bool bVal) : AptValueNoGC(AptVFT_Boolean)
    {
        mbValue = bVal;
        setIsDefined(1);
        setRefCount(MAX_REFCOUNT);
    }
};
