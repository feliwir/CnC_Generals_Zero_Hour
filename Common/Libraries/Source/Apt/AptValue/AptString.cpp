#include "_Apt.h"
#include "AptStringMembers.h"
#include "MainInline.h"
#include "AptValue/AptValueVector.h"
#include "AptGlobal.h"

#if !defined(APT_ENABLE_INLINE)
#include "AptValue/AptString.inl"
#endif

static const int AptString_length = 1;
#if APT_USE_STRING_METHODS
static const int AptString_charAt       = 2;
static const int AptString_charCodeAt   = 3;
static const int AptString_concat       = 4;
static const int AptString_fromCharCode = 5;
static const int AptString_indexOf      = 6;
static const int AptString_lastIndexOf  = 7;
static const int AptString_slice        = 8;
static const int AptString_split        = 9;
static const int AptString_substr       = 10;
static const int AptString_substring    = 11;
static const int AptString_toLowerCase  = 12;
static const int AptString_toUpperCase  = 13;
#endif

/** Class-specific operator new; allocates via the non-garbage-collecting pool. */
void *AptString::operator new(size_t size)
{
    return APT_NONGC_NEW(size);
}

/** Class-specific operator delete. */
void AptString::operator delete(void *p, size_t size)
{
    APT_NONGC_DELETE(p, size);
}

/** Class-specific operator new[]. */
void *AptString::operator new[](size_t size)
{
    return AptNonGCAllocSaveSize(APT_ALLOC_PARAMS(size));
}

/** Class-specific operator delete[]. */
void AptString::operator delete[](void *p)
{
    AptValueNoGC::VerifyAptValueNoGC();
    AptNonGCFreeSavedSize(p);
}

/** Destroys the native function objects backing the String prototype's member functions. */
void AptString::CleanNativeFunctions()
{
#if APT_USE_STRING_METHODS
    NATIVE_MEMBER_FUNCTION_DESTROY(charAt);
    NATIVE_MEMBER_FUNCTION_DESTROY(charCodeAt);
    NATIVE_MEMBER_FUNCTION_DESTROY(concat);
    NATIVE_MEMBER_FUNCTION_DESTROY(fromCharCode);
    NATIVE_MEMBER_FUNCTION_DESTROY(indexOf);
    NATIVE_MEMBER_FUNCTION_DESTROY(lastIndexOf);
    NATIVE_MEMBER_FUNCTION_DESTROY(slice);
    NATIVE_MEMBER_FUNCTION_DESTROY(split);
    NATIVE_MEMBER_FUNCTION_DESTROY(substr);
    NATIVE_MEMBER_FUNCTION_DESTROY(substring);
    NATIVE_MEMBER_FUNCTION_DESTROY(toLowerCase);
    NATIVE_MEMBER_FUNCTION_DESTROY(toUpperCase);
#endif
}

#if defined(APT_USE_STRING_METHODS)
NATIVE_MEMBER_FUNCTION(AptString, charAt)
{
    AptValue *pIndex              = gAptActionInterpreter.stackAt(0);
    int nIndex                    = pIndex->toInteger();
    AptNativeString *pLocalString = pThis->c_string()->GetInternalString();

    if (nIndex < 0)
    {
        return gpUndefinedValue;
    }
    else
    {
        const char *pBuffer = pLocalString->UTF8_GetBuffer(nIndex);
        if (pBuffer == NULL)
        {
            return (gpUndefinedValue);
        }
        AptNativeString localString;
        localString.UTF8_Append(pBuffer, 1); //  Take only one UTF8 character
        AptString *pReturnedString = AptString::Create();
        pReturnedString->cpy(&localString);
        return (pReturnedString);
    }
}

NATIVE_MEMBER_FUNCTION(AptString, charCodeAt)
{
    AptValue *pIndex              = gAptActionInterpreter.stackAt(0);
    int nIndex                    = pIndex->toInteger();
    AptNativeString *pLocalString = pThis->c_string()->GetInternalString();

    if (nIndex < 0)
    {
        return gpUndefinedValue;
    }
    else
    {
        const char *pBuffer = pLocalString->UTF8_GetBuffer(nIndex);
        if (pBuffer == NULL)
        {
            return (gpUndefinedValue);
        }
        int32_t unicode = AptNativeString::UTF8_GetCharacter(pBuffer);
        return AptInteger::Create(unicode);
    }
}

NATIVE_MEMBER_FUNCTION(AptString, concat)
{
    AptNativeString sResult;
    pThis->toString(sResult);

    for (int i = 0; i < nParams; i++)
    {
        AptNativeString sAdd;
        AptValue *pVal = gAptActionInterpreter.stackAt(i);
        pVal->toString(sAdd);
        sResult += sAdd;
    }

    AptString *pStr = AptString::Create();
    pStr->str       = sResult;
    return pStr;
}

NATIVE_MEMBER_FUNCTION(AptString, fromCharCode)
{
    AptNativeString localString(2 * nParams); //  If we use this function, we expect several 2 bytes code
    AptValue *pAptCode;
    int32_t i;
    int32_t unicode;

    for (i = 0; i < nParams; ++i)
    {
        pAptCode = gAptActionInterpreter.stackAt(i);
        unicode  = pAptCode->toInteger();
        AptNativeString strOneCode;
        strOneCode.UTF8_Initialize(unicode);
        localString += strOneCode;
    }

    AptString *pReturnedString = AptString::Create();
    pReturnedString->cpy(&localString);
    return (pReturnedString);
}

NATIVE_MEMBER_FUNCTION(AptString, indexOf)
{
    AptNativeString sString;
    AptNativeString sLookFor;
    int nStartOffsset = 0;
    pThis->toString(sString);

    if (nParams == 0)
        return gpUndefinedValue;

    AptValue *pLookFor = gAptActionInterpreter.stackAt(0);
    pLookFor->toString(sLookFor);

    // Added support for startOffset parameter.
    if (nParams == 2)
    {
        AptValue *pStartOffset = gAptActionInterpreter.stackAt(1);
        APT_ASSERT(pStartOffset->isInteger() || pStartOffset->isFloat());
        nStartOffsset = pStartOffset->toInteger();
        if (nStartOffsset < 0)
            nStartOffsset = 0; // This is what the flash player does.
    }
    return (AptInteger::Create(sString.UTF8_Find(sLookFor, nStartOffsset)));
}

NATIVE_MEMBER_FUNCTION(AptString, lastIndexOf)
{
    AptNativeString sString;
    AptNativeString sLookFor;
    int iLastIdx = 0;
    pThis->toString(sString);

    if (nParams == 0)
    {
        return gpUndefinedValue;
    }

    AptValue *pLookFor = gAptActionInterpreter.stackAt(0);
    pLookFor->toString(sLookFor);

    if (nParams > 1)
    {
        AptValue *pStart = gAptActionInterpreter.stackAt(1);
        if (!pStart->isUndefined())
        {
            iLastIdx = pStart->toInteger(); // 2nd parameter: last position to be search
        }
        else
        {
            // The second param was undefined, so let's search the whole string
            iLastIdx = sString.GetLength();
        }
    }
    else
    {
        iLastIdx = sString.GetLength(); // Default 2nd parameter : search all the string
    }

    return (AptInteger::Create(sString.LastIndexOf(sLookFor, iLastIdx)));
}

NATIVE_MEMBER_FUNCTION(AptString, slice)
{
    AptNativeString sString;
    int nStart = -1;
    int nEnd   = 9999999;

    if (nParams == 0)
        return gpUndefinedValue;

    if (nParams >= 1)
    {
        AptValue *pStart = gAptActionInterpreter.stackAt(0);
        nStart           = pStart->toInteger();
    }
    if (nParams >= 2)
    {
        AptValue *pEnd = gAptActionInterpreter.stackAt(1);
        nEnd           = pEnd->toInteger();
    }

    pThis->toString(sString);
    int nStrLen = sString.UTF8_Size();
    if (nStart < 0)
        nStart = nStrLen + nStart; // negative value if based off of end.
    if (nEnd < 0)
        nEnd = nStrLen + nEnd; // negative value if based off of end.
    if (nStart < 0)
        nStart = 0; // if negative value > strlen, ratchet to 0 (what flash does).
    if (nEnd < 0)
        nEnd = 0; // if negative value > strlen, ratchet to 0 (what flash does).
    if (nStart >= nStrLen)
        nStart = nStrLen;
    if (nEnd >= nStrLen)
        nEnd = nStrLen;

    AptNativeString newString;
    newString          = sString.UTF8_Mid(nStart, nEnd - nStart);
    AptString *pSubStr = AptString::Create();
    pSubStr->str       = newString;
    return (pSubStr);
}

NATIVE_MEMBER_FUNCTION(AptString, split)
{
    AptArray *pArray = new AptArray();
    if (nParams == 0)
    {
        pArray->set(0, pThis);
    }
    else if (nParams >= 1)
    {
        AptNativeString sSeps;
        AptValue *pStr = gAptActionInterpreter.stackAt(0);
        int32_t nLimit = 999999;
        pStr->toString(sSeps);

        if (nParams >= 2)
        {
            nLimit = gAptActionInterpreter.stackAt(1)->toInteger();
        }

        AptNativeString localString(*pThis->c_string()->GetInternalString());

        //  Get the size in ASCII mode, don't use the UTF8 as everything will be in ASCII
        int32_t nSepSize = sSeps.Size();

        if (nSepSize == 0) // no seps means one char per array element
        {
            int32_t iChar, i;
            const char *pBuffer = localString.c_str();
            for (i = 0; i < nLimit; ++i)
            {
                pBuffer = AptNativeString::UTF8_ReadCharacter(pBuffer, &iChar);
                if (iChar == 0)
                {
                    break;
                }
                AptNativeString subString;
                subString.UTF8_Initialize(iChar);
                AptString *pTemp = AptString::Create();
                pTemp->cpy(subString);
                pArray->set(i, pTemp);
            }
        }
        else
        {
            int32_t index;
            int32_t lastIndex        = 0;
            int32_t i                = 0;
            const char *pLocalBuffer = localString.c_str();
            for (; i < nLimit;)
            {
                //  The find will work in ASCII and UTF8, so don't bother with slow UTF8 search
                index = localString.Find(sSeps, lastIndex);
                if (index == -1)
                {
                    AptString *pTemp = AptString::Create();
                    AptNativeString subString;
                    if (lastIndex != index)
                    {
                        //  Append the string only if not empty
                        //  Use append with size because we don't have \0 at the end
                        //  In this case, it should work with ASCII and UTF8, as the find returns the ASCII index
                        subString.Append(localString.c_str() + lastIndex, index - lastIndex);
                    }
                    pTemp->cpy(&subString);
                    pArray->set(i, pTemp);
                    break;
                }
                AptString *pTemp = AptString::Create();
                AptNativeString subString;
                //  Use append with size because we don't have \0 at the end
                //  In this case, it should work with ASCII and UTF8, as the find returns the ASCII index
                subString.Append(pLocalBuffer + lastIndex, index - lastIndex);
                pTemp->cpy(&subString);
                pArray->set(i, pTemp);
                ++i;
                lastIndex = index + nSepSize;
            }
        }
    }

    return pArray;
}

NATIVE_MEMBER_FUNCTION(AptString, substr)
{
    AptNativeString sString;
    int nStart  = -1;
    int nLength = 9999999;

    if (nParams == 0)
        return gpUndefinedValue;

    if (nParams >= 1)
    {
        AptValue *pStart = gAptActionInterpreter.stackAt(0);
        nStart           = pStart->toInteger();
    }
    if (nParams >= 2)
    {
        AptValue *pLength = gAptActionInterpreter.stackAt(1);
        nLength           = pLength->toInteger();
    }

    pThis->toString(sString);
    int nStrLen = sString.UTF8_Size();
    if (nStart < 0)
    {
        nStart = nStrLen + nStart;
    }

    AptString *pSubStr = AptString::Create();
    pSubStr->str       = sString.UTF8_Mid(nStart, nLength);
    return pSubStr;
}

NATIVE_MEMBER_FUNCTION(AptString, substring)
{
    AptNativeString sString;
    int nStart = -1;
    int nEnd   = 9999999;
    if (nParams == 0)
    {
        return gpUndefinedValue;
    }
    if (nParams >= 1)
    {
        AptValue *pStart = gAptActionInterpreter.stackAt(0);
        nStart           = pStart->toInteger();
    }
    if (nParams >= 2)
    {
        AptValue *pEnd = gAptActionInterpreter.stackAt(1);
        nEnd           = pEnd->toInteger();
    }

    if (nStart > nEnd)
    {
        int nTemp = nEnd;
        nEnd      = nStart;
        nStart    = nTemp;
    }

    if (nStart < 0)
    {
        nStart = 0;
    }
    if (nEnd < 0)
    {
        nEnd = 0;
    }

    if (nStart > nEnd)
    {
        int nTemp = nEnd;
        nEnd      = nStart;
        nStart    = nTemp;
    }

    pThis->toString(sString);
    AptString *pSubStr = AptString::Create();
    pSubStr->str       = sString.UTF8_Mid(nStart, nEnd - nStart);
    return pSubStr;
}

NATIVE_MEMBER_FUNCTION(AptString, toLowerCase)
{
    AptNativeString sNew;
    pThis->toString(sNew);
    // * 4 because case mapping is not a 1:1 operation in utf8.
    unsigned int pUtf8OutSize = sNew.GetLength() * 4;
    if (pUtf8OutSize > 0)
    {
        if (AptGetUserFuncs().pfnLowerCaseString)
        {
            char *pUtf8Out = static_cast<char *>(APT_MALLOC_BLOCK(pUtf8OutSize));
            APT_ASSERT(pUtf8Out != NULL);
            AptGetUserFuncs().pfnLowerCaseString(sNew.UTF8_GetBuffer(0), pUtf8Out, pUtf8OutSize);
            sNew = pUtf8Out;
            APT_FREE_BLOCK(pUtf8Out, pUtf8OutSize);
        }
        else
        {
            sNew.UTF8_MakeLower();
        }
    }
    AptString *pStr = AptString::Create();
    pStr->str       = sNew;
    return pStr;
}

NATIVE_MEMBER_FUNCTION(AptString, toUpperCase)
{
    AptNativeString sNew;
    pThis->toString(sNew);
    // * 4 because case mapping is not a 1:1 operation in utf8.
    unsigned int pUtf8OutSize = sNew.GetLength() * 4;
    if (pUtf8OutSize > 0)
    {
        if (AptGetUserFuncs().pfnUpperCaseString)
        {
            char *pUtf8Out = static_cast<char *>(APT_MALLOC_BLOCK(pUtf8OutSize));
            APT_ASSERT(pUtf8Out != NULL);
            AptGetUserFuncs().pfnUpperCaseString(sNew.UTF8_GetBuffer(0), pUtf8Out, pUtf8OutSize);
            sNew = pUtf8Out;
            APT_FREE_BLOCK(pUtf8Out, pUtf8OutSize);
        }
        else
        {
            sNew.UTF8_MakeUpper();
        }
    }
    AptString *pStr = AptString::Create();
    pStr->str       = sNew;
    return pStr;
}
#endif

/**
 * Looks up a String prototype member ("length" or, if enabled, one of the String native
 * methods) by name.
 * @param pContext the string value the member is being looked up on
 * @param pName member name to resolve
 * @return the resolved value, or NULL if pName isn't a known String member
 */
AptValue *AptString::objectMemberLookup(AptValue *const pContext, const AptNativeString *const pName) const
{
    StringMembers *pProp = pContext ? StringMembersIndex::in_word_set(pName->c_str(), pName->Size()) : NULL;
    if (pProp)
    {
        switch (pProp->nIndex)
        {
        case (AptString_length):
        {
            AptNativeString sBuf;
            APT_ASSERT(pContext);
            pContext->toString(sBuf);
            return AptInteger::Create(sBuf.UTF8_Size());
        }
#if APT_USE_STRING_METHODS
        break;
        case AptString_charAt:

        {
            NATIVE_MEMBER_FUNCTION_DISPATCH(charAt);
        }
        break;
        case AptString_charCodeAt:

        {
            NATIVE_MEMBER_FUNCTION_DISPATCH(charCodeAt);
        }
        break;
        case AptString_concat:

        {
            NATIVE_MEMBER_FUNCTION_DISPATCH(concat);
        }
        break;
        case AptString_fromCharCode:

        {
            NATIVE_MEMBER_FUNCTION_DISPATCH(fromCharCode);
        }
        break;
        case AptString_indexOf:

        {
            NATIVE_MEMBER_FUNCTION_DISPATCH(indexOf);
        }
        break;
        case AptString_lastIndexOf:

        {
            NATIVE_MEMBER_FUNCTION_DISPATCH(lastIndexOf);
        }
        break;
        case AptString_slice:

        {
            NATIVE_MEMBER_FUNCTION_DISPATCH(slice);
        }
        break;
        case AptString_split:

        {
            NATIVE_MEMBER_FUNCTION_DISPATCH(split);
        }
        break;
        case AptString_substr:

        {
            NATIVE_MEMBER_FUNCTION_DISPATCH(substr);
        }
        break;
        case AptString_substring:

        {
            NATIVE_MEMBER_FUNCTION_DISPATCH(substring);
        }
        break;
        case AptString_toLowerCase:

        {
            NATIVE_MEMBER_FUNCTION_DISPATCH(toLowerCase);
        }
        break;
        case AptString_toUpperCase:

        {
            NATIVE_MEMBER_FUNCTION_DISPATCH(toUpperCase);
        }
#endif
        }
    }

#if defined(EA_DEBUG)
    if (
        (pName->EqualNoCase("length"))
#if APT_USE_STRING_METHODS
        || (pName->EqualNoCase("charAt")) ||
        (pName->EqualNoCase("charCodeAt")) ||
        (pName->EqualNoCase("concat")) ||
        (pName->EqualNoCase("fromCharCode")) ||
        (pName->EqualNoCase("indexOf")) ||
        (pName->EqualNoCase("lastIndexOf")) ||
        (pName->EqualNoCase("slice")) ||
        (pName->EqualNoCase("split")) ||
        (pName->EqualNoCase("substr")) ||
        (pName->EqualNoCase("substring")) ||
        (pName->EqualNoCase("toLowerCase")) ||
        (pName->EqualNoCase("toUpperCase"))
#endif
    )
    {
        APT_DEBUGPRINT(APT_DEBUG_MSG_WARNING_LVL, "AptString: Incorrect case for '%s'.\n", pName->c_str());
        APT_ASSERT(0);
    }
#endif

    // Return NULL instead of gpUndefinedValue: this function is supposed to return NULL if the
    // member isn't found (returning undefined here would cause ambiguity).
    return NULL;
}

/** Formats a printf-style string directly into this AptString's contents. */
void AptString::printf(const char *szFormat, ...)
{
    static char szBuf[2048];
    va_list arg;

    va_start(arg, szFormat);
    vsnprintf(szBuf, 2048, szFormat, arg);
    szBuf[2047] = '\0';
    va_end(arg);

    str = szBuf;
}


/**
 * Creates a new AptString, reusing a pooled instance if the string pool has a free entry.
 * @param szValue initial contents of the string
 * @return the new (or recycled) string
 */
AptString *AptString::Create(const char *szValue)
{
    AptUpdateAutoLock lock;

#if APT_POOL_STRINGS
    if (StringPool::spFirstFree != NULL)
    {
        AptString *pNewString = StringPool::spFirstFree; // Re-read inside the lock.
        if (pNewString)
        {
            //  The type should be string
            APT_ASSERT(pNewString->getVtblIndex() == AptVFT_StringValue);
            //  The refCount should be equal to zero
            APT_ASSERT(pNewString->getRefCount() == 0);
            // May not have been deleted from the deferred vector. Set Release At End flag.
            pNewString->SetReleaseAtEnd();

            //  Push the value on the release vector
            //  So it will be recycled if possible
            AptGetLib()->mpValuesToRelease->PushValue(pNewString);

            if (pNewString->str.IsEmpty() == false)
            {
                //  We call ReserveSize(0), the string is cleared but the buffer is still allocated
                //  Therefore we don't reallocate the buffer next time
                //  If the string is empty, there is no need to do that
                pNewString->str.ReserveSize(0);
            }

            StringPool::spFirstFree = pNewString->mpNext;

            if (szValue[0] != '\0')
                pNewString->cpy(szValue);

            return (pNewString);
        }
    }
#endif
    return (new AptString(szValue));
}

/** Either returns this instance to the string pool or deletes it, depending on how we are configured. */
void AptString::Destroy()
{
    AptUpdateAutoLock lock;

#if APT_POOL_STRINGS
    {
        //  Add to the free pool
        mpNext                  = StringPool::spFirstFree;
        StringPool::spFirstFree = this;

        if (str.IsEnoughSize(MAX_SIZE_KEPT_TEMP_STRING + 1))
        {
            //  The capacity is bigger than MAX_SIZE_KEPT_TEMP_STRING
            //  We don't want each temporary string to grow up to the max size of the manipulated strings
            //  It could be very big and after a while all the temp strings could be 10 KB long!
            //  So we clear the string, it releases the buffer
            str.Clear();
        }
    }
#else
    {
        delete this;
    }
#endif
}
