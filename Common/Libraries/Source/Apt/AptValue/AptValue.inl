/**
 * Out-of-line definitions of the typed AptValue::c_*() cast accessors declared APT_INLINE in
 * AptValue.h. Each asserts the value is actually of the expected type before casting.
 */

#include "_Apt.h"
#include "AptValue/AptValue.h"
#include "AptValue/AptValueVector.h"
#include "AptGlobal.h"

AptLookup *AptValue::c_lookup() const
{
    APT_ASSERT(isLookup());
    return (AptLookup *)this;
}

AptInteger *AptValue::c_integer() const
{
    APT_ASSERT(isInteger());
    return (AptInteger *)this;
}

AptRegister *AptValue::c_register() const
{
    APT_ASSERT(isRegister());
    return (AptRegister *)this;
}

AptFloat *AptValue::c_float() const
{
    APT_ASSERT(isFloat());
    return (AptFloat *)this;
}

AptBoolean *AptValue::c_boolean() const
{
    APT_ASSERT(isBoolean());
    return (AptBoolean *)this;
}

AptScriptFunctionBase *AptValue::c_scriptfunction() const
{
    APT_ASSERT(isScriptFunction());
    return (AptScriptFunctionBase *)this;
}

/** @return this cast to an AptNativeFunction. */
AptNativeFunction *AptValue::c_nativefunction() const
{
    APT_ASSERT(isNativeFunction());
    return (AptNativeFunction *)this;
}

/** @return this cast to an AptExternalFunction. */
AptExternalFunction *AptValue::c_externalfunction() const
{
    APT_ASSERT(isExternalFunction());
    return (AptExternalFunction *)this;
}

AptCIH *AptValue::c_cih(bool bUndefOK) const
{
    APT_ASSERT(isCIH(bUndefOK));
    return (AptCIH *)this;
}

#if defined(APT_USE_SOUND_OBJECT)
AptSound *AptValue::c_sound() const
{
    APT_ASSERT(isSound());
    return (AptSound *)this;
}
#endif

AptKey *AptValue::c_key() const
{
    APT_ASSERT(isKey());
    return (AptKey *)this;
}

#if defined(APT_USE_MOUSE)
inline AptMouse *AptValue::c_mouse() const
{
    APT_ASSERT(isMouse());
    return (AptMouse *)this;
}
#endif

inline AptGlobal *AptValue::c_global() const
{
    APT_ASSERT(isKey());
    return (AptGlobal *)this;
}

#if APT_USE_MATH_OBJECT
AptMathObj *AptValue::c_math() const
{
    APT_ASSERT(isMath());
    return (AptMathObj *)this;
}
#endif

#if APT_USE_SCRIPTCOLOUR_OBJECT
AptScriptColour *AptValue::c_scriptcolour() const
{
    APT_ASSERT(isScriptColour());
    return (AptScriptColour *)this;
}
#endif

AptObject *AptValue::c_object() const
{
    APT_ASSERT(isObject());
    return (AptObject *)this;
}

AptPrototype *AptValue::c_prototype() const
{
    APT_ASSERT(isPrototype());
    return (AptPrototype *)this;
}

AptDate *AptValue::c_date() const
{
    APT_ASSERT(isDate());
    return (AptDate *)this;
}

AptMovieClipLoader *AptValue::c_movieClipLoader() const
{
    APT_ASSERT(isMovieClipLoader());
    return (AptMovieClipLoader *)this;
}

AptTextFormat *AptValue::c_textformat() const
{
    APT_ASSERT(isTextFormat());
    return (AptTextFormat *)this;
}

AptMovieClip *AptValue::c_movieClip() const
{
    APT_ASSERT(isMovieClip());
    return (AptMovieClip *)this;
}

AptXmlNode *AptValue::c_xmlnode() const
{
    APT_ASSERT(isXmlNode());
    return (AptXmlNode *)this;
}

AptXml *AptValue::c_xml() const
{
    APT_ASSERT(isXml());
    return (AptXml *)this;
}

AptXmlAttributes *AptValue::c_xmlattributes() const
{
    APT_ASSERT(isXmlAttributes());
    return (AptXmlAttributes *)this;
}

#if APT_USE_LOADVARS_OBJECT
AptLoadVars *AptValue::c_loadvars() const
{
    APT_ASSERT(isLoadVars());
    return (AptLoadVars *)this;
}
#endif

AptStage *AptValue::c_stage() const
{
    APT_ASSERT(isStage());
    return (AptStage *)this;
}

#if APT_USE_UTILITY
AptUtil *AptValue::c_util() const
{
    APT_ASSERT(isAptUtil());
    return (AptUtil *)this;
}
#endif
