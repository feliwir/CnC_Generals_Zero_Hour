#pragma once

#include "AptObject/AptObject.h"

/** @brief Actionscript "Stage" object. */
class AptStage : public AptObject
{
  public:
#if defined(APT_USE_STAGE_OBJECT)

    APT_VALUE_GC_NEW_DELETE_OPERATORS

    AptStage() : AptObject(AptVFT_Stage, APT_OBJECTHASHSIZE)
    {
        setGCRoot(1);
    }

    NATIVE_MEMBER_FUNCTION_DECL(addListener);
    NATIVE_MEMBER_FUNCTION_DECL(removeListener);
    virtual AptValue *objectMemberLookup(AptValue *const pContext, const AptNativeString *const pName) const;
    static void CleanNativeFunctions();
#else
    AptStage() : AptObject(AptVFT_Stage, APT_OBJECTHASHSIZE)
    {
        APT_ASSERT(0 && "APT-Warning-Stage object cannot be used as it is compiled out, make sure to compile with APT_USE_STAGE_OBJECT defined in Aptdefine.h");
    }

    static void *operator new(size_t size)
    {
        APT_ASSERT(0 && "APT-Warning-Stage object cannot be used as it is compiled out, make sure to compile with APT_USE_STAGE_OBJECT defined in Aptdefine.h");
        return (void *)0x00000000;
    }
    static void operator delete(void *p, size_t size) { APT_ASSERT(0 && "APT-Warning-Stage object cannot be used as it is compiled out, make sure to compile with APT_USE_STAGE_OBJECT defined in Aptdefine.h"); }
    static void *operator new[](size_t)
    {
        APT_ASSERT(0 && "APT-Warning-Stage object cannot be used as it is compiled out, make sure to compile with APT_USE_STAGE_OBJECT defined in Aptdefine.h");
        return (void *)0x00000000;
    }
    static void operator delete[](void *) { APT_ASSERT(0 && "APT-Warning-Stage object cannot be used as it is compiled out, make sure to compile with APT_USE_STAGE_OBJECT defined in Aptdefine.h"); }

#endif

  protected:
    APT_INLINE
    virtual ~AptStage()
    {
        //  Do nothing...
    }

  private:
};
