/*** Include files ********************************************************************************/
#include "AptValueHelper.h"
#include "AptValue.h"
#include "_Apt.h"
#include "MainInline.h"

#include "AptObject/AptGlobalObject.h"
#include "AptTarget.h"
#include "AptAnimationTarget.h"
#include "AptCIH.h"
#include "AptCharacterInst.h"

#if defined(APT_DEBUGGER_ENABLE)
#include "AptDebugger/AptDebugger.h"
#endif

/*** Defines **************************************************************************************/

/*** Macros ***************************************************************************************/

/*** Type Definitions *****************************************************************************/

/*** Function Prototypes **************************************************************************/

/*** Variables ************************************************************************************/

// Private variables

// Public variables

/*** Private Functions ****************************************************************************/

/*** Public Functions *****************************************************************************/

bool AptValueHelper::GetMember(AptValue *pValue, const char *szValue, int32_t *pOutInteger)
{
    APT_ASSERT(pOutInteger != NULL);

    AptUpdateAutoLock lock;

    if (pValue->ContainsNativeHashVirtual() == false || pOutInteger == NULL)
    {
        return (false);
    }

    AptNativeString keyName(szValue);
    AptValue *pKey = pValue->GetNativeHashVirtual()->Lookup(&keyName);
    if (pKey == NULL)
    {
        return (false);
    }
    *pOutInteger = pKey->toInteger();

    return true;
}

bool AptValueHelper::GetMember(AptValue *pValue, const char *szValue, float_t *pOutFloat)
{
    APT_ASSERT(pOutFloat != NULL);

    AptUpdateAutoLock lock;

    if (pValue->ContainsNativeHashVirtual() == false || pOutFloat == NULL)
    {
        return (false);
    }

    AptNativeString keyName(szValue);
    AptValue *pKey = pValue->GetNativeHashVirtual()->Lookup(&keyName);
    if (pKey == NULL)
    {
        return (false);
    }
    *pOutFloat = pKey->toFloat();

    return true;
}

bool AptValueHelper::GetMember(AptValue *pValue, const char *szValue, bool *pOutBool)
{
    APT_ASSERT(pOutBool != NULL);

    AptUpdateAutoLock lock;

    if (pValue->ContainsNativeHashVirtual() == false || pOutBool == NULL)
    {
        return (false);
    }

    AptNativeString keyName(szValue);
    AptValue *pKey = pValue->GetNativeHashVirtual()->Lookup(&keyName);
    if (pKey == NULL)
    {
        return (false);
    }
    *pOutBool = pKey->toBool();

    return true;
}

// it is assumed that pOutString has enough bytes reserved for strcpy
bool AptValueHelper::GetMember(AptValue *pValue, const char *szValue, char *pOutString, uint32_t nMaxChars)
{
    APT_ASSERT(pOutString != NULL);

    AptUpdateAutoLock lock;

    if (pValue->ContainsNativeHashVirtual() == false || pOutString == NULL)
    {
        return (false);
    }

    AptNativeString keyName(szValue);
    AptValue *pKey = pValue->GetNativeHashVirtual()->Lookup(&keyName);
    if (pKey == NULL)
    {
        return (false);
    }

    if (pKey->isString())
    {
        // don't create a new AptNativeString if it's already a string
        strncpy((char *)pOutString, pKey->c_string()->GetInternalString()->c_str(), nMaxChars);
    }
    else
    {
        AptNativeString stringValue;
        pKey->toString(stringValue);
        strncpy((char *)pOutString, stringValue.c_str(), nMaxChars);
    }

    return true;
}

bool AptValueHelper::GetMember(AptValue *pValue, const AptNativeString *pStringKey, int32_t *pOutInteger)
{
    APT_ASSERT(pOutInteger != NULL);

    AptUpdateAutoLock lock;

    if (pValue->ContainsNativeHashVirtual() == false || pOutInteger == NULL)
    {
        return (false);
    }

    AptValue *pKey = pValue->GetNativeHashVirtual()->Lookup(pStringKey);
    if (pKey == NULL)
    {
        return (false);
    }
    *pOutInteger = pKey->toInteger();

    return true;
}

bool AptValueHelper::GetMember(AptValue *pValue, const AptNativeString *pStringKey, float_t *pOutFloat)
{
    APT_ASSERT(pOutFloat != NULL);

    AptUpdateAutoLock lock;

    if (pValue->ContainsNativeHashVirtual() == false || pOutFloat == NULL)
    {
        return (false);
    }

    AptValue *pKey = pValue->GetNativeHashVirtual()->Lookup(pStringKey);
    if (pKey == NULL)
    {
        return (false);
    }
    *pOutFloat = pKey->toFloat();

    return true;
}

bool AptValueHelper::GetMember(AptValue *pValue, const AptNativeString *pStringKey, bool *pOutBool)
{
    APT_ASSERT(pOutBool != NULL);

    AptUpdateAutoLock lock;

    if (pValue->ContainsNativeHashVirtual() == false || pOutBool == NULL)
    {
        return (false);
    }

    AptValue *pKey = pValue->GetNativeHashVirtual()->Lookup(pStringKey);
    if (pKey == NULL)
    {
        return (false);
    }
    *pOutBool = pKey->toBool();

    return true;
}

// it is assumed that pOutString has enough bytes reserved for strcpy
bool AptValueHelper::GetMember(AptValue *pValue, const AptNativeString *pStringKey, char *pOutString, uint32_t nMaxChars)
{
    APT_ASSERT(pOutString != NULL);

    AptUpdateAutoLock lock;

    if (pValue->ContainsNativeHashVirtual() == false || pOutString == NULL)
    {
        return (false);
    }

    AptValue *pKey = pValue->GetNativeHashVirtual()->Lookup(pStringKey);
    if (pKey == NULL)
    {
        return (false);
    }

    if (pKey->isString())
    {
        // don't create a new AptNativeString if it's already a string
        strncpy((char *)pOutString, pKey->c_string()->GetInternalString()->c_str(), nMaxChars);
    }
    else
    {
        AptNativeString stringValue;
        pKey->toString(stringValue);
        strncpy((char *)pOutString, stringValue.c_str(), nMaxChars);
    }

    return true;
}

/******************************************************************************/
/**
    AptValueHelper::GetUndefined

    @brief  Returns AptValue* associated with "undefined"

    @param  output  - Output for undefined
    @return         - void.

    Call APT_DEC on the output when you are done with it.
*/
/******************************************************************************/
void AptValueHelper::GetUndefined(AptValue **output)
{
    AptUpdateAutoLock lock;

    *output = gpUndefinedValue;
    APT_INC(*output);
}

/******************************************************************************/
/**
    AptValueHelper::GetGlobal

    @brief  Returns AptValue* associated with _global

    @param  output  - Output for _global
    @return         - void.

    Call APT_DEC on the output when you are done with it.
*/
/******************************************************************************/
void AptValueHelper::GetGlobal(AptValue **output)
{
    AptUpdateAutoLock lock;

    *output = AptGetLib()->mpGlobalGlobalObject;
    APT_INC(*output);
}

/******************************************************************************/
/**
    AptValueHelper::GetThis

    @brief  Returns AptValue* associated with this

    @param  currentContext -- Current function runtime context
    @param  output         -- Output for this

    @note  This function is only available when AptDebugger is enabled.
*/
/******************************************************************************/
void AptValueHelper::GetThis(AptValue *currentContext, AptValue **output, int level)
{
#ifdef APT_DEBUGGER_ENABLE
    if (AptDebugger::GetInstance()->IsStopped())
    {
        *output = AptDebugger::GetInstance()->GetCallStack()->GetThisObject(level);
    }
    else
    {
        *output = gpUndefinedValue;
    }
#else
    *output = gpUndefinedValue;
#endif
}

/******************************************************************************/
/**
    AptValueHelper::GetSuper

    @brief  Returns AptValue* associated with super

    @param  currentContext  - Current function runtime context
    @param  output          - Output for super
    @param  callStackLevel  - Call stack level

    @note  This function is only available when AptDebugger is enabled.
*/
/******************************************************************************/
void AptValueHelper::GetSuper(AptValue *currentContext, AptValue **output, int level)
{
#ifdef APT_DEBUGGER_ENABLE
    if (AptDebugger::GetInstance()->IsStopped())
    {
        *output = AptDebugger::GetInstance()->GetCallStack()->GetThisObject(level);
    }
    else
    {
        *output = gpUndefinedValue;
    }
#else
    *output = gpUndefinedValue;
#endif
}

/******************************************************************************/
/**
    AptValueHelper::GetLevel

    @brief  Returns AptValue* associated with the given level
    Returns NULL if _level n does not yet exist

    @param  n       - Level number
    @param  output  - Output for the level
    @return         - void.

    Call APT_DEC on the output when you are done with it.
*/
/******************************************************************************/
void AptValueHelper::GetLevel(int n, AptValue **output)
{
    AptUpdateAutoLock lock;
    *output = NULL;

    if (GetTargetSim() &&
        GetTargetSim()->GetAnimationTarget() &&
        GetTargetSim()->GetAnimationTarget()->GetDisplayList() &&
        GetTargetSim()->GetAnimationTarget()->GetDisplayList()->pState)
    {
        AptCIH *pCur = GetTargetSim()->GetAnimationTarget()->GetDisplayList()->pState->GetFirstItem();

        while (pCur && !*output)
        {
            if (pCur->GetDepth() == n)
            {
                *output = pCur;
                APT_INC(*output);
            }
            pCur = pCur->GetDisplayListNext();
        }
    }
}

/******************************************************************************/
/**
    AptValueHelper::GetMember

    @brief  Returns a child member of pValue, or NULL if it doesn't exist. Recurses;
            member name szValue can have a period in it.

    @param  instance - Starting instance
    @param  name     - Name of the member you wish, can have .s in it
    @param  output  - Output for the member
    @param  callStackLevel  - Ordered call stack level default value is 0 (top level)
    @return          - void.

    Unlike GetMemberAsAptValue, this method calls APT_INC on the return value before returning.

    Call APT_DEC on the output when you are done with it.
    @note  CallStackLevel is just used to get correct "this" object when user double click any item
           in call stack.
*/
/******************************************************************************/
void AptValueHelper::GetMember(AptValue *instance, const char *name, AptValue **output, int callStackLevel)
{
    AptUpdateAutoLock lock;

    AptValue *targetInstance             = instance;
    const size_t MAXIMUM_VAR_NAMR_LENGTH = 256;
    if (targetInstance)
    {
        int isFunctionScope = targetInstance->isScriptFunction() ? 1 : 0;

        size_t targetStringBufferSize = 0;

        size_t firstPosition = 0;
        for (size_t i = 0; i <= strlen(name); ++i)
        {
            if (name[i] == '.' || name[i] == '\0')
            {
                targetStringBufferSize                     = i - firstPosition + 1; // Include '\0' in string buffer
                char targetString[MAXIMUM_VAR_NAMR_LENGTH] = {0};
                strncpy(targetString, &name[firstPosition], targetStringBufferSize - 1);

                AptNativeString strName(targetString);
                targetInstance = gAptActionInterpreter.getVariable(targetInstance, NULL, &strName, 1, isFunctionScope, 1);

                if (targetInstance == gpUndefinedValue)
                {
                    // If targetInstance is invalid
                    if (0 == firstPosition)
                    {
                        targetInstance = GetThisMember(instance, targetString, isFunctionScope, callStackLevel);
                    }
                    if (targetInstance == gpUndefinedValue)
                    {
                        // Judge targetInstance again so as to deal with the result of GetThisMember.
                        break;
                    }
                }

                firstPosition = i + 1;
            }
        }
        *output = targetInstance;
    }

    if (*output)
    {
        APT_INC(*output);
    }
}

/******************************************************************************/
/**
    AptValueHelper::GetThisMember

    @brief  Return variable value specified by name as it's member of "this".

    @param  context              - Current runtime context.
    @param  strName              - Variable name
    @param  isFunctionScope      - If it is function scope, may be "frame-time" codes
    @param  level                - Call stack level

    @return                      - Inquire result
*/
/******************************************************************************/
AptValue *AptValueHelper::GetThisMember(AptValue *context, const char *strName, int isFunctionScope, int level)
{
    AptValue *thisContext = NULL;
    GetThis(context, &thisContext, level);
    AptNativeString nativeString(strName);
    AptValue *output = gAptActionInterpreter.getVariable(thisContext, NULL, &nativeString, 1, isFunctionScope, 1);
    return output;
}

/******************************************************************************/
/**
    WriteDelimitedName

    @brief  Helper function to put a colon on the end of a name safely

    @param  name    - Incoming name
    @param  buf     - Buffer for output
    @param  len     - Where we want to start in buf
    @param  maxLen  - Length of buf
    @return         - How far through buf we ended up, not including the \0 we appended

    I think this may be an overcomplicated, underpowered version of Snprintf?
*/
/******************************************************************************/
static int WriteDelimitedName(const char *name, char *buf, int len, int maxLen)
{
    bool needsAColon = (len > 0);
    int newLen       = static_cast<uint32_t>(len + strlen(name) + (needsAColon ? 1 : 0));
    if (newLen <= maxLen - 1) // We will be appending a '\0' at the end, so we need an extra 1 char:  (newLen + 1) <= maxLen
    {
        if (buf)
        {
            if (needsAColon)
            {
                buf[len] = ':';
                strcpy(&buf[len + 1], name);
            }
            else
            {
                strcpy(&buf[len], name);
            }
            buf[newLen] = '\0';
        }
        len = newLen;
    }
    else
    {
        // Not long enough to fit the full name; terminate anyway
        if (buf && len < maxLen)
        {
            buf[len] = '\0';
        }
    }
    return len;
}

/******************************************************************************/
/**
    GetTextMemberNames

    @brief  Get TextFiled Component's members

    @param  pValue      - Incoming AptValue or AptCIH
    @param  buf           - Buffer to list out all the member names
    @param  maxChars - Size of buf

    @return                   - The number of chars written to buf (excluding trailing null)

    This function allocates no memory, so on input buf must be a buffer of at least
    size maxChars.  On return, buf will contain the members of pValue separated
    by a colon (:).
*/
/******************************************************************************/
int GetTextMemberNames(AptValue *pValue, char *buf, int maxChars)
{
    // These member strings are copied from AptTextMembers.h
    static const char *textMemebrList[] =
        {
            // Original members
            "_height",
            "autoSize",
            "background",
            "backgroundColor",
            "border",
            "borderColor",
            "hscroll",
            "length",
            "maxChars",
            "maxscroll",
            "mouseWheelEnabled",
            "multiline",
            "scroll",
            "text",
            "textColor",
            "textHeight",
            "textWidth",
            "variable"};
    int len                               = 0;
    static const int textMemebrListLength = sizeof(textMemebrList) / sizeof(textMemebrList[0]);
    for (auto &i : textMemebrList)
    {
        len = WriteDelimitedName(i, buf, len, maxChars);
    }

    return len;
}

/******************************************************************************/
/**
    GetSpriteMemberNames

    @brief  Get Sprite(MovieClip) Component's members

    @param  pValue      - Incoming AptValue or AptCIH
    @param  buf           - Buffer to list out all the member names
    @param  maxChars - Size of buf

    @return                   - The number of chars written to buf (excluding trailing null)

    This function allocates no memory, so on input buf must be a buffer of at least
    size maxChars.  On return, buf will contain the members of pValue separated
    by a colon (:).
*/
/******************************************************************************/
int GetSpriteMemberNames(AptValue *pValue, char *buf, int maxChars)
{
    // These member strings are copied from AptSpriteMembers.cpp
    static const char *spriteMemebrList[] =
        {
            // Properties
            "_alpha",
            "_currentframe",
            "_droptarget",
            "_focusrect",
            "_framesloaded",
            "_height",
            "_highquality",
            "_name",
            "_parent",
            "_quality",
            "_renderflags",
            "_rotation",
            "_soundbuftime",
            "_target",
            "_totalframes",
            "_url",
            "_visible",
            "_width",
            "_x",
            "_xmouse",
            "_xrotation",
            "_xscale",
            "_y",
            "_ymouse",
            "_yrotation",
            "_yscale",
            "_z",
            "_zscale"

            // Events
        };
    int len                                 = 0;
    static const int spriteMemebrListLength = sizeof(spriteMemebrList) / sizeof(spriteMemebrList[0]);
    for (auto &i : spriteMemebrList)
    {
        len = WriteDelimitedName(i, buf, len, maxChars);
    }
    return len;
}

/******************************************************************************/
/**
    AptValueHelper::GetExtrinsicMemberNames

    @brief  Gets member names of a value if that value is an AptCIH or AptObject.
            Only returns names of members added by the user, not intrinsic members
            (e.g. _x and _y would be excluded).

    @param  pValue   - Incoming AptValue or AptCIH
    @param  buf      - Buffer to list out all the member names
    @param  maxChars - Size of buf
    @param  isSuper  - If host variable name is "super"
    @return          - The number of chars written to buf (excluding trailing null)

    This function allocates no memory, so on input buf must be a buffer of at least
    size maxChars.  On return, buf will contain the members of pValue separated
    by a colon (:).
*/
/******************************************************************************/
int AptValueHelper::GetExtrinsicMemberNames(AptValue *pValue, char *buf, int maxChars, bool isSuper)
{
    AptNativeHash *pLeafNativeHash = pValue->GetNativeHashVirtual();
    AptNativeHash *protoNativeHash = NULL;
    if (NULL != pLeafNativeHash && pLeafNativeHash->Get__Proto__())
    {
        protoNativeHash = pLeafNativeHash->Get__Proto__()->GetNativeHashVirtual();
    }
    int len = 0;

    if (protoNativeHash)
    {
        AptHashItem *hashItem = protoNativeHash->GetFirstItem();
        while (hashItem)
        {
            if (hashItem->Key.IsValid() && !hashItem->Key.IsEmpty())
            {
                const char *name = hashItem->Key.GetBuffer();
                len              = WriteDelimitedName(name, buf, len, maxChars);
            }
            hashItem = protoNativeHash->GetNextItem(hashItem);
        }
    }

    if (!isSuper && pLeafNativeHash)
    {
        AptHashItem *hashItem = pLeafNativeHash->GetFirstItem();
        while (hashItem)
        {
            if (hashItem->Key.IsValid() && !hashItem->Key.IsEmpty())
            {
                const char *name = hashItem->Key.GetBuffer();
                if ((NULL != protoNativeHash && !protoNativeHash->Lookup(&hashItem->Key)) || protoNativeHash == NULL)
                {
                    len = WriteDelimitedName(name, buf, len, maxChars);
                }
            }
            hashItem = pLeafNativeHash->GetNextItem(hashItem);
        }
    }

    if (pValue->isArray())
    {
        AptArray *pArray = pValue->c_array();
        for (int i = 0; i < pArray->length(); ++i)
        {
            static char name[64];
            sprintf(name, "%d", i);
            len = WriteDelimitedName(name, buf, len, maxChars);
        }
    }
    return len;
}

/******************************************************************************/
/** AptValueHelper :: GetIntrinsicMember

    @brief  Gets an intrinsic member of an AptCIH as an integer.

    @param  pValue     - The AptValue. Will return false if this is not an AptCIH.
    @param  szValue    - The name of the intrinsic member to get.
    @param  pOutInteger- Integer pointer to fill with it.
    @return            - Whether the named intrinsic member exists.
*/
/******************************************************************************/
bool AptValueHelper::GetIntrinsicMember(AptValue *pValue, const char *szValue, int *pOutInteger)
{
    AptUpdateAutoLock lock;

    if (pValue->isCIH())
    {
        AptNativeString keyName(szValue);
        AptCIH *cih     = pValue->c_cih();
        AptValue *value = cih->objectMemberLookup(cih, &keyName);

        if (NULL != value && value->isInteger())
        {
            *pOutInteger = value->toInteger();

            return true;
        }
    }

    return false;
}

/******************************************************************************/
/** AptValueHelper :: GetIntrinsicMember

    @brief  Gets an intrinsic member of an AptCIH as a float.

    @param  pValue     - The AptValue. Will return false if this is not an AptCIH.
    @param  szValue    - The name of the intrinsic member to get.
    @param  pOutFloat  - Float pointer to fill with it.
    @return            - Whether the named intrinsic member exists.
*/
/******************************************************************************/
bool AptValueHelper::GetIntrinsicMember(AptValue *pValue, const char *szValue, float *pOutFloat)
{
    AptUpdateAutoLock lock;

    if (pValue->isCIH())
    {
        AptNativeString keyName(szValue);
        AptCIH *cih     = pValue->c_cih();
        AptValue *value = cih->objectMemberLookup(cih, &keyName);

        if (NULL != value && value->isFloat())
        {
            *pOutFloat = value->toFloat();

            return true;
        }
    }

    return false;
}

/******************************************************************************/
/** AptValueHelper :: GetIntrinsicMember

    @brief  Gets an intrinsic member of an AptCIH as a boolean.

    @param  pValue     - The AptValue. Will return false if this is not an AptCIH.
    @param  szValue    - The name of the intrinsic member to get.
    @param  pOutBool   - Boolean pointer to fill with it.
    @return            - Whether the named intrinsic member exists.
*/
/******************************************************************************/
bool AptValueHelper::GetIntrinsicMember(AptValue *pValue, const char *szValue, bool *pOutBool)
{
    AptUpdateAutoLock lock;

    if (pValue->isCIH())
    {
        AptNativeString keyName(szValue);
        AptCIH *cih     = pValue->c_cih();
        AptValue *value = cih->objectMemberLookup(cih, &keyName);

        if (NULL != value && value->isBoolean())
        {
            *pOutBool = value->toBool();

            return true;
        }
    }

    return false;
}

/******************************************************************************/
/** AptValueHelper :: GetIntrinsicMember

    @brief  Gets an intrinsic member of an AptCIH as a string.

    @param  pValue     - The AptValue. Will return false if this is not an AptCIH.
    @param  szValue    - The name of the intrinsic member to get.
    @param  pOutString - String buffer to fill with it.
    @param  nMaxChars  - Size of the string buffer.
    @return            - Whether the named intrinsic member exists.
*/
/******************************************************************************/
bool AptValueHelper::GetIntrinsicMember(AptValue *pValue, const char *szValue, char *pOutString, int nMaxChars)
{
    AptUpdateAutoLock lock;

    if (pValue->isCIH())
    {
        AptNativeString keyName(szValue);
        AptCIH *cih     = pValue->c_cih();
        AptValue *value = cih->objectMemberLookup(cih, &keyName);

        if (NULL != value)
        {
            if (value->isString())
            {
                // don't create a new AptNativeString if it's already a string
                strncpy((char *)pOutString, value->c_string()->GetInternalString()->c_str(), nMaxChars);
            }
            else
            {
                AptNativeString stringValue;
                value->toString(stringValue);
                strncpy((char *)pOutString, stringValue.c_str(), nMaxChars);
            }

            return true;
        }
    }

    return false;
}

//-------------------------------------------------------------------------------------------

bool AptValueHelper::GetArrayMember(AptValue *pValue, int32_t nIndex, int32_t *pOutInteger)
{
    APT_ASSERT(pOutInteger != NULL);

    AptUpdateAutoLock lock;

    if (pValue->ContainsNativeHashVirtual() == false || pOutInteger == NULL)
    {
        return (false);
    }

    if (!pValue->isArray())
    {
        return false;
    }

    AptValue *pRetValue = pValue->c_array()->GetAt(nIndex);
    if (pRetValue == NULL)
    {
        return (false);
    }
    *pOutInteger = pRetValue->toInteger();

    return true;
}

bool AptValueHelper::GetArrayMember(AptValue *pValue, int32_t nIndex, float_t *pOutFloat)
{
    APT_ASSERT(pOutFloat != NULL);

    AptUpdateAutoLock lock;

    if (pValue->ContainsNativeHashVirtual() == false || pOutFloat == NULL)
    {
        return (false);
    }

    if (!pValue->isArray())
    {
        return false;
    }

    AptValue *pRetValue = pValue->c_array()->GetAt(nIndex);
    if (pRetValue == NULL)
    {
        return (false);
    }
    *pOutFloat = pRetValue->toFloat();

    return true;
}

bool AptValueHelper::GetArrayMember(AptValue *pValue, int32_t nIndex, bool *pOutBool)
{
    APT_ASSERT(pOutBool != NULL);

    AptUpdateAutoLock lock;

    if (pValue->ContainsNativeHashVirtual() == false || pOutBool == NULL)
    {
        return (false);
    }

    if (!pValue->isArray())
    {
        return false;
    }

    AptValue *pRetValue = pValue->c_array()->GetAt(nIndex);
    if (pRetValue == NULL)
    {
        return (false);
    }
    *pOutBool = pRetValue->toBool();

    return true;
}

bool AptValueHelper::GetArrayMember(AptValue *pValue, int32_t nIndex, char *pOutString, uint32_t nMaxChars)
{
    APT_ASSERT(pOutString != NULL);

    AptUpdateAutoLock lock;

    if (pValue->ContainsNativeHashVirtual() == false || pOutString == NULL)
    {
        return (false);
    }

    if (!pValue->isArray())
    {
        return false;
    }

    AptValue *pRetValue = pValue->c_array()->GetAt(nIndex);
    if (pRetValue == NULL)
    {
        return (false);
    }

    if (pRetValue->isString())
    {
        // don't create a new AptNativeString if it's already a string
        strncpy((char *)pOutString, pRetValue->c_string()->GetInternalString()->c_str(), nMaxChars);
    }
    else
    {
        AptNativeString stringValue;
        pRetValue->toString(stringValue);
        strncpy((char *)pOutString, stringValue.c_str(), nMaxChars);
    }

    return true;
}

// following functions are defined because array index can be a string also.

bool AptValueHelper::GetArrayMember(AptValue *pValue, const char *szValue, int32_t *pOutInteger)
{
    APT_ASSERT(pOutInteger != NULL);

    AptUpdateAutoLock lock;

    if (pValue->ContainsNativeHashVirtual() == false || pOutInteger == NULL)
    {
        return (false);
    }

    if (!pValue->isArray())
    {
        return false;
    }

    AptNativeString sKey(szValue);
    AptValue *pRetValue = pValue->c_array()->Lookup(&sKey);
    if (pRetValue == NULL)
    {
        return (false);
    }
    *pOutInteger = pRetValue->toInteger();

    return true;
}

bool AptValueHelper::GetArrayMember(AptValue *pValue, const char *szValue, float_t *pOutFloat)
{
    APT_ASSERT(pOutFloat != NULL);

    AptUpdateAutoLock lock;

    if (pValue->ContainsNativeHashVirtual() == false || pOutFloat == NULL)
    {
        return (false);
    }

    if (!pValue->isArray())
    {
        return false;
    }

    AptNativeString sKey(szValue);
    AptValue *pRetValue = pValue->c_array()->Lookup(&sKey);
    if (pRetValue == NULL)
    {
        return (false);
    }
    *pOutFloat = pRetValue->toFloat();

    return true;
}

bool AptValueHelper::GetArrayMember(AptValue *pValue, const char *szValue, bool *pOutBool)
{
    APT_ASSERT(pOutBool != NULL);

    AptUpdateAutoLock lock;

    if (pValue->ContainsNativeHashVirtual() == false || pOutBool == NULL)
    {
        return (false);
    }

    if (!pValue->isArray())
    {
        return false;
    }

    AptNativeString sKey(szValue);
    AptValue *pRetValue = pValue->c_array()->Lookup(&sKey);
    if (pRetValue == NULL)
    {
        return (false);
    }
    *pOutBool = pRetValue->toBool();

    return true;
}

bool AptValueHelper::GetArrayMember(AptValue *pValue, const char *szValue, char *pOutString, uint32_t nMaxChars)
{
    APT_ASSERT(pOutString != NULL);

    AptUpdateAutoLock lock;

    if (pValue->ContainsNativeHashVirtual() == false || pOutString == NULL)
    {
        return (false);
    }

    if (!pValue->isArray())
    {
        return false;
    }

    AptNativeString sKey(szValue);
    AptValue *pRetValue = pValue->c_array()->Lookup(&sKey);
    if (pRetValue == NULL)
    {
        return (false);
    }

    if (pRetValue->isString())
    {
        // don't create a new AptNativeString if it's already a string
        strncpy((char *)pOutString, pRetValue->c_string()->GetInternalString()->c_str(), nMaxChars);
    }
    else
    {
        AptNativeString stringValue;
        pRetValue->toString(stringValue);
        strncpy((char *)pOutString, stringValue.c_str(), nMaxChars);
    }

    return true;
}

AptValue *AptValueHelper::GetMemberAsAptValue(AptValue *pValue, const char *szValue)
{
    APT_ASSERT(pValue != NULL);

    AptUpdateAutoLock lock;

    if (pValue->ContainsNativeHashVirtual() == false)
    {
        return (NULL);
    }

    AptNativeString sKey(szValue);
    AptValue *pRetValue = pValue->GetNativeHashVirtual()->Lookup(&sKey);

    return pRetValue;
}

AptValue *AptValueHelper::GetMemberAsAptValue(AptValue *pValue, const AptNativeString *pKey)
{
    APT_ASSERT(pValue != NULL);

    AptUpdateAutoLock lock;

    if (pValue->ContainsNativeHashVirtual() == false)
    {
        return (NULL);
    }

    AptValue *pRetValue = pValue->GetNativeHashVirtual()->Lookup(pKey);

    return pRetValue;
}

// these functions are added to AptValueHelper class just because some of the other libraries are
// including _apt.h for creating and accessing AptArray class. But when we are giving out Apt libs to external developers
// they do not get _Apt.h and because of that these other libs could not be built. So exposing required AptArray functions
// through AptValueHelper Class.

AptValue *AptValueHelper::GetFromAptArray(AptValue *pArray, int nIndex)
{
    if (pArray)
    {
        AptUpdateAutoLock lock;
        AptArray *pArr = pArray->c_array();
        return (pArr->get(nIndex));
    }
    return (NULL);
}

void AptValueHelper::SetInAptArray(AptValue *pArray, int nIndex, AptValue *pValue)
{
    if (pArray)
    {
        AptUpdateAutoLock lock;
        AptArray *pArr = pArray->c_array();
        pArr->set(nIndex, pValue);
    }
}

int AptValueHelper::GetArrayLength(AptValue *pArray)
{
    if (pArray)
    {
        AptUpdateAutoLock lock;
        AptArray *pArr = pArray->c_array();
        return (pArr->length());
    }

    return (0);
}

bool AptValueHelper::IsMovieClipVisible(AptValue *pValue)
{
    APT_ASSERT(pValue);
    if (pValue && pValue->isCIH())
    {
        AptCIH *pCih = pValue->c_cih();
        return pCih->IsVisible();
    }
    return false;
}

float AptValueHelper::NumberToFloat(AptValue *value)
{
    if (value)
    {
        if (value->isFloat())
        {
            return value->toFloat();
        }
        if (value->isInteger())
        {
            return (float)value->toInteger();
        }
    }
    APT_FAIL("numberToFloat() : not a number!");
    return 0.0f;
}

int AptValueHelper::NumberToInteger(AptValue *value)
{
    if (value)
    {
        if (value->isFloat())
        {
            return (int)value->toFloat();
        }
        else if (value->isInteger())
        {
            return value->toInteger();
        }
    }
    APT_FAIL("numberToInteger() : not a number!");
    return 0;
}

/******************************************************************************/
/**
    @brief  Return bool converted from an AptValue* known to be a Boolean

    @param  value Value to convert

    @return         - Evaluation of the Apt boolean
*/
/******************************************************************************/
bool AptValueHelper::GetBoolean(AptValue *value)
{
    bool ret = false;
    if (value && value->isBoolean())
    {
        ret = value->toBool();
    }
    else
    {
        APT_FAIL("GetBoolean() : not a boolean!");
    }
    return ret;
}

/******************************************************************************/
/**
    @brief  Return a string from an AptValue* known to be a string

    @param  value   Incoming AptValue
    @param  length  Output length of the string

    @return         - Pointer to the string buffer
*/
/******************************************************************************/
const char *AptValueHelper::GetString(AptValue *value, int *length)
{
    const char *ret = NULL;
    *length         = 0;
    if (value && value->isString())
    {
        AptNativeString str;
        value->toString(str);
        *length = static_cast<int>(str.GetLength());
        ret     = str.c_str(); // This c_str is still pointing to the AptValue's internal data and is not owned by the AptNativeString here; this is safe.
    }
    else
    {
        APT_FAIL("Not passed a string");
    }
    return ret;
}

/******************************************************************************/
/**
    @brief  Return the first HashItem in an Object's hash

    @param  value   Object

    @return         - First item in the hash, or NULL if none
*/
/******************************************************************************/
AptHashItem *AptValueHelper::GetObjectItemFirst(AptValue *value)
{
    AptHashItem *ret = NULL;
    if (value && value->isObject())
    {
        AptNativeHash *nativeHash = value->GetNativeHashVirtual();
        if (nativeHash)
        {
            ret = nativeHash->GetFirstItem();
        }
    }
    else
    {
        APT_FAIL("Not passed an object");
    }
    return ret;
}

/******************************************************************************/
/**
    @brief  Return the next HashItem in an Object's hash

    @param  value   Object
    @param  item    Previous item

    @return         - Next item in the hash, or NULL if none
*/
/******************************************************************************/
AptHashItem *AptValueHelper::GetObjectItemNext(AptValue *value, AptHashItem *item)
{
    AptHashItem *ret = NULL;
    if (item)
    {
        if (value && value->isObject())
        {
            AptNativeHash *nativeHash = value->GetNativeHashVirtual();
            if (nativeHash)
            {
                ret = nativeHash->GetNextItem(item);
            }
            else
            {
                APT_FAIL("Object did not have a hash");
            }
        }
        else
        {
            APT_FAIL("Not passed an object");
        }
    }
    else
    {
        APT_FAIL("Not passed an item");
    }
    return ret;
}

/******************************************************************************/
/** AptValueHelper :: GetGeometryFromMovieClip

    @brief  Get the rendering unit of the first shape found in the given MovieClip.

    @param  pValue  - The MovieClip we want to get the geometry from
    @return         - Returns the rendering unit of the first shape found in the given MovieClip.
                      NULL if no shape or rendering unit found.

*/
/******************************************************************************/
AptAssetRenderingUnit AptValueHelper::GetGeometryFromMovieClip(AptValue *pValue)
{
    AptAssetRenderingUnit ru = NULL;

    if (pValue->isCIH())
    {
        AptCIH *cih                        = pValue->c_cih();
        AptCharacterSpriteInstBase *sprite = cih->GetSpriteInstBase();
        if (sprite)
        {
            AptCIH *clip = sprite->mDisplayList.pState->GetFirstItem();
            if (clip)
            {
                const AptCharacter *character = clip->GetCharacterInst()->GetCharacterConst();
                if (character && (character->eType == AptCharacterType_Shape))
                {
                    ru = character->shape.pRenderUnit->mRenderUnit;
                }
            }
        }
    }
    return ru;
}

/******************************************************************************/
/** AptValueHelper :: GetTextureIdFromMovieClip

    @brief  Get the texture ID of the first shape found in the given MovieClip.

    @param  pValue  - MovieClip we want the texture ID for
    @return         - Returns the texture ID of the first shape found in the given MovieClip.
                      Returns -1 if no shape or texture found.
*/
/******************************************************************************/
#if defined(APT_DECOUPLED_RENDERING)
int32_t AptValueHelper::GetTextureIdFromMovieClip(AptValue *pValue)
{
    int32_t id = -1;
    if (pValue->isCIH())
    {
        AptCIH *cih                        = pValue->c_cih();
        AptCharacterSpriteInstBase *sprite = cih->GetSpriteInstBase();
        if (sprite)
        {
            AptCIH *clip = sprite->mDisplayList.pState->GetFirstItem();
            if (clip)
            {
                const AptCharacter *character = clip->GetCharacterInst()->GetCharacterConst();
                if (character && (character->eType == AptCharacterType_Shape))
                {
                    id = character->m_shapeData.m_textID;
                }
            }
        }
    }
    return id;
}
#endif

/**************************************************************************************************/
/**
    AptValueHelper::SetMember

    @brief
        Function to set a member out of AptValue's hash table as a integer

    @param      instance          - pointer to AptValue
    @param      szValue           - pointer to name of the variable
    @param      newVal            - integer variable will be set

    @return     void
*/
/**************************************************************************************************/
void AptValueHelper::SetMember(AptValue *instance, const char *szValue, int newVal)
{
    AptUpdateAutoLock lock;

    if (NULL != instance)
    {
        AptNativeString strName(szValue);
        AptValue *newValue = AptInteger::Create(newVal);
        gAptActionInterpreter.setVariable(instance, NULL, &strName, newValue, 1, 1, ((strchr(szValue, '.') == NULL) ? 1 : 0));
    }
}

/**************************************************************************************************/
/**
    AptValueHelper::SetMember

    @brief
        Function to set a member out of AptValue's hash table as a float

    @param      instance          - pointer to AptValue
    @param      szValue           - pointer to name of the variable
    @param      newVal            - float variable will be set

    @return     void
*/
/**************************************************************************************************/
void AptValueHelper::SetMember(AptValue *instance, const char *szValue, float newVal)
{
    AptUpdateAutoLock lock;

    if (NULL != instance)
    {
        AptNativeString strName(szValue);
        AptValue *newValue = AptFloat::Create(newVal);
        gAptActionInterpreter.setVariable(instance, NULL, &strName, newValue, 1, 1, ((strchr(szValue, '.') == NULL) ? 1 : 0));
    }
}

/**************************************************************************************************/
/**
    AptValueHelper::SetMember

    @brief
        Function to set a member out of AptValue's hash table as a boolean

    @param      instance          - pointer to AptValue
    @param      szValue           - pointer to name of the variable
    @param      newVal            - boolean variable will be set

    @return     void
*/
/**************************************************************************************************/
void AptValueHelper::SetMember(AptValue *instance, const char *szValue, bool newVal)
{
    AptUpdateAutoLock lock;

    if (NULL != instance)
    {
        AptNativeString strName(szValue);
        AptValue *newValue = AptBoolean::Create(newVal);
        gAptActionInterpreter.setVariable(instance, NULL, &strName, newValue, 1, 1, ((strchr(szValue, '.') == NULL) ? 1 : 0));
    }
}

/**************************************************************************************************/
/**
    AptValueHelper::SetMember

    @brief
        Function to set a member out of AptValue's hash table as a string

    @param      instance          - pointer to AptValue
    @param      szValue           - pointer to name of the variable
    @param      newVal            - string variable will be set

    @return     void
*/
/**************************************************************************************************/
void AptValueHelper::SetMember(AptValue *instance, const char *szValue, const char *newVal)
{
    AptUpdateAutoLock lock;

    if (NULL != instance)
    {
        AptNativeString strName(szValue);
        AptValue *newValue = AptString::Create(newVal);
        gAptActionInterpreter.setVariable(instance, NULL, &strName, newValue, 1, 1, ((strchr(szValue, '.') == NULL) ? 1 : 0));
    }
}

/**************************************************************************************************/
/**
    AptValueHelper::SetMember

    @brief
        Function to set a member out of AptValue's hash table as a string

    @param      instance          - pointer to AptValue
    @param      szValue           - pointer to name of the variable
    @param      newVal            - string variable will be set

    @return     void
*/
/**************************************************************************************************/
void AptValueHelper::SetMember(AptValue *instance, const char *szValue, AptValue *newVal)
{
    AptUpdateAutoLock lock;

    if (NULL != instance)
    {
        AptNativeString strName(szValue);
        gAptActionInterpreter.setVariable(instance, NULL, &strName, newVal, 1, 1, ((strchr(szValue, '.') == NULL) ? 1 : 0));
    }
}

/******************************************************************************/
/**
    AptValueHelper::GetLocalNameList
    @brief  Get local variables' name by function runtime context.

    @param  currentContext  The corresponding runtime context got from push operation.
    @param  buf             Used to keep result string buffer.
    @param  maxChars        Max size of result string buffer.

    @return Result string length.
*/
/******************************************************************************/
int AptValueHelper::GetLocalNameList(AptValue *currentContext, char *buf, int maxChars)
{
    AptNativeHash *nativeHash = NULL;
    if (currentContext->isScriptFunction())
    {
        AptScriptFunctionBase *currentFunction = static_cast<AptScriptFunctionBase *>(currentContext);
        APT_ASSERT(NULL != currentFunction);
        if (currentFunction->GetFrameStack())
        {
            nativeHash = currentFunction->GetFrameStack()->GetNativeHashVirtual();
        }
    }
    else
    {
        nativeHash = currentContext->GetNativeHashVirtual();
    }

    int length = 0;
    length     = WriteDelimitedName("this", buf, length, maxChars); // We should always display "this" in the local view.
    if (nativeHash == NULL)
    {
        return length;
    }

    AptHashItem *firstItem = nativeHash->GetFirstItem();
    AptHashItem *nextItem  = firstItem;

    while (NULL != nextItem)
    {
        const char *name = nextItem->Key.c_str();
        length           = WriteDelimitedName(name, buf, length, maxChars);
        nextItem         = nativeHash->GetNextItem(nextItem);
    }
    return length;
}

/******************************************************************************/
