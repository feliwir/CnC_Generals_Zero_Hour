#include "_Apt.h"
#include "AptTarget.h"
#include "AptAnimationTarget.h"
#include "AptObject/AptKey.h"
#include "AptObject/AptKeyMembers.h"
#include "AptObject/AptMathObj.h"
#include "AptObject/AptMathMembers.h"
#include "AptObject/AptLoadVars.h"
#if APT_USE_LOADVARS_OBJECT
#include "AptObject/AptLoadVarsMembers.h"
#endif
#include "AptObject/AptError.h"
#include "AptObject/AptStringObject.h"
#include "AptObject/AptTextFormat.h"
#include "AptObject/AptTextFormatMembers.h"
#include "AptObject/AptUtil.h"
#if APT_USE_UTILITY
#include "AptObject/AptUtilMembers.h"
#include "AptValueHelper.h"
#include "AptValueFactory.h"
#endif
#include "MainInline.h"

static const int AptKey_BACKSPACE            = 1;
static const int AptKey_CAPSLOCK             = 2;
static const int AptKey_CONTROL              = 3;
static const int AptKey_DELETEKEY            = 4;
static const int AptKey_DOWN                 = 5;
static const int AptKey_END                  = 6;
static const int AptKey_ENTER                = 7;
static const int AptKey_ESCAPE               = 8;
static const int AptKey_HOME                 = 9;
static const int AptKey_INSERT               = 10;
static const int AptKey_LEFT                 = 11;
static const int AptKey_PGDN                 = 12;
static const int AptKey_PGUP                 = 13;
static const int AptKey_RIGHT                = 14;
static const int AptKey_SHIFT                = 15;
static const int AptKey_SPACE                = 16;
static const int AptKey_TAB                  = 17;
static const int AptKey_UP                   = 18;
static const int AptKey_isDown               = 100;
static const int AptKey_isToggled            = 101;
static const int AptKey_getCode              = 102;
static const int AptKey_getController        = 103;
static const int AptKey_addListener          = 104;
static const int AptKey_removeListener       = 105;
static const int AptKey_getAnalogStickInfo   = 106;
static const int AptKey_getAscii             = 107;
static const int AptKey_getAnalogTriggerInfo = 108;
static const int AptKey_getGestureInfo       = 109;

#if APT_USE_MATH_OBJECT
static const int AptMathObj_sin    = 1;
static const int AptMathObj_cos    = 2;
static const int AptMathObj_atan2  = 3;
static const int AptMathObj_round  = 4;
static const int AptMathObj_min    = 5;
static const int AptMathObj_max    = 6;
static const int AptMathObj_abs    = 7;
static const int AptMathObj_acos   = 8;
static const int AptMathObj_asin   = 9;
static const int AptMathObj_atan   = 10;
static const int AptMathObj_ceil   = 11;
static const int AptMathObj_exp    = 12;
static const int AptMathObj_floor  = 13;
static const int AptMathObj_log    = 14;
static const int AptMathObj_pow    = 15;
static const int AptMathObj_random = 16;
static const int AptMathObj_sqrt   = 17;
static const int AptMathObj_tan    = 18;
#endif

#if APT_USE_LOADVARS_OBJECT
static const int AptLoadVars_load           = 1;
static const int AptLoadVars_send           = 2;
static const int AptLoadVars_sendAndLoad    = 3;
static const int AptLoadVars_getBytesTotal  = 4;
static const int AptLoadVars_getBytesLoaded = 5;
static const int AptLoadVars_loaded         = 6;
static const int AptLoadVars_toString       = 7;
static const int AptLoadVars_contentType    = 8;
#endif

static const int AptTextFormat_align       = 1;
static const int AptTextFormat_blockIndent = 2;
static const int AptTextFormat_bold        = 3;
static const int AptTextFormat_bullet      = 4;
static const int AptTextFormat_color       = 5;
static const int AptTextFormat_font        = 6;
static const int AptTextFormat_indent      = 7;
static const int AptTextFormat_italic      = 8;
static const int AptTextFormat_leading     = 9;
static const int AptTextFormat_leftMargin  = 10;
static const int AptTextFormat_rightMargin = 11;
static const int AptTextFormat_tabStops    = 12;
static const int AptTextFormat_target      = 13;
static const int AptTextFormat_size        = 14;
static const int AptTextFormat_underline   = 15;
static const int AptTextFormat_url         = 16;
static const int AptTextFormat_tracking    = 17;

#if APT_USE_UTILITY
enum
{
    AptUtil_formatNumberString = 1,
    AptUtil_replaceString,
    AptUtil_trimString,
    AptUtil_trimLeftString,
    AptUtil_trimRightString,
    AptUtil_searchArray,
    AptUtil_reverseSearchArray,
    AptUtil_countArray,
    AptUtil_getAptVersion,
    AptUtil_safeForIn,
    AptUtil_convertHsvToColorTransform,
    AptUtil_colorMatrixMultiply,
    AptUtil_profileBlockStart,
    AptUtil_profileBlockEnd,
};

// helper for warnings about sending too many args
#if APT_DEBUG_LEVEL >= APT_DEBUG_MSG_NORMAL_LVL
#define ASSERT_IF_TOO_MANY_DEFINED_ARGS(maximumUsed, numArgsSent, message) \
    for (int i = maximumUsed; i < numArgsSent; ++i)                        \
    {                                                                      \
        if (!gAptActionInterpreter.stackAt(i)->isUndefined())              \
        {                                                                  \
            APT_ASSERTM(false, message);                                   \
            break;                                                         \
        }                                                                  \
    }
#else
#define ASSERT_IF_TOO_MANY_DEFINED_ARGS(maximumUsed, numArgsSent, message)
#endif
#endif // APT_USE_UTILITY

//------------------------------------------------------------------------------------------
// AptMathObj object

/** Frees the native functions cached by the AptMathObj class. Must be called at least during AptShutdown. */
void AptMathObj::CleanNativeFunctions()
{
#if APT_USE_MATH_OBJECT
    NATIVE_MEMBER_FUNCTION_DESTROY(sin);
    NATIVE_MEMBER_FUNCTION_DESTROY(cos);
    NATIVE_MEMBER_FUNCTION_DESTROY(atan2);
    NATIVE_MEMBER_FUNCTION_DESTROY(round);
    NATIVE_MEMBER_FUNCTION_DESTROY(min);
    NATIVE_MEMBER_FUNCTION_DESTROY(max);

    NATIVE_MEMBER_FUNCTION_DESTROY(abs);
    NATIVE_MEMBER_FUNCTION_DESTROY(acos);
    NATIVE_MEMBER_FUNCTION_DESTROY(asin);
    NATIVE_MEMBER_FUNCTION_DESTROY(atan);
    NATIVE_MEMBER_FUNCTION_DESTROY(ceil);
    NATIVE_MEMBER_FUNCTION_DESTROY(exp);
    NATIVE_MEMBER_FUNCTION_DESTROY(floor);
    NATIVE_MEMBER_FUNCTION_DESTROY(log);
    NATIVE_MEMBER_FUNCTION_DESTROY(pow);
    NATIVE_MEMBER_FUNCTION_DESTROY(random);
    NATIVE_MEMBER_FUNCTION_DESTROY(sqrt);
    NATIVE_MEMBER_FUNCTION_DESTROY(tan);
#endif
}

#if APT_USE_MATH_OBJECT

/**
 * Looks up a member of the Math object by name.
 * @return the AptValue for the member, or gpUndefinedValue if unknown.
 */
AptValue *AptMathObj::objectMemberLookup(AptValue *const pContext, const AptNativeString *const pName) const
{
    MathMembers *pProp = pContext ? MathMembersIndex::in_word_set(pName->c_str(), pName->Size()) : NULL;

    if (pProp)
    {
        switch (pProp->nIndex)
        {
        case (AptMathObj_sin):
        {
            NATIVE_MEMBER_FUNCTION_DISPATCH(sin);
        }
        break;
        case AptMathObj_cos:
        {
            NATIVE_MEMBER_FUNCTION_DISPATCH(cos);
        }
        break;
        case AptMathObj_atan2:
        {
            NATIVE_MEMBER_FUNCTION_DISPATCH(atan2);
        }
        break;
        case AptMathObj_round:
        {
            NATIVE_MEMBER_FUNCTION_DISPATCH(round);
        }
        break;
        case AptMathObj_min:
        {
            NATIVE_MEMBER_FUNCTION_DISPATCH(min);
        }
        break;
        case AptMathObj_max:
        {
            NATIVE_MEMBER_FUNCTION_DISPATCH(max);
        }
        break;
        case AptMathObj_abs:
        {
            NATIVE_MEMBER_FUNCTION_DISPATCH(abs);
        }
        break;
        case AptMathObj_acos:
        {
            NATIVE_MEMBER_FUNCTION_DISPATCH(acos);
        }
        break;
        case AptMathObj_asin:
        {
            NATIVE_MEMBER_FUNCTION_DISPATCH(asin);
        }
        break;
        case AptMathObj_atan:
        {
            NATIVE_MEMBER_FUNCTION_DISPATCH(atan);
        }
        break;
        case AptMathObj_ceil:
        {
            NATIVE_MEMBER_FUNCTION_DISPATCH(ceil);
        }
        break;
        case AptMathObj_exp:
        {
            NATIVE_MEMBER_FUNCTION_DISPATCH(exp);
        }
        break;
        case AptMathObj_floor:
        {
            NATIVE_MEMBER_FUNCTION_DISPATCH(floor);
        }
        break;
        case AptMathObj_log:
        {
            NATIVE_MEMBER_FUNCTION_DISPATCH(log);
        }
        break;
        case AptMathObj_pow:
        {
            NATIVE_MEMBER_FUNCTION_DISPATCH(pow);
        }
        break;
        case AptMathObj_random:
        {
            NATIVE_MEMBER_FUNCTION_DISPATCH(random);
        }
        break;
        case AptMathObj_sqrt:
        {
            NATIVE_MEMBER_FUNCTION_DISPATCH(sqrt);
        }
        break;
        case AptMathObj_tan:
        {
            NATIVE_MEMBER_FUNCTION_DISPATCH(tan);
        }
        }
    }

#if defined(EA_DEBUG)
    if (
        (pName->EqualNoCase("sin")) ||
        (pName->EqualNoCase("cos")) ||
        (pName->EqualNoCase("atan2")) ||
        (pName->EqualNoCase("round")) ||
        (pName->EqualNoCase("min")) ||
        (pName->EqualNoCase("max")) ||
        (pName->EqualNoCase("abs")) ||
        (pName->EqualNoCase("acos")) ||
        (pName->EqualNoCase("asin")) ||
        (pName->EqualNoCase("atan")) ||
        (pName->EqualNoCase("ceil")) ||
        (pName->EqualNoCase("exp")) ||
        (pName->EqualNoCase("floor")) ||
        (pName->EqualNoCase("log")) ||
        (pName->EqualNoCase("pow")) ||
        (pName->EqualNoCase("random")) ||
        (pName->EqualNoCase("sqrt")) ||
        (pName->EqualNoCase("tan")))
    {
        APT_DEBUGPRINT(APT_DEBUG_MSG_WARNING_LVL, "AptMathObj: Incorrect case for '%s'.\n", pName->c_str());
        APT_ASSERT(0);
    }
#endif

    return 0;
}

NATIVE_MEMBER_FUNCTION(AptMathObj, sin)
{
    if (nParams < 1)
        return gpUndefinedValue;
    AptValue *pParam = gAptActionInterpreter.stackAt(0);
    return AptFloat::Create(sinf(pParam->toFloat()));
}

NATIVE_MEMBER_FUNCTION(AptMathObj, cos)
{
    if (nParams < 1)
        return gpUndefinedValue;
    AptValue *pParam = gAptActionInterpreter.stackAt(0);
    return AptFloat::Create(cosf(pParam->toFloat()));
}

NATIVE_MEMBER_FUNCTION(AptMathObj, atan2)
{
    if (nParams < 2)
        return gpUndefinedValue;
    AptValue *pParam0 = gAptActionInterpreter.stackAt(0);
    AptValue *pParam1 = gAptActionInterpreter.stackAt(1);
    return AptFloat::Create(atan2f(pParam0->toFloat(), pParam1->toFloat()));
}

NATIVE_MEMBER_FUNCTION(AptMathObj, round)
{
    if (nParams <= 0)
        return gpUndefinedValue;
    AptValue *pParam0 = gAptActionInterpreter.stackAt(0);
    float fVal        = pParam0->toFloat();
    int nVal          = fVal > 0 ? (int)(fVal + 0.5f) : (int)(fVal - 0.5f);
    return AptInteger::Create(nVal);
}

NATIVE_MEMBER_FUNCTION(AptMathObj, min)
{
    if (nParams < 2)
        return gpUndefinedValue;
    AptValue *pParam0 = gAptActionInterpreter.stackAt(0);
    AptValue *pParam1 = gAptActionInterpreter.stackAt(1);
    return AptFloat::Create(pParam0->toFloat() < pParam1->toFloat() ? pParam0->toFloat() : pParam1->toFloat());
}

NATIVE_MEMBER_FUNCTION(AptMathObj, max)
{
    if (nParams < 2)
        return gpUndefinedValue;
    AptValue *pParam0 = gAptActionInterpreter.stackAt(0);
    AptValue *pParam1 = gAptActionInterpreter.stackAt(1);
    return AptFloat::Create(pParam0->toFloat() > pParam1->toFloat() ? pParam0->toFloat() : pParam1->toFloat());
}

NATIVE_MEMBER_FUNCTION(AptMathObj, abs)
{
    if (nParams < 1)
        return gpUndefinedValue;
    AptValue *pParam0 = gAptActionInterpreter.stackAt(0);

    if (pParam0->isInteger())
    {
        return AptInteger::Create(abs(pParam0->toInteger()));
    }
    else // abs() also needs to handle floats.
    {
        return AptFloat::Create(fabsf((pParam0->toFloat())));
    }
}

NATIVE_MEMBER_FUNCTION(AptMathObj, acos)
{
    if (nParams < 1)
        return gpUndefinedValue;
    AptValue *pParam = gAptActionInterpreter.stackAt(0);
    return AptFloat::Create(acosf(pParam->toFloat()));
}

NATIVE_MEMBER_FUNCTION(AptMathObj, asin)
{
    if (nParams < 1)
        return gpUndefinedValue;
    AptValue *pParam = gAptActionInterpreter.stackAt(0);
    return AptFloat::Create(asinf(pParam->toFloat()));
}
NATIVE_MEMBER_FUNCTION(AptMathObj, atan)
{
    if (nParams < 1)
        return gpUndefinedValue;
    AptValue *pParam = gAptActionInterpreter.stackAt(0);
    return AptFloat::Create(atanf(pParam->toFloat()));
}
NATIVE_MEMBER_FUNCTION(AptMathObj, ceil)
{
    if (nParams < 1)
        return gpUndefinedValue;
    AptValue *pParam = gAptActionInterpreter.stackAt(0);
    return AptFloat::Create(ceilf(pParam->toFloat()));
}
NATIVE_MEMBER_FUNCTION(AptMathObj, exp)
{
    if (nParams < 1)
        return gpUndefinedValue;
    AptValue *pParam = gAptActionInterpreter.stackAt(0);
    return AptFloat::Create(expf(pParam->toFloat()));
}
NATIVE_MEMBER_FUNCTION(AptMathObj, floor)
{
    if (nParams < 1)
        return gpUndefinedValue;
    AptValue *pParam = gAptActionInterpreter.stackAt(0);
    return AptFloat::Create(floorf(pParam->toFloat()));
}
NATIVE_MEMBER_FUNCTION(AptMathObj, log)
{
    if (nParams < 1)
        return gpUndefinedValue;
    AptValue *pParam = gAptActionInterpreter.stackAt(0);
    return AptFloat::Create(logf(pParam->toFloat()));
}
NATIVE_MEMBER_FUNCTION(AptMathObj, pow)
{
    if (nParams < 2)
        return gpUndefinedValue;
    AptValue *pParam0 = gAptActionInterpreter.stackAt(0);
    AptValue *pParam1 = gAptActionInterpreter.stackAt(1);
    return AptFloat::Create(powf(pParam0->toFloat(), pParam1->toFloat()));
}

NATIVE_MEMBER_FUNCTION(AptMathObj, random)
{
    return AptFloat::Create((float)AptRand() / 0xffffffff);
}
NATIVE_MEMBER_FUNCTION(AptMathObj, sqrt)
{
    if (nParams < 1)
        return gpUndefinedValue;
    AptValue *pParam = gAptActionInterpreter.stackAt(0);
    return AptFloat::Create(sqrtf(pParam->toFloat()));
}
NATIVE_MEMBER_FUNCTION(AptMathObj, tan)
{
    if (nParams < 1)
        return gpUndefinedValue;
    AptValue *pParam = gAptActionInterpreter.stackAt(0);
    return AptFloat::Create(tanf(pParam->toFloat()));
}
#endif // APT_USE_MATH_OBJECT

//------------------------------------------------------------------------------------------
// AptKey object


/** Frees the native functions cached by the AptKey class. Must be called at least during AptShutdown. */
void AptKey::CleanNativeFunctions()
{
    NATIVE_MEMBER_FUNCTION_DESTROY(isDown);
    NATIVE_MEMBER_FUNCTION_DESTROY(isToggled);
    NATIVE_MEMBER_FUNCTION_DESTROY(getCode);
    NATIVE_MEMBER_FUNCTION_DESTROY(getController);
    NATIVE_MEMBER_FUNCTION_DESTROY(addListener);
    NATIVE_MEMBER_FUNCTION_DESTROY(removeListener);
    NATIVE_MEMBER_FUNCTION_DESTROY(getAnalogStickInfo);
    NATIVE_MEMBER_FUNCTION_DESTROY(getAnalogTriggerInfo);
    NATIVE_MEMBER_FUNCTION_DESTROY(getGestureInfo);
    NATIVE_MEMBER_FUNCTION_DESTROY(getAscii);
}

/**
 * Looks up a member of the Key object by name.
 * @return the AptValue for the member, or gpUndefinedValue if unknown.
 */
AptValue *AptKey::objectMemberLookup(AptValue *const pContext, const AptNativeString *const pName) const
{
    KeyMembers *pProp = pContext && pContext->isKey() ? KeyMembersIndex::in_word_set(pName->c_str(), pName->Size()) : NULL;

    if (pProp)
    {
        switch (pProp->nIndex)
        {
        case (AptKey_BACKSPACE):
        {
            return AptInteger::Create(8);
        }
        break;
        case AptKey_CAPSLOCK:
        {
            return AptInteger::Create(20);
        }
        break;
        case AptKey_CONTROL:
        {
            return AptInteger::Create(17);
        }
        break;
        case AptKey_DELETEKEY:
        {
            return AptInteger::Create(46);
        }
        break;
        case AptKey_DOWN:
        {
            return AptInteger::Create(40);
        }
        break;
        case AptKey_END:
        {
            return AptInteger::Create(35);
        }
        break;
        case AptKey_ENTER:
        {
            return AptInteger::Create(13);
        }
        break;
        case AptKey_ESCAPE:
        {
            return AptInteger::Create(27);
        }
        break;
        case AptKey_HOME:
        {
            return AptInteger::Create(36);
        }
        break;
        case AptKey_INSERT:
        {
            return AptInteger::Create(45);
        }
        break;
        case AptKey_LEFT:
        {
            return AptInteger::Create(37);
        }
        break;
        case AptKey_PGDN:
        {
            return AptInteger::Create(34);
        }
        break;
        case AptKey_PGUP:
        {
            return AptInteger::Create(33);
        }
        break;
        case AptKey_RIGHT:
        {
            return AptInteger::Create(39);
        }
        break;
        case AptKey_SHIFT:
        {
            return AptInteger::Create(16);
        }
        break;
        case AptKey_SPACE:
        {
            return AptInteger::Create(32);
        }
        break;
        case AptKey_TAB:
        {
            return AptInteger::Create(9);
        }
        break;
        case AptKey_UP:
        {
            return AptInteger::Create(38);
        }
        break;
        case AptKey_isDown:
        {
            NATIVE_MEMBER_FUNCTION_DISPATCH(isDown);
        }
        break;
        case AptKey_isToggled:
        {
            NATIVE_MEMBER_FUNCTION_DISPATCH(isToggled);
        }
        break;
        case AptKey_getCode:
        {
            NATIVE_MEMBER_FUNCTION_DISPATCH(getCode);
        }
        break;
        case AptKey_getController:
        {
            NATIVE_MEMBER_FUNCTION_DISPATCH(getController);
        }
        break;
        case AptKey_addListener:
        {
            NATIVE_MEMBER_FUNCTION_DISPATCH(addListener);
        }
        break;
        case AptKey_removeListener:
        {
            NATIVE_MEMBER_FUNCTION_DISPATCH(removeListener);
        }
        break;
        case AptKey_getAnalogStickInfo:
        {
            NATIVE_MEMBER_FUNCTION_DISPATCH(getAnalogStickInfo);
        }
        break;
        case AptKey_getAnalogTriggerInfo:
        {
            NATIVE_MEMBER_FUNCTION_DISPATCH(getAnalogTriggerInfo);
        }
        break;
        case AptKey_getGestureInfo:
        {
            NATIVE_MEMBER_FUNCTION_DISPATCH(getGestureInfo);
        }
        break;
        case AptKey_getAscii:
        {
            NATIVE_MEMBER_FUNCTION_DISPATCH(getAscii);
        }
        }
    }
    return 0;
}

// Flash has two different sets of keycodes: one for button actions, one for Key.XYZ. This
// translates from button actions (which we use, and are in Apt.h) to what Key.getCode wants.
static const int BUTTONACTIONCODE_TO_KEYCODE[] =
    {
        -1, // unused
        37, // LEFT = 1,
        39, // RIGHT = 2,
        36, // HOME = 3,
        35, // END = 4,
        45, // INSERT = 5,
        46, // DELETE = 6,
        -1, // unused
        8,  // BACKSPACE = 8,
        -1, // unused
        -1, // unused
        -1, // unused
        -1, // unused
        13, // ENTER = 13,
        38, // UP = 14,
        40, // DOWN = 15,
        33, // PGUP = 16,
        34, // PGDN = 17,
        9,  // TAB = 18,
        27, // ESCAPE = 19,
};

/**
 * key.isDown() only works within onKeyPress events; not fully supported at this time.
 */
NATIVE_MEMBER_FUNCTION(AptKey, isDown)
{
    int nInt         = 0;
    AptValue *pValue = gAptActionInterpreter.stackAt(0);

    if (INPUT_IS_KEY(&gAptActionInterpreter.input))
    {
        GET_ACTION_TYPE(gAptActionInterpreter.input, nInt);

        // getCode always uses uppercase
        if (nInt >= AptInputType_ASCII32 && nInt <= AptInputType_ASCII126)
        {
            nInt = toupper(nInt);
        }
        else if (nInt < APT_ARRAYSIZE(BUTTONACTIONCODE_TO_KEYCODE))
        {
            nInt = BUTTONACTIONCODE_TO_KEYCODE[nInt];
        }

        int lookingForValue = pValue->toInteger();
        return AptBoolean::Create((nInt) == lookingForValue);
    }
    return AptBoolean::Create(false);
}

/** Not implemented; intended to return info about num-lock and caps-lock. */
NATIVE_MEMBER_FUNCTION(AptKey, isToggled)
{
    APT_ASSERT(false); // Not Implemented

    return AptBoolean::Create(false);
}

/**
 * Implements key.getCode(): returns whatever the game/auxiliary library passed in via
 * AptAddToInputQueue when the controller is a keyboard.
 */
NATIVE_MEMBER_FUNCTION(AptKey, getCode)
{
    int nInt;
    GET_ACTION_TYPE(gAptActionInterpreter.input, nInt);

    // getCode always uses uppercase
    if (nInt >= AptInputType_ASCII32 && nInt <= AptInputType_ASCII126)
    {
        nInt = toupper(nInt);
    }
    else if (nInt < APT_ARRAYSIZE(BUTTONACTIONCODE_TO_KEYCODE))
    {
        nInt = BUTTONACTIONCODE_TO_KEYCODE[nInt];
    }

    return AptInteger::Create(nInt);
}

/**
 * Implements key.getAscii(): returns whatever the game/auxiliary library passed in via
 * AptAddToInputQueue when the controller is a keyboard.
 */
NATIVE_MEMBER_FUNCTION(AptKey, getAscii)
{
    int nInt;
    int eController;
    GET_ACTION_TYPE(gAptActionInterpreter.input, nInt);
    GET_ACTION_CONTROLLER(gAptActionInterpreter.input, eController);

    if (eController == AptInputController_Keyboard)
    {
        return AptInteger::Create(nInt);
    }
    else if (nInt < APT_ARRAYSIZE(BUTTONACTIONCODE_TO_KEYCODE))
    {
        nInt = BUTTONACTIONCODE_TO_KEYCODE[nInt];
    }

    return AptInteger::Create(nInt);
}

NATIVE_MEMBER_FUNCTION(AptKey, getController)
{
    int eController; // Actually of type AptInputController but easier to handle as int here.
    GET_ACTION_CONTROLLER(gAptActionInterpreter.input, eController);

    eController -= AptInputController_Gamepad0; // convert to 0 based (assuming gamepad constants are contiguous)

    return AptInteger::Create(eController);
}

// Links to listener objects may remain in mListenerSet even after the object goes out of scope,
// since the link keeps its refcount at least 1. ActionScript must call removeListener when the
// scope of the object ends, to allow it to eventually be deleted.
NATIVE_MEMBER_FUNCTION(AptKey, addListener)
{
    if (nParams != 1)
        return gpUndefinedValue;

    AptValue *pParam = gAptActionInterpreter.stackAt(0);
    if (pParam->getIsDefined())
    {
        if (pParam->isCIH()) // Don't add to listener set if the CIH is a zombie.
        {
            if (((AptCIH *)pParam)->GetCIHState() != AptCIH::AptCIHState_Normal)
            {
                return gpUndefinedValue;
            }
        }
        // Object is defined and valid, so add it to mListenerSet.
        if (!GetTargetSim()->GetAnimationTarget()->GetListenerSet()->has(pParam))
            GetTargetSim()->GetAnimationTarget()->GetListenerSet()->add(pParam);
    }

    return gpUndefinedValue;
}

// ActionScript should call this when the listening object goes out of scope. Note that since
// there is currently only one listener set for all event types, this also removes the object
// from listening to any other events (e.g. mouseUp).
NATIVE_MEMBER_FUNCTION(AptKey, removeListener)
{
    if (nParams != 1)
        return AptBoolean::Create(false);

    AptValue *pParam = gAptActionInterpreter.stackAt(0);
    if (pParam->getIsDefined())
    {
        if (GetTargetSim()->GetAnimationTarget()->GetListenerSet()->has(pParam))
        {
            GetTargetSim()->GetAnimationTarget()->GetListenerSet()->remove(pParam);
            return AptBoolean::Create(true);
        }
    }

    return AptBoolean::Create(false);
}

/**
 * Implements Key.getAnalogStickInfo(). ActionScript detects an analog stick event via
 * key.getCode(), then calls this to retrieve the last known position data for that stick.
 */
NATIVE_MEMBER_FUNCTION(AptKey, getAnalogStickInfo)
{
    int nController    = 0;
    AptInputType eType = AptInputType_LeftAnalogStick;
    if (gAptActionInterpreter.input != 0)
    {
        GET_ANALOG_INPUT(gAptActionInterpreter.input, eType, nController);
    }

    int nController0Based = nController - AptInputController_Gamepad0; // convert to 0 based (assuming gamepad constants are contiguous)

    AptObject *pObject = new AptObject(AptVFT_Object);

    // set controller id
    AptInteger *pController = AptInteger::Create(nController0Based);
    pObject->Set(StringPool::GetString(SC_controller), pController);

    AptAnalogStickInfo *pAnalogInfo = NULL;
    if (eType == AptInputType_LeftAnalogStick)
    {
        pAnalogInfo = GetTargetSim()->GetAnimationTarget()->GetAStickLeft(nController);
    }
    else if (eType == AptInputType_RightAnalogStick)
    {
        pAnalogInfo = GetTargetSim()->GetAnimationTarget()->GetAStickRight(nController);
    }

    APT_ASSERT(pAnalogInfo != NULL && "invalid stick id extracted out of input")

    if (pAnalogInfo)
    {
        AptFloat *pfXAxis = AptFloat::Create(pAnalogInfo->fXAxisValue);
        AptFloat *pfYAxis = AptFloat::Create(pAnalogInfo->fYAxisValue);

        AptNativeString pTmpStr = "fXAxisValue";
        pObject->Set(&pTmpStr, pfXAxis);
        pTmpStr = "fYAxisValue";
        pObject->Set(&pTmpStr, pfYAxis);
    }
    return pObject;
}

/**
 * Implements Key.getAnalogTriggerInfo(): returns the last known position data for the analog
 * triggers of the controller that most recently generated input.
 */
NATIVE_MEMBER_FUNCTION(AptKey, getAnalogTriggerInfo)
{
    int nController = AptInputController_Gamepad0;
    // Gets the last controller that received input
    if (gAptActionInterpreter.input != 0)
    {
        GET_ACTION_CONTROLLER(gAptActionInterpreter.input, nController);
    }

    AptObject *pObject = new AptObject(AptVFT_Object);

    // 0 based controller
    pObject->Set(StringPool::GetString(SC_controller), AptInteger::Create(nController - AptInputController_Gamepad0));

    // Last values of the triggers
    AptAnalogStickInfo *pAnalogInfo = GetTargetSim()->GetAnimationTarget()->GetATriggers(nController);
    AptNativeString tempStr         = "fLeftTrigger";
    pObject->Set(&tempStr, AptFloat::Create(pAnalogInfo->fXAxisValue));
    tempStr = "fRightTrigger";
    pObject->Set(&tempStr, AptFloat::Create(pAnalogInfo->fYAxisValue));

    return pObject;
}

/**
 * Implements Key.getGestureInfo(). ActionScript detects a gesture event via key.getCode(), then
 * calls this to retrieve the last known position data from the motion controller.
 */
NATIVE_MEMBER_FUNCTION(AptKey, getGestureInfo)
{
    AptInputController nController = AptInputController_Gamepad0;
    AptInputType eType             = AptInputType_NumInputs;

    if (gAptActionInterpreter.input != 0)
    {
        GET_ACTION_CONTROLLER(gAptActionInterpreter.input, nController);

        GET_ACTION_TYPE(gAptActionInterpreter.input, eType);
    }

    int nController0Based = nController - AptInputController_Gamepad0; // convert to 0 based (assuming gamepad constants are contiguous)

    AptObject *pObject = new AptObject(AptVFT_Object);

    // set controller id
    AptInteger *pController = AptInteger::Create(nController0Based);
    pObject->Set(StringPool::GetString(SC_controller), pController);

    AptGestureInfo *pGestureInfo = NULL;

    pGestureInfo = GetTargetSim()->GetAnimationTarget()->GetGestureInfo(nController, eType);

    APT_ASSERT(pGestureInfo != NULL && "invalid controller id extracted out of input")

    if (pGestureInfo)
    {
        AptFloat *pfGestureData = AptFloat::Create(pGestureInfo->fGestureData);

        AptNativeString pTmpStr = "fGestureData";
        pObject->Set(&pTmpStr, pfGestureData);
    }
    return pObject;
}

#if defined(APT_ALTERNATE_INPUT)
//------------------------------------------------------------------------------------------
// AptAlternateInput object


/** Frees the native functions cached by the AptAlternateInput class. */
void AptAlternateInput::CleanNativeFunctions()
{
    NATIVE_MEMBER_FUNCTION_DESTROY(addListener);
    NATIVE_MEMBER_FUNCTION_DESTROY(removeListener);
}

/**
 * Looks up a member of the AlternateInput object by name.
 * @return the AptValue for the member, or gpUndefinedValue if unknown.
 */
AptValue *AptAlternateInput::objectMemberLookup(AptValue *const pContext, const AptNativeString *const pName) const
{
    if (*pName == "addListener")
    {
        NATIVE_MEMBER_FUNCTION_DISPATCH(addListener);
    }
    else if (*pName == "removeListener")
    {
        NATIVE_MEMBER_FUNCTION_DISPATCH(removeListener);
    }
    return 0;
}

/** Adds the passed in parameter to the alternate input set. */
NATIVE_MEMBER_FUNCTION(AptAlternateInput, addListener)
{
    if (nParams != 1)
        return gpUndefinedValue;

    AptValue *pParam = gAptActionInterpreter.stackAt(0);
    if (pParam->getIsDefined())
    {
        if (pParam->isCIH()) // Don't add to listener set if the CIH is a zombie.
        {
            if (((AptCIH *)pParam)->GetCIHState() != AptCIH::AptCIHState_Normal)
            {
                return gpUndefinedValue;
            }
        }
        if (!GetTargetSim()->GetAnimationTarget()->GetAlternateInputSet()->has(pParam))
            GetTargetSim()->GetAnimationTarget()->GetAlternateInputSet()->add(pParam);
    }

    return gpUndefinedValue;
}

/**
 * Removes the passed in parameter from the alternate input set. ActionScript should call this
 * when the listening object goes out of scope.
 */
NATIVE_MEMBER_FUNCTION(AptAlternateInput, removeListener)
{
    if (nParams != 1)
        return AptBoolean::Create(false);

    AptValue *pParam = gAptActionInterpreter.stackAt(0);
    if (pParam->getIsDefined())
    {
        if (GetTargetSim()->GetAnimationTarget()->GetAlternateInputSet()->has(pParam->c_cih()))
        {
            GetTargetSim()->GetAnimationTarget()->GetAlternateInputSet()->remove(pParam->c_cih());
            return AptBoolean::Create(true);
        }
    }

    return AptBoolean::Create(false);
}
#endif // #if defined(APT_ALTERNATE_INPUT)

//------------------------------------------------------------------------------------------
// AptLoadVars object

#if APT_USE_LOADVARS_OBJECT

/**
 * Looks up a member of the LoadVars object by name.
 * @return the AptValue for the member, or gpUndefinedValue if unknown.
 */
AptValue *AptLoadVars::objectMemberLookup(AptValue *const pContext, const AptNativeString *const pName) const
{
    LoadVarsMembers *pProp = pContext ? LoadVarsMembersIndex::in_word_set(pName->c_str(), pName->Size()) : NULL;
    if (pProp)
    {
        switch (pProp->nIndex)
        {
        case (AptLoadVars_load):
        {
            NATIVE_MEMBER_FUNCTION_DISPATCH(load);
        }
        break;
        case AptLoadVars_send:
        {
            NATIVE_MEMBER_FUNCTION_DISPATCH(send);
        }
        break;
        case AptLoadVars_sendAndLoad:
        {
            NATIVE_MEMBER_FUNCTION_DISPATCH(sendAndLoad);
        }
        break;
        case AptLoadVars_getBytesTotal:
        {
            NATIVE_MEMBER_FUNCTION_DISPATCH(getBytesTotal);
        }
        break;
        case AptLoadVars_getBytesLoaded:
        {
            NATIVE_MEMBER_FUNCTION_DISPATCH(getBytesLoaded);
        }
        break;
        case AptLoadVars_loaded:
        {
            return AptBoolean::Create(this->iIsLoaded != 0);
        }
        break;
        case AptLoadVars_toString:
        {
            NATIVE_MEMBER_FUNCTION_DISPATCH(toString);
        }
        break;
        case AptLoadVars_contentType:
        {
            AptString *pString = AptString::Create();
            pString->cpy("application/x-www-form-urlencoded");
            return pString;
        }
        }
    }

#if defined(EA_DEBUG)
    if (
        (pName->EqualNoCase("load")) ||
        (pName->EqualNoCase("send")) ||
        (pName->EqualNoCase("sendAndLoad")) ||
        (pName->EqualNoCase("getBytesTotal")) ||
        (pName->EqualNoCase("getBytesLoaded")) ||
        (pName->EqualNoCase("loaded")) ||
        (pName->EqualNoCase("toString")) ||
        (pName->EqualNoCase("contentType")))
    {
        APT_DEBUGPRINT(APT_DEBUG_MSG_WARNING_LVL, "AptLoadVars: Incorrect case for '%s'.\n", pName->c_str());
        APT_ASSERT(0);
    }
#endif

    return 0;
}
#endif // #if APT_USE_LOADVARS_OBJECT

AptLoadVars::~AptLoadVars()
{
    //  Do nothing...
}

/** Frees the native functions cached by the AptLoadVars class. Must be called at least during AptShutdown. */
void AptLoadVars::CleanNativeFunctions()
{
#if APT_USE_LOADVARS_OBJECT
    NATIVE_MEMBER_FUNCTION_DESTROY(load);
    NATIVE_MEMBER_FUNCTION_DESTROY(send);
    NATIVE_MEMBER_FUNCTION_DESTROY(sendAndLoad);
    NATIVE_MEMBER_FUNCTION_DESTROY(getBytesTotal);
    NATIVE_MEMBER_FUNCTION_DESTROY(getBytesLoaded);
    NATIVE_MEMBER_FUNCTION_DESTROY(toString);
#endif
}

#if APT_USE_LOADVARS_OBJECT
// Calls interpreter.loadVariables with this object as context, filling all the variables inside it.
NATIVE_MEMBER_FUNCTION(AptLoadVars, load)
{
    pThis->c_loadvars()->iIsLoaded = 0;
    if ((nParams <= 0) || (nParams > 1))
        return AptBoolean::Create(false);
    AptValue *pUrl = gAptActionInterpreter.stackAt(0);
    AptNativeString sBuf;
    pUrl->toString(sBuf);

    // first unset all the properties currently present in this loadVars object
    AptNativeHash *pObjHash = pThis->GetNativeHashVirtual();
    if (!pObjHash)
        return AptBoolean::Create(false);

    for (AptHashItem *pInitItem = pObjHash->GetFirstItem(); pInitItem; pInitItem = pObjHash->GetNextItem(pInitItem))
    {
        // TODO: theoretically, on every hash key there's supposed to be a DontEnum/DontDelete flag; we don't have that so hardcode some dontenum's here..
        if ((pInitItem->Key.Equal(*StringPool::GetString(SC___proto__))) ||
            (pInitItem->Key.Equal(*StringPool::GetString(SC_prototype))))
        {
            continue;
        }

        pObjHash->Unset(&pInitItem->Key);
    }

    // now get new variables from the auxiliary library.
    gAptActionInterpreter.loadVariables(pThis, NULL, &sBuf);
    pThis->c_loadvars()->iIsLoaded = 1;
    return AptBoolean::Create(true);
}

NATIVE_MEMBER_FUNCTION(AptLoadVars, send)
{
    if ((nParams <= 0) || (nParams > 3))
        return AptBoolean::Create(false);

    AptValue *pUrl = gAptActionInterpreter.stackAt(0);
    AptNativeString sUrl;
    pUrl->toString(sUrl);

    AptNativeString sTarget;
    if (nParams > 1)
    {
        AptValue *pTarget = gAptActionInterpreter.stackAt(1);
        pTarget->toString(sTarget);
    }

    AptNativeString sMethod;
    if (nParams > 2)
    {
        AptValue *pMethod = gAptActionInterpreter.stackAt(2);
        pMethod->toString(sMethod);
    }

    AptNativeString sProp = pThis->urlEncode();

    APT_ASSERT(AptGetUserFuncs().pfnSendVariables);
    {
        if (AptGetUserFuncs().pfnSendVariables != NULL)
            AptGetUserFuncs().pfnSendVariables(sUrl.ConstRawPtr(), sTarget.ConstRawPtr(), sMethod.ConstRawPtr(), sProp.ConstRawPtr(), 0);
    }

    return AptBoolean::Create(true);
}

NATIVE_MEMBER_FUNCTION(AptLoadVars, sendAndLoad)
{
    pThis->c_loadvars()->iIsLoaded = 0;
    if ((nParams <= 0) || (nParams > 3))
        return AptBoolean::Create(false);

    AptValue *pUrl = gAptActionInterpreter.stackAt(0);
    AptNativeString sUrl;
    pUrl->toString(sUrl);

    AptValue *pTarget = NULL;
    AptNativeString sTarget;
    if (nParams > 1)
    {
        pTarget = gAptActionInterpreter.stackAt(1);
    }

    AptValue *pMethod = NULL;
    AptNativeString sMethod;
    if (nParams > 2)
    {
        pMethod = gAptActionInterpreter.stackAt(2);
        pMethod->toString(sMethod);
    }

    AptNativeString sProp = pThis->urlEncode();

    APT_ASSERT(AptGetUserFuncs().pfnSendVariables);
    {
        if (AptGetUserFuncs().pfnSendVariables != NULL)
            AptGetUserFuncs().pfnSendVariables(sUrl.ConstRawPtr(), sTarget.ConstRawPtr(), sMethod.ConstRawPtr(), sProp.ConstRawPtr(), 1);
    }

    // first unset all the properties currently present in this loadVars object
    AptNativeHash *pObjHash = pThis->GetNativeHashVirtual();
    if (!pObjHash)
        return AptBoolean::Create(false);

    for (AptHashItem *pInitItem = pObjHash->GetFirstItem(); pInitItem; pInitItem = pObjHash->GetNextItem(pInitItem))
    {
        // TODO: theoretically, on every hash key there's supposed to be a DontEnum/DontDelete flag; we don't have that so hardcode some dontenum's here..
        if ((pInitItem->Key.Equal(*StringPool::GetString(SC___proto__))) ||
            (pInitItem->Key.Equal(*StringPool::GetString(SC_prototype))))
        {
            continue;
        }

        pObjHash->Unset(&pInitItem->Key);
    }
    // now call loadvariables to get result of send operation with a NULL parameter.
    gAptActionInterpreter.loadVariables(pTarget, NULL, NULL);
    pThis->c_loadvars()->iIsLoaded = 1;
    return AptBoolean::Create(true);
}
NATIVE_MEMBER_FUNCTION(AptLoadVars, getBytesTotal)
{
    APT_ASSERT(AptGetUserFuncs().pfnGetBytesTotal);
    float fFloat = (float)AptGetUserFuncs().pfnGetBytesTotal(NULL, AptGetBytesEnum_LoadVars);
    return AptFloat::Create(fFloat);
}
NATIVE_MEMBER_FUNCTION(AptLoadVars, getBytesLoaded)
{
    APT_ASSERT(AptGetUserFuncs().pfnGetBytesLoaded);
    float fFloat = (float)AptGetUserFuncs().pfnGetBytesLoaded(NULL, AptGetBytesEnum_LoadVars);
    return AptFloat::Create(fFloat);
}

NATIVE_MEMBER_FUNCTION(AptLoadVars, toString)
{
    AptString *pRet = AptString::Create();
    pRet->cpy(pThis->urlEncode().ConstRawPtr());
    return pRet;
}
#endif // #if APT_USE_LOADVARS_OBJECT

//------------------------------------------------------------------------------------------
// AptError object


AptValue *AptError::objectMemberLookup(AptValue *const pContext, const AptNativeString *const pName) const
{
    if (pName->Equal("message"))
    {
        AptString *t = AptString::Create();
        t->cpy(msMessage);
        return t;
    }
    else if (pName->Equal("name"))
    {
        AptString *t = AptString::Create();
        t->cpy(msName);
        return t;
    }
    else if (pName->Equal("toString"))
    {
        NATIVE_MEMBER_FUNCTION_DISPATCH(toString);
    }

#if defined(EA_DEBUG)
    if (
        (pName->EqualNoCase("message")) ||
        (pName->EqualNoCase("name")) ||
        (pName->EqualNoCase("toString")))
    {
        APT_DEBUGPRINT(APT_DEBUG_MSG_WARNING_LVL, "AptError: Incorrect case for '%s'.\n", pName->c_str());
        APT_ASSERT(0);
    }
#endif

    return 0;
}

bool AptError::objectMemberSet(AptValue *const pContext, const AptNativeString *const pName, AptValue *const pValue)
{
    if (pName->Equal("message"))
    {
        AptNativeString t;
        pValue->toString(t);
        msMessage.Duplicate(t);
        return true;
    }
    else if (pName->Equal("name"))
    {
        AptNativeString t;
        pValue->toString(t);
        msName.Duplicate(t);
        return true;
    }
#if defined(EA_DEBUG)
    if (
        (pName->EqualNoCase("message")) ||
        (pName->EqualNoCase("name")))
    {
        APT_DEBUGPRINT(APT_DEBUG_MSG_WARNING_LVL, "AptError: Incorrect case for '%s'.\n", pName->c_str());
        APT_ASSERT(0);
    }
#endif

    return 0;
}

AptError::~AptError()
{
    //  Do nothing...
}
NATIVE_MEMBER_FUNCTION(AptError, toString)
{
    AptString *pRet = AptString::Create();
    pRet->cpy(pThis->urlEncode().ConstRawPtr());
    return pRet;
}

void AptError::CleanNativeFunctions()
{
    NATIVE_MEMBER_FUNCTION_DESTROY(toString);
}

//------------------------------------------------------------------------------------------
// AptStringObject object


AptStringObject::~AptStringObject()
{
    APT_DECSAFE(mpStringObject);
    mpStringObject = NULL;
}

void AptStringObject::setString(AptString *pString)
{
    APT_INC(pString)
    mpStringObject = pString;
}

AptValue *AptStringObject::objectMemberLookup(AptValue *const pContext, const AptNativeString *const pName) const
{
    AptValue *pValue = mpStringObject->objectMemberLookup(pContext, pName);
    if (pValue == NULL)
    {
        pValue = AptObject::objectMemberLookup(pContext, pName);
    }
    return pValue;
}

//------------------------------------------------------------------------------------------
// AptTextFormat object


/**
 * Looks up a member of the TextFormat object by name.
 * @return the AptValue for the member, or gpUndefinedValue if unknown.
 */
AptValue *AptTextFormat::objectMemberLookup(AptValue *const pContext, const AptNativeString *const pName) const
{
    TextFormatMembers *pProp = pContext ? TextFormatMembersIndex::in_word_set(pName->c_str(), pName->Size()) : NULL;
    if (pProp)
    {
        switch (pProp->nIndex)
        {
        case (AptTextFormat_align):
        {
            AptString *pTemp = AptString::Create();
            switch (mTextFormat.eAlignment)
            {
            case AptStringAlignment_Left:
            {
                pTemp->cpy(StringPool::GetString(SC_left));
            }
            break;
            case AptStringAlignment_Right:
            {
                pTemp->cpy(StringPool::GetString(SC_right));
            }
            break;
            case AptStringAlignment_Center:
            {
                pTemp->cpy(StringPool::GetString(SC_center));
            }
            break;
            case AptStringAlignment_None:
            {
                return gpUndefinedValue;
            }
            break;
            default:
            {
                return gpUndefinedValue;
            }
            }
            return pTemp;
        }
        break;
        case AptTextFormat_bold:
        {
            if ((mTextFormat.nFontStyle & AptFontStyle_isBoldSet) != AptFontStyle_isBoldSet)
            {
                return gpUndefinedValue;
            }
            bool bIsBold = false;
            if ((mTextFormat.nFontStyle & AptFontStyle_Bold) == AptFontStyle_Bold)
            {
                bIsBold = true;
            }
            return AptBoolean::Create(bIsBold);
        }
        break;
        case AptTextFormat_color:
        {
            if (mTextFormat.nColor == -1)
            {
                return gpUndefinedValue;
            }
            return AptInteger::Create(mTextFormat.nColor & 0xffffff);
        }
        break;
        case AptTextFormat_font:
        {
            // If no font is present, return undefined rather than an empty string.
            if (mTextFormat.pFontName.IsEmpty())
                return gpUndefinedValue;

            AptString *pString = AptString::Create();
            pString->cpy(mTextFormat.pFontName);
            return pString;
        }
        break;
        case AptTextFormat_indent:
        {
            if (mTextFormat.nIndent == -1)
            {
                return gpUndefinedValue;
            }
            return AptInteger::Create(mTextFormat.nIndent);
        }
        break;
        case AptTextFormat_italic:
        {
            if ((mTextFormat.nFontStyle & AptFontStyle_isItalicSet) != AptFontStyle_isItalicSet)
            {
                return gpUndefinedValue;
            }
            bool bIsItalic = false;
            if ((mTextFormat.nFontStyle & AptFontStyle_Italic) == AptFontStyle_Italic)
            {
                bIsItalic = true;
            }
            return AptBoolean::Create(bIsItalic);
        }
        break;
        case AptTextFormat_leftMargin:
        {
            if (mTextFormat.nLeftMargin == -1)
            {
                return gpUndefinedValue;
            }
            return AptInteger::Create(mTextFormat.nLeftMargin);
        }
        break;
        case AptTextFormat_rightMargin:
        {
            if (mTextFormat.nRightMargin == -1)
            {
                return gpUndefinedValue;
            }
            return AptInteger::Create(mTextFormat.nRightMargin);
        }
        break;
        case AptTextFormat_leading:
        {
            if (mTextFormat.nLeading == TextFormat::UNDEFINED_LEADING_VALUE)
            {
                return gpUndefinedValue;
            }
            return AptInteger::Create(mTextFormat.nLeading);
        }
        break;
        case AptTextFormat_tracking:
        {
            if (mTextFormat.nTracking == TextFormat::UNDEFINED_TRACKING_VALUE)
            {
                return gpUndefinedValue;
            }
            return AptInteger::Create(mTextFormat.nTracking);
        }
        break;
        case AptTextFormat_size:
        {
            if (mTextFormat.fSize == -1.f)
            {
                return gpUndefinedValue;
            }
            return AptFloat::Create(mTextFormat.fSize);
        }
        break;
        case AptTextFormat_underline:
        {
            if ((mTextFormat.nFontStyle & AptFontStyle_isUnderlineSet) != AptFontStyle_isUnderlineSet)
            {
                return gpUndefinedValue;
            }
            bool bIsUnderline = false;
            if ((mTextFormat.nFontStyle & AptFontStyle_Underline) == AptFontStyle_Underline)
            {
                bIsUnderline = true;
            }
            return AptBoolean::Create(bIsUnderline);
        }
        }
    }

#if defined(EA_DEBUG)
    APT_ASSERT(!pName->EqualNoCase("blockIndent") && "TextFormat: blockIndent is unimplemented property.\n");
    APT_ASSERT(!pName->EqualNoCase("bullet") && "TextFormat: bullet is unimplemented property.\n");
    APT_ASSERT(!pName->EqualNoCase("leading") && "TextFormat: leading is unimplemented property.\n");
    APT_ASSERT(!pName->EqualNoCase("tabStops") && "TextFormat: tabStops is unimplemented property.\n");
    APT_ASSERT(!pName->EqualNoCase("target") && "TextFormat: target is unimplemented property.\n");
    APT_ASSERT(!pName->EqualNoCase("url") && "TextFormat: url is unimplemented property.\n");
#endif

    return 0;
}

/**
 * Sets a member of the TextFormat object by name.
 * @return whether the member was recognized and set.
 */
bool AptTextFormat::objectMemberSet(AptValue *const pContext, const AptNativeString *const pName, AptValue *const pValue)
{
    TextFormatMembers *pProp = pContext ? TextFormatMembersIndex::in_word_set(pName->c_str(), pName->Size()) : NULL;
    if (pProp)
    {
        switch (pProp->nIndex)
        {
        case (AptTextFormat_align):
        {
            AptNativeString szBuf;
            pValue->toString(szBuf);
            if (szBuf == "left" || szBuf == "true")
            {
                mTextFormat.eAlignment = AptStringAlignment_Left;
            }
            else if (szBuf == "center")
            {
                mTextFormat.eAlignment = AptStringAlignment_Center;
            }
            else if (szBuf == "right")
            {
                mTextFormat.eAlignment = AptStringAlignment_Right;
            }
            else if (szBuf == "false" || szBuf == "none")
            {
                mTextFormat.eAlignment = AptStringAlignment_None;
            }
            return (true);
        }
        case (AptTextFormat_color):
        {
            mTextFormat.nColor = (pValue->toInteger() & 0xffffff);
            return (true);
        }
        case (AptTextFormat_font):
        {
            pValue->toString(mTextFormat.pFontName);
            return (true);
        }
        case (AptTextFormat_blockIndent):
        {
            return (true);
        }
        case (AptTextFormat_bold):
        {
            AptNativeString szBuf;
            pValue->toString(szBuf);
            if (szBuf == "true")
            {
                mTextFormat.nFontStyle |= AptFontStyle_isBoldSet;
                mTextFormat.nFontStyle |= AptFontStyle_Bold;
            }

            if (szBuf == "false")
            {
                mTextFormat.nFontStyle |= AptFontStyle_isBoldSet;
                if ((mTextFormat.nFontStyle & AptFontStyle_Bold) == AptFontStyle_Bold)
                {
                    mTextFormat.nFontStyle ^= AptFontStyle_Bold;
                }
            }
            return (true);
        }
        case (AptTextFormat_bullet):
        {
            return (true);
        }
        case (AptTextFormat_indent):
        {
            mTextFormat.nIndent = pValue->toInteger();
            return (true);
        }
        case (AptTextFormat_italic):
        {
            AptNativeString szBuf;
            pValue->toString(szBuf);

            if (szBuf == "true")
            {
                mTextFormat.nFontStyle |= AptFontStyle_isItalicSet;
                mTextFormat.nFontStyle |= AptFontStyle_Italic;
            }
            else if (szBuf == "false")
            {
                mTextFormat.nFontStyle |= AptFontStyle_isItalicSet;
                if ((mTextFormat.nFontStyle & AptFontStyle_Italic) == AptFontStyle_Italic)
                {
                    mTextFormat.nFontStyle ^= AptFontStyle_Italic;
                }
            }
            else // Handle passing of anything other than true/false, e.g. undefined.
            {
                mTextFormat.nFontStyle |= AptFontStyle_Italic;
                mTextFormat.nFontStyle ^= AptFontStyle_Italic;
            }
            return (true);
        }
        case (AptTextFormat_leading):
        {
            if (pValue->getIsDefined())
            {
                mTextFormat.nLeading = pValue->toInteger();
            }
            else
            {
                mTextFormat.nLeading = TextFormat::UNDEFINED_LEADING_VALUE;
            }
            return (true);
        }
        case (AptTextFormat_tracking):
        {
            if (pValue->getIsDefined())
            {
                mTextFormat.nTracking = pValue->toInteger();
            }
            else
            {
                mTextFormat.nTracking = TextFormat::UNDEFINED_TRACKING_VALUE;
            }
            return (true);
        }
        case (AptTextFormat_leftMargin):
        {
            mTextFormat.nLeftMargin = pValue->toInteger();
            return (true);
        }
        case (AptTextFormat_rightMargin):
        {
            mTextFormat.nRightMargin = pValue->toInteger();
            return (true);
        }
        case (AptTextFormat_tabStops):
        case (AptTextFormat_target):
        {
            return (true);
        }
        case (AptTextFormat_underline):
        {
            AptNativeString szBuf;
            pValue->toString(szBuf);
            if (szBuf == "true")
            {
                mTextFormat.nFontStyle |= AptFontStyle_isUnderlineSet;
                mTextFormat.nFontStyle |= AptFontStyle_Underline;
            }
            else if (szBuf == "false")
            {
                mTextFormat.nFontStyle |= AptFontStyle_isUnderlineSet;
                if ((mTextFormat.nFontStyle & AptFontStyle_Underline) == AptFontStyle_Underline)
                {
                    mTextFormat.nFontStyle ^= AptFontStyle_Underline;
                }
            }
            else // Handle passing of anything other than true/false, e.g. undefined.
            {
                mTextFormat.nFontStyle |= AptFontStyle_isUnderlineSet;
                mTextFormat.nFontStyle ^= AptFontStyle_Underline;
            }
            return (true);
        }
        case (AptTextFormat_url):
        {
            return (true);
        }
        case (AptTextFormat_size):
        {
            mTextFormat.fSize = pValue->toFloat();
            return (true);
        }
        }
    }

#if defined(EA_DEBUG)
    if (
        (pName->EqualNoCase("blockIndent")) ||
        (pName->EqualNoCase("bullet")) ||
        (pName->EqualNoCase("leading")) ||
        (pName->EqualNoCase("tabStops")) ||
        (pName->EqualNoCase("target")) ||
        (pName->EqualNoCase("url")))
    {
        APT_DEBUGPRINT(APT_DEBUG_MSG_WARNING_LVL, "TextFormat: Incorrect case or unimplemented property '%s'.\n", pName->c_str());
        APT_ASSERT(0);
    }
#endif

    return (false);
}

//------------------------------------------------------------------------------------------
// AptUtil object

/** Frees the native functions cached by the AptUtil class. Must be called at least during AptShutdown. */
void AptUtil::CleanNativeFunctions()
{
#if APT_USE_UTILITY
    NATIVE_MEMBER_FUNCTION_DESTROY(formatNumberString);
    NATIVE_MEMBER_FUNCTION_DESTROY(replaceString);
    NATIVE_MEMBER_FUNCTION_DESTROY(trimString);
    NATIVE_MEMBER_FUNCTION_DESTROY(trimLeftString);
    NATIVE_MEMBER_FUNCTION_DESTROY(trimRightString);
    NATIVE_MEMBER_FUNCTION_DESTROY(searchArray);
    NATIVE_MEMBER_FUNCTION_DESTROY(reverseSearchArray);
    NATIVE_MEMBER_FUNCTION_DESTROY(countArray);
    NATIVE_MEMBER_FUNCTION_DESTROY(getAptVersion);
    NATIVE_MEMBER_FUNCTION_DESTROY(safeForIn);
    NATIVE_MEMBER_FUNCTION_DESTROY(convertHsvToColorTransform);
    NATIVE_MEMBER_FUNCTION_DESTROY(colorMatrixMultiply);
#endif
}

#if APT_USE_UTILITY

/**
 * Looks up a member of the AptUtil object by name.
 * @return the AptValue for the member, or gpUndefinedValue if unknown.
 */
AptValue *AptUtil::objectMemberLookup(AptValue *const pContext, const AptNativeString *const pName) const
{
    UtilMembers *pProp = pContext ? UtilMembersIndex::in_word_set(pName->c_str(), pName->Size()) : NULL;
    if (pProp)
    {
        switch (pProp->nIndex)
        {
        case AptUtil_formatNumberString:
            NATIVE_MEMBER_FUNCTION_DISPATCH(formatNumberString);
            break;
        case AptUtil_replaceString:
            NATIVE_MEMBER_FUNCTION_DISPATCH(replaceString);
            break;
        case AptUtil_trimString:
            NATIVE_MEMBER_FUNCTION_DISPATCH(trimString);
            break;
        case AptUtil_trimLeftString:
            NATIVE_MEMBER_FUNCTION_DISPATCH(trimLeftString);
            break;
        case AptUtil_trimRightString:
            NATIVE_MEMBER_FUNCTION_DISPATCH(trimRightString);
            break;
        case AptUtil_searchArray:
            NATIVE_MEMBER_FUNCTION_DISPATCH(searchArray);
            break;
        case AptUtil_reverseSearchArray:
            NATIVE_MEMBER_FUNCTION_DISPATCH(reverseSearchArray);
            break;
        case AptUtil_countArray:
            NATIVE_MEMBER_FUNCTION_DISPATCH(countArray);
            break;
        case AptUtil_getAptVersion:
            NATIVE_MEMBER_FUNCTION_DISPATCH(getAptVersion);
            break;
        case AptUtil_safeForIn:
            NATIVE_MEMBER_FUNCTION_DISPATCH(safeForIn);
            break;
        case AptUtil_convertHsvToColorTransform:
            NATIVE_MEMBER_FUNCTION_DISPATCH(convertHsvToColorTransform);
            break;
        case AptUtil_colorMatrixMultiply:
            NATIVE_MEMBER_FUNCTION_DISPATCH(colorMatrixMultiply);
            break;
        }
    }
    return 0;
}

/**
 * Formats a number into a pretty string: @p decimals digits appear after the decimal point, and
 * if there are at least @p minimumCommaSize digits to the left of the decimal point, groups of
 * three are separated with commas (e.g. 1000000.2 becomes "1,000,000.2").
 */
static void formatNumber(float number, int32_t decimals, int32_t minimumCommaSize, char *text)
{
    enum
    {
        MAX_STR_SIZE = 512,
        MAX_DECIMALS = 32
    };

    char *periodOrEnd = text;

    if (decimals <= 0)
    {
        int32_t chars = sprintf(text, "%d", (int32_t)number);
        periodOrEnd   = text + chars;
    }
    else
    {
        APT_ASSERT(decimals <= MAX_DECIMALS);

        if (decimals > MAX_DECIMALS)
        {
            decimals = MAX_DECIMALS;
        }
        int32_t chars = sprintf(text, "%.*f", decimals, number);
        periodOrEnd   = text + chars - decimals - 1; // -1 for the period
    }

    int32_t size       = periodOrEnd - text,
            prefixSize = 0;

    const char *p = text;
    while (*p && !isdigit(*p))
    {
        prefixSize++;
        p++;
    }

    if (minimumCommaSize >= 3 && (size - prefixSize) > minimumCommaSize)
    {
        char tmp[MAX_STR_SIZE];
        const char *p = text;
        char *q       = tmp;
        while (!isdigit(*p)) // copy minus sign, etc. coming before the number
        {
            *q++ = *p++;
        }
        *q++ = *p++; // copy first digit
        for (; p < periodOrEnd; p++, q++)
        {
            if ((periodOrEnd - p) % 3 == 0)
            {
                *q++ = ',';
            }
            *q = *p;
        }
        strcpy(q, p);
        strcpy(text, tmp);
    }
}

/**
 * Implements AptUtil.trimString/trimLeftString/trimRightString: trims the given string, either
 * on both ends, the left, or the right depending on @p type.
 * @param paramCount stack param 0 is the string to trim; stack param 1 is an optional string
 *  containing the characters to remove.
 * @return the trimmed string, or undefined if no string was given.
 */
AptValue *AptUtil::trim(Trim type, int32_t paramCount)
{
    bool failed            = true;
    AptNativeString *text  = 0;
    const char *characters = " \t\r\n\f";

    ASSERT_IF_TOO_MANY_DEFINED_ARGS(2, paramCount, "Sent too many args to AptUtil.trim*()! It expects (text:String, characters:String=' \t\r\n\f'):String");

    if (paramCount >= 1)
    {
        AptValue *pText  = gAptActionInterpreter.stackAt(0),
                 *pChars = paramCount >= 2 ? gAptActionInterpreter.stackAt(1) : 0;

        if (pText->isString())
        {
            if ((text = static_cast<AptString *>(pText)->GetInternalString()) != 0)
            {
                failed = false;
            }
            AptNativeString *chars = pChars ? static_cast<AptString *>(pChars)->GetInternalString() : 0;
            if (chars)
            {
                characters = chars->GetBuffer();
            }
        }
    }

    if (failed)
    {
        APT_ASSERTM(false, "AptUtil.trim*() expects (text:String, characters:String=' \t\r\n\f'):String");

        return gpUndefinedValue;
    }

    AptString *newString = AptString::Create();
    newString->cpy(text);

    switch (type)
    {
    case LEFT:
        newString->GetInternalString()->TrimLeft(characters);
        break;
    case RIGHT:
        newString->GetInternalString()->TrimRight(characters);
        break;
    default:
        newString->GetInternalString()->Trim(characters);
        break;
    }

    return newString;
}

namespace
{

/**
 * Refactors common functionality shared by searchArray() and countArray(): converts the
 * params passed to the ActionScript function into AptValues.
 * @param nParams stack param 0: array to search; param 1: index to start searching at;
 *  param 2: value to look for; param 3: optional property to search for, if the array
 *  contains Objects.
 * @return the index to start searching at.
 */
static int32_t GetArraySearchParams(
    int32_t nParams,
    AptArray **ppArray,
    AptValue **ppEquals,
    AptValue **ppProperty)
{
    bool failed        = true;
    AptArray *array    = 0;
    AptValue *equals   = 0,
             *property = 0;
    int32_t start      = 0;

    ASSERT_IF_TOO_MANY_DEFINED_ARGS(4, nParams, "Sent too many args to AptUtil.countArray/searchArray()! It expects (array:Array, start:Number, equals:Object, property:Object=undefined):Number");

    if (nParams >= 3)
    {
        AptValue *pArray = gAptActionInterpreter.stackAt(0),
                 *pStart = gAptActionInterpreter.stackAt(1);
        equals           = gAptActionInterpreter.stackAt(2),
        property         = nParams > 3 ? gAptActionInterpreter.stackAt(3) : 0;

        if (equals && pArray && pArray->isArray() && pStart && pStart->isNumber())
        {
            failed = false;
            array  = static_cast<AptArray *>(pArray);
            start  = AptValueHelper::NumberToInteger(pStart);

            // clamp start number between 0 and array length
            int32_t arrLength = pArray->c_array()->length();
            if (start < 0 || start >= arrLength)
            {
                // Don't assert on startIndex 0 even when it is technically outside the valid range,
                // since searching an empty array should be okay.
                if (0 != start)
                {
                    APT_ASSERTM(false, "AptUtil.countArray/searchArray() expects start position between 0 and array length");
                }

                if (start >= arrLength)
                {
                    start = arrLength - 1;
                }
                else
                {
                    start = 0;
                }
            }
        }
    }

    if (failed)
    {
        APT_ASSERTM(false, "AptUtil.countArray/searchArray() expects (array:Array, start:Number, equals:Object, property:Object=undefined):Number");

        return -1;
    }

    *ppArray    = array;
    *ppEquals   = equals;
    *ppProperty = property;
    return start;
}
} // namespace

/**
 * Implements AptUtil.searchArray/reverseSearchArray: searches for a value in an array (or a
 * property of an Object, if the array contains Objects).
 * @param reverse whether to search backwards instead of forwards.
 * @param paramCount stack param 0: array to search; param 1: index to start searching at;
 *  param 2: value to look for; param 3: optional property to search for.
 * @return the index of the matching item, or -1 if not found.
 */
AptValue *AptUtil::search(bool reverse, int32_t paramCount)
{
    AptArray *array    = 0;
    AptValue *equals   = 0,
             *property = 0;

    int32_t start = GetArraySearchParams(paramCount, &array, &equals, &property);

    return AptInteger::Create(start < 0 ? -1 : array->find(AptArray::objectFindComparator, start, equals, property, reverse));
}

/**
 * Formats a number to a specific number of decimal places, leaving integers alone and
 * truncating floats.
 * @param stack param 0 the number to format; param 1 the number of decimal places; param 2
 *  optional, if true inserts commas into the thousands place.
 * @return the formatted string.
 */
NATIVE_MEMBER_FUNCTION(AptUtil, formatNumberString)
{
    bool failed      = false;
    float number     = 0.f;
    int32_t decimals = 0;
    bool commas      = false;

#if (APT_CURR_DEBUG_MSG_LVL >= APT_DEBUG_MSG_ASSERT_LVL)
    const char *BAD_ARGS_MSG = "AptUtil.formatNumberString() expects (number:Number, decimals:Number) or (number:Number, decimals:Number, commas:Boolean)";
#endif

    ASSERT_IF_TOO_MANY_DEFINED_ARGS(3, nParams, "Sent too many args to AptUtil.formatNumberString()! It expects (number:Number, decimals:Number) or (number:Number, decimals:Number, commas:Boolean)");

    if (2 <= nParams)
    {
        AptValue *pNumber   = gAptActionInterpreter.stackAt(0);
        AptValue *pDecimals = gAptActionInterpreter.stackAt(1);

        if (pNumber->isNumber() && pDecimals->isNumber())
        {
            number   = AptValueHelper::NumberToFloat(pNumber);
            decimals = AptValueHelper::NumberToInteger(pDecimals);

            if ((number > (float)0x7FFFFFFF) || (number < -(float)0x7FFFFFFF))
            {
                APT_ASSERTM(false, ("AptUtil.formatNumberString(): number outside of valid range ([>= -2^31 and < 2^31])"));
                failed = true;
            }
            if (decimals < 0)
            {
                APT_ASSERTM(false, ("AptUtil.formatNumberString(): number of decimal places should be non-negative"));
                failed = true;
            }
        }
        else
        {
            APT_ASSERTM(false, (BAD_ARGS_MSG));
            failed = true;
        }

        if ((!failed) && (3 <= nParams))
        {
            AptValue *pCommas = gAptActionInterpreter.stackAt(2);
            if (pCommas->isBoolean())
            {
                commas = pCommas->toBool();
            }
            else if (pCommas->isUndefined())
            {
                // treat undefined the same as if it was not provided at all
            }
            else
            {
                APT_ASSERTM(false, (BAD_ARGS_MSG));
                failed = true;
            }
        }
    }
    else
    {
        APT_ASSERTM(false, (BAD_ARGS_MSG));
        failed = true;
    }

    if (failed)
    {
        return gpUndefinedValue;
    }

    enum
    {
        MAX_STR_SIZE = 512
    };

    char text[MAX_STR_SIZE];

    formatNumber(number, decimals, commas ? 3 : -1, text);

    return AptString::Create(text);
}

/**
 * Finds and replaces all instances of a substring, returning the new string.
 * @param stack param 0 the original text; param 1 the text to find; param 2 the text to replace
 *  it with; param 3 optional, if true ignores case (defaults to false).
 * @return the new string.
 */
NATIVE_MEMBER_FUNCTION(AptUtil, replaceString)
{
    bool failed           = true;
    AptNativeString *text = 0;
    const char *find      = 0,
               *replace   = 0;
    bool ignoreCase       = false;

    ASSERT_IF_TOO_MANY_DEFINED_ARGS(4, nParams, "Sent too many args to AptUtil.replaceString()! It expects (text:String, find:String, replace:String, ignoreCase:Boolean=false):String");

    if (nParams >= 3)
    {
        AptValue *pText    = gAptActionInterpreter.stackAt(0),
                 *pFind    = gAptActionInterpreter.stackAt(1),
                 *pReplace = gAptActionInterpreter.stackAt(2);

        if (pText->isString() && pFind->isString() && pReplace->isString())
        {
            text    = static_cast<AptString *>(pText)->GetInternalString();
            find    = static_cast<AptString *>(pFind)->GetInternalString()->GetBuffer();
            replace = static_cast<AptString *>(pReplace)->GetInternalString()->GetBuffer();

            if (text && find && replace)
            {
                if (nParams >= 4)
                {
                    AptValue *pIgnoreCase = gAptActionInterpreter.stackAt(3);
                    if (pIgnoreCase->isBoolean())
                    {
                        ignoreCase = pIgnoreCase->toBool();
                        failed     = false;
                    }
                }
                else
                {
                    failed = false;
                }
            }
        }
    }

    APT_ASSERTM((!failed), ("AptUtil.replaceString() expects (text:String, find:String, replace:String, ignoreCase:Boolean=false):String"));

    if (failed)
    {
        return gpUndefinedValue;
    }

    AptString *newString = AptString::Create();
    newString->cpy(text);
    newString->GetInternalString()->Replace(find, replace, ignoreCase);

    return newString;
}

/** C++ wrapper for AptUtil.trimString: trims both ends of the given string. */
NATIVE_MEMBER_FUNCTION(AptUtil, trimString)
{
    return trim(BOTH, nParams);
}

/** C++ wrapper for AptUtil.trimLeftString: trims only the left side of the given string. */
NATIVE_MEMBER_FUNCTION(AptUtil, trimLeftString)
{
    return trim(LEFT, nParams);
}

/** C++ wrapper for AptUtil.trimRightString: trims only the right side of the given string. */
NATIVE_MEMBER_FUNCTION(AptUtil, trimRightString)
{
    return trim(RIGHT, nParams);
}

/** C++ wrapper for AptUtil.searchArray: searches forward for a value (or Object property) in an array. */
NATIVE_MEMBER_FUNCTION(AptUtil, searchArray)
{
    return search(false, nParams);
}

/** C++ wrapper for AptUtil.reverseSearchArray: searches backward for a value (or Object property) in an array. */
NATIVE_MEMBER_FUNCTION(AptUtil, reverseSearchArray)
{
    return search(true, nParams);
}

/**
 * Implements AptUtil.countArray: counts the number of times a value (or Object property) occurs
 * in an array.
 */
NATIVE_MEMBER_FUNCTION(AptUtil, countArray)
{
    AptArray *array    = 0;
    AptValue *equals   = 0,
             *property = 0;

    int32_t count = 0;
    int32_t start = GetArraySearchParams(nParams, &array, &equals, &property);
    if (start >= 0)
    {
        start--;
        while ((start = array->find(AptArray::objectFindComparator, start + 1, equals, property, false)) != -1)
        {
            count++;
        }
    }
    return AptInteger::Create(count);
}

namespace
{
static inline int32_t ExtractDecimal(int32_t iValue, int32_t iShifts)
{
    iValue = (iValue >> iShifts) & 0xff;        // Return iValue directly if 2.10 is represented as 0x20a0000
    return (iValue & 0xf) + (iValue >> 4) * 10; // Conversion if 2.10 is represented as 0x2100000
}
} // namespace

/**
 * Implements AptUtil.getAptVersion: returns the components of the Apt version number as Object
 * properties (Major, Minor, SubMinor, Patch, and the string property Version).
 */
NATIVE_MEMBER_FUNCTION(AptUtil, getAptVersion)
{
    enum
    {
        FORMATTED_VERSION_SIZE = 32,
        MAJOR_SHIFTS           = 24,
        MINOR_SHIFTS           = 16,
        SUBMINOR_SHIFTS        = 8,
        PATCH_SHIFTS           = 0
    };

    AptObject *pObject = new AptObject(AptVFT_Object);

    static const char majorPropertyName[]    = "Major";
    static const char minorPropertyName[]    = "Minor";
    static const char subminorPropertyName[] = "SubMinor";
    static const char patchPropertyName[]    = "Patch";
    static const char versionPropertyName[]  = "Version";

    int32_t iMajor    = ExtractDecimal(APT_VERSION, MAJOR_SHIFTS);
    int32_t iMinor    = ExtractDecimal(APT_VERSION, MINOR_SHIFTS);
    int32_t iSubminor = ExtractDecimal(APT_VERSION, SUBMINOR_SHIFTS);
    int32_t iPatch    = ExtractDecimal(APT_VERSION, PATCH_SHIFTS);

    AptNativeString pTmpStr = majorPropertyName;
    pObject->Set(&pTmpStr, AptInteger::Create(iMajor));

    pTmpStr = minorPropertyName;
    pObject->Set(&pTmpStr, AptInteger::Create(iMinor));

    pTmpStr = subminorPropertyName;
    pObject->Set(&pTmpStr, AptInteger::Create(iSubminor));

    pTmpStr = patchPropertyName;
    pObject->Set(&pTmpStr, AptInteger::Create(iPatch));

    pTmpStr = versionPropertyName;
    char compoundVersionStr[FORMATTED_VERSION_SIZE];
    sprintf(compoundVersionStr, (iPatch > 0) ? "%d.%02d.%02d-%d" : "%d.%02d.%02d", iMajor, iMinor, iSubminor, iPatch);
    pObject->Set(&pTmpStr, AptString::Create(compoundVersionStr));

    return pObject;
}

/**
 * Implements AptUtil.colorMatrixMultiply: multiplies two 4x5 matrices, ~98% faster than the
 * equivalent ActionScript, which makes animating the ColorMatrix ~25% faster overall.
 * @param stack param 0 matrix A; param 1 matrix B.
 * @return an AptArray with the multiplied result.
 */
NATIVE_MEMBER_FUNCTION(AptUtil, colorMatrixMultiply)
{
    const int kArraySize = 5 * 4;

    AptValue *pOutputMatrix[kArraySize];
    float pMatrixA[kArraySize];
    float pMatrixB[kArraySize];

    AptValue *pFilterA = gAptActionInterpreter.stackAt(0);
    AptValue *pFilterB = gAptActionInterpreter.stackAt(1);

    for (int32_t i = 0; i < kArraySize; i++)
    {
        pMatrixA[i] = static_cast<AptArray *>(pFilterA)->get(i)->toFloat();
        pMatrixB[i] = static_cast<AptArray *>(pFilterB)->get(i)->toFloat();
    }

    // There is no existing 5x4 mult func around; AptMath may be a better home for this if needed elsewhere.
    float col[kArraySize];
    float tempVal;
    int32_t i;
    int32_t j;
    int32_t k;

    for (i = 0; i < 4; i++)
    {
        for (j = 0; j < 5; j++)
        {
            col[j] = pMatrixB[j + i * 5];
        }
        for (j = 0; j < 4; j++)
        {
            tempVal = 0;
            for (k = 0; k < 5; k++)
            {
                tempVal += pMatrixA[j + k * 5] * col[k];
            }
            pMatrixB[j + i * 5] = tempVal;
        }
    }

    for (int32_t i = 0; i < kArraySize; i++)
    {
        pOutputMatrix[i] = AptFloat::Create(pMatrixB[i]);
    }

    return (AptValueFactory::CreateArray(kArraySize, pOutputMatrix));
}



/**
 * Helper for AptUtil.safeForIn: given (someObject, someFunction), calls
 * someFunction(someObject, key, value) for each key in someObject. Stops iterating early if
 * someFunction returns false.
 */
void _SafeForIn(AptValue *object, AptValue *func)
{
    bool keepGoing             = true;
    AptNativeHash *pNativeHash = object->GetNativeHashVirtual();
    while (pNativeHash && keepGoing)
    {
        for (AptHashItem *pItem = pNativeHash->GetFirstItem(); pItem; pItem = pNativeHash->GetNextItem(pItem))
        {
            if ((pItem->Key.EqualNoCase(*StringPool::GetString(SC___proto__))) ||
                (pItem->Key.EqualNoCase(*StringPool::GetString(SC_prototype))))
            {
                continue;
            }
            AptString *pString = AptString::Create();
            pString->cpy(&pItem->Key);
            AptValue *retVal = AptCallFunctionObject(func, 3, object, pString, pItem->mValue);

            if (!retVal->isUndefined() && !retVal->toBool())
            {
                APT_DEC(retVal);
                keepGoing = false;
                break;
            }

            APT_DEC(retVal);
        }
        AptValue *pProto = pNativeHash->Get__Proto__();
        pNativeHash      = pProto ? pProto->GetNativeHashVirtual() : 0;
    }
}

/**
 * Implements AptUtil.safeForIn: given (someObject, someFunction), calls
 * someFunction(someObject, key, value) for each key in someObject, stopping early if
 * someFunction returns false.
 */
NATIVE_MEMBER_FUNCTION(AptUtil, safeForIn)
{
    ASSERT_IF_TOO_MANY_DEFINED_ARGS(2, nParams, "Sent too many args to AptUtil.safeForIn()! It expects (someObject:Object, someFunction:Function=):String");

    _SafeForIn(gAptActionInterpreter.stackAt(0), gAptActionInterpreter.stackAt(1));
    return gpUndefinedValue;
}

/**
 * Implements AptUtil.convertHsvToColorTransform: given (hue, saturation, brightness), returns a
 * transform Object usable with Color.setTransform.
 */
NATIVE_MEMBER_FUNCTION(AptUtil, convertHsvToColorTransform)
{
    ASSERT_IF_TOO_MANY_DEFINED_ARGS(3, nParams, "Sent too many args to AptUtil.transformFromHsv()! It expects (hue:Number, saturation:Number, brightness:Number");

    AptObject *transform = new AptObject(AptVFT_Object);

    AptValue *hueVal        = gAptActionInterpreter.stackAt(0);
    AptValue *saturationVal = gAptActionInterpreter.stackAt(1);
    AptValue *brightnessVal = gAptActionInterpreter.stackAt(2);

    float hue        = hueVal->toFloat();
    float saturation = saturationVal->toFloat();
    float brightness = brightnessVal->toFloat();

#if defined(APT_USE_FLASH_COLOR_RANGE)
    float redVal   = 0.f;
    float greenVal = 0.f;
    float blueVal  = 0.f;
#else
    int redVal   = 0;
    int greenVal = 0;
    int blueVal  = 0;
#endif

    if (0.f < brightness)
    {
        // Ensure hue is in the range [0, 360).
        hue = fmodf(hue, 360.f);
        if (hue < 0.f)
        {
            hue += 360.f;
        }

        // See http://en.wikipedia.org/wiki/HSL_and_HSV#From_HSV for algorithm details.
        saturation /= 100.f;
        brightness /= 100.f;
        hue /= 60.f;

        int i   = static_cast<int>(floor(hue));
        float f = hue - i;
        float p = brightness * (1.f - saturation);
        float q = brightness * (1.f - (saturation * f));
        float t = brightness * (1.f - (saturation * (1.f - f)));

        float red, green, blue;
        switch (i)
        {
        case 0:
            red   = brightness;
            green = t;
            blue  = p;
            break;

        case 1:
            red   = q;
            green = brightness;
            blue  = p;
            break;

        case 2:
            red   = p;
            green = brightness;
            blue  = t;
            break;

        case 3:
            red   = p;
            green = q;
            blue  = brightness;
            break;

        case 4:
            red   = t;
            green = p;
            blue  = brightness;
            break;

        case 5:
            red   = brightness;
            green = p;
            blue  = q;
            break;

        default:
            // Not reachable: hue is in [0, 360), divided by 60 and floored. Kept to satisfy the compiler.
            red = green = blue = 0.f;
            break;
        }

        // Simple rounding math to get within range. APT_USE_FLASH_COLOR_RANGE maxes out at 100
        // instead of 255.
#if defined(APT_USE_FLASH_COLOR_RANGE)
        redVal   = floor(red * 100.f + 0.5f);
        greenVal = floor(green * 100.f + 0.5f);
        blueVal  = floor(blue * 100.f + 0.5f);
#else
        redVal   = static_cast<int>(floor(red * 255.f + 0.5f));
        greenVal = static_cast<int>(floor(green * 255.f + 0.5f));
        blueVal  = static_cast<int>(floor(blue * 255.f + 0.5f));
#endif
    }

    // Return an object like { ra: redValue, ga: greenValue, ba: blueValue }, matching what
    // Color.setTransform takes.
#if defined(APT_USE_FLASH_COLOR_RANGE)
    transform->Set(StringPool::GetString(SC_ra), AptFloat::Create(redVal));
    transform->Set(StringPool::GetString(SC_ga), AptFloat::Create(greenVal));
    transform->Set(StringPool::GetString(SC_ba), AptFloat::Create(blueVal));
#else
    transform->Set(StringPool::GetString(SC_ra), AptInteger::Create(redVal));
    transform->Set(StringPool::GetString(SC_ga), AptInteger::Create(greenVal));
    transform->Set(StringPool::GetString(SC_ba), AptInteger::Create(blueVal));
#endif
    return transform;
}
#endif // APT_USE_UTILITY

//----------------------------------------------------------------------
#if defined(APT_USE_MOUSE)
#include "AptObject/AptMouse.h"

/** Releases the AptNativeScript functions; must be called during AptShutdown. */
void AptMouse::CleanNativeFunctions()
{
    NATIVE_MEMBER_FUNCTION_DESTROY(addListener);
    NATIVE_MEMBER_FUNCTION_DESTROY(removeListener);
}

AptValue *AptMouse::objectMemberLookup(AptValue *const pContext, const AptNativeString *const pName) const
{
    if (*pName == "addListener")
    {
        NATIVE_MEMBER_FUNCTION_DISPATCH(addListener);
    }
    else if (*pName == "removeListener")
    {
        NATIVE_MEMBER_FUNCTION_DISPATCH(removeListener);
    }
    return 0;
}

// link to these listener objects may remain in mListenerSet even if sometimes object is
// geting deleted/out of scope, and because there is a link to the object in this
// listenerset, its refcount will be atleast 1 ans so it won't get deleted and might stay
// active in memory.
// actionscript User has to take care of calling removeListener whenever the scope of object is getting
// finished, this will remove object from listenerset and decrement refcount, which might finally end
// up in correctly deleting the object.
NATIVE_MEMBER_FUNCTION(AptMouse, addListener)
{
    if (nParams != 1)
        return gpUndefinedValue;

    AptValue *pParam = gAptActionInterpreter.stackAt(0);
    if (pParam->getIsDefined())
    {
        if (pParam->isCIH()) // don't add to listener set if the CIH is a Zombie.
        {
            if (((AptCIH *)pParam)->GetCIHState() != AptCIH::AptCIHState_Normal)
            {
                return gpUndefinedValue;
            }
        }
        // object is defined and valid so now add it to mListenerSet in GetTargetSim()->GetAnimationTarget().
        if (!GetTargetSim()->GetAnimationTarget()->GetMouseListenerSet()->has(pParam))
            GetTargetSim()->GetAnimationTarget()->GetMouseListenerSet()->add(pParam);
    }

    return gpUndefinedValue;
}

// actionscript programmer should remember to call this function when object is going out of scope
// Note : if same object is also listening to some other non-key events like mouseUp etc then
// even in that case the object will be removed from the mListenerSet and object willnot be
// listening to any more event. This is because currently there is only one listenerset
// for all types of events.
NATIVE_MEMBER_FUNCTION(AptMouse, removeListener)
{
    if (nParams != 1)
        return AptBoolean::Create(false);

    AptValue *pParam = gAptActionInterpreter.stackAt(0);
    if (pParam->getIsDefined())
    {
        // object is defined and valid so now remove it from listener set
        if (GetTargetSim()->GetAnimationTarget()->GetMouseListenerSet()->has(pParam))
        {
            GetTargetSim()->GetAnimationTarget()->GetMouseListenerSet()->remove(pParam);
            return AptBoolean::Create(true);
        }
    }

    return AptBoolean::Create(false);
}
#endif // APT_USE_MOUSE
