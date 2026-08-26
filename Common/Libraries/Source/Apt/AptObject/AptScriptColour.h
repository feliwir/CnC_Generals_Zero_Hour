#pragma once

#include "_Apt.h"

class AptScriptColour : public AptObject
{
  public:
    APT_VALUE_GC_NEW_DELETE_OPERATORS

#if APT_USE_SCRIPTCOLOUR_OBJECT
    AptCIH *pSprite;

    AptScriptColour(AptValue *const pMovie);

    virtual AptValue *objectMemberLookup(AptValue *const pContext, const AptNativeString *const pName) const;

    //-------------------------------------------------
    // members overrided from superclass.
    virtual void RegisterReferences();
    virtual void DestroyGCPointers();
#else
    AptScriptColour(AptValue *const pMovie) : AptObject(AptVFT_ScriptColour)
    {
        APT_ASSERT(0 && "APT-Warning-ScriptColour object cannot be used as it is compiled out, make sure to compile with APT_USE_SCRIPTCOLOUR_OBJECT defined as 1");
    }
#endif

    static void CleanNativeFunctions();

  protected:
    virtual ~AptScriptColour();

  private:
#if APT_USE_SCRIPTCOLOUR_OBJECT
    AptScriptColour();
    AptScriptColour(const AptScriptColour &other);
    AptScriptColour &operator=(const AptScriptColour &other);

    NATIVE_MEMBER_FUNCTION_DECL(setRGB);
    NATIVE_MEMBER_FUNCTION_DECL(getRGB);
    NATIVE_MEMBER_FUNCTION_DECL(getTransform);
    NATIVE_MEMBER_FUNCTION_DECL(setTransform);
#endif
};
