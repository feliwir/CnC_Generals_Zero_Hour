#include "_Apt.h"
#include "AptArrayMembers.h"
#include "AptValueHelper.h"

#include "MainInline.h"

static const int AptArray_length = 1;
#if APT_USE_ARRAY_METHODS
static const int AptArray_concat   = 2;
static const int AptArray_join     = 3;
static const int AptArray_pop      = 4;
static const int AptArray_push     = 5;
static const int AptArray_shift    = 6;
static const int AptArray_unshift  = 7;
static const int AptArray_reverse  = 8;
static const int AptArray_sort     = 9;
static const int AptArray_splice   = 10;
static const int AptArray_slice    = 11;
static const int AptArray_sortOn   = 12;
static const int AptArray_toString = 13;
#endif // APT_USE_ARRAY_METHODS

#define APTARRAY_START_SIZE 8


/** Class-specific operator new; allocates via the garbage-collecting Dogma allocator. */
void *AptArray::operator new(size_t size)
{
    return APT_GC_NEW(size);
}

/** Placement new operator. */
void *AptArray::operator new(size_t t, void *p)
{
    return p;
}

/** Class-specific operator delete. */
void AptArray::operator delete(void *p, size_t size)
{
    AptValueGC::VerifyAptValueGC();
    APT_GC_DELETE(p, size);
}

AptArray::AptArray(int nElements, AptValue **pAptValue)
    : AptObject(AptVFT_Array, nElements)
{
    mnCapacity = 0;
    mpValues   = NULL;
    mnLength   = nElements;

    _reserve(nElements);

    for (int i = 0; i < mnLength; i++)
    {
        SetAt(i, pAptValue[i]);
    }
}

AptArray::AptArray()
    : AptObject(AptVFT_Array)
{
    mnCapacity = 0;
    mpValues   = NULL;
    mnLength   = 0;
}

// Warning: NEVER decrement GC'd objects in the destructor, that is for the DestroyGCPointers call.
AptArray::~AptArray()
{
    APT_ASSERT(mpValues == NULL && "DestroyGCPointers Was not called on this object before deletion!");
    APT_ASSERT(mnLength == 0 && "DestroyGCPointers Was not called on this object before deletion!");
    APT_ASSERT(mnCapacity == 0 && "DestroyGCPointers Was not called on this object before deletion!");
}

/** Releases the native member functions. Must be called at least during AptShutdown. */
void AptArray::CleanNativeFunctions()
{
#if APT_USE_ARRAY_METHODS
    NATIVE_MEMBER_FUNCTION_DESTROY(concat);
    NATIVE_MEMBER_FUNCTION_DESTROY(join);
    NATIVE_MEMBER_FUNCTION_DESTROY(pop);
    NATIVE_MEMBER_FUNCTION_DESTROY(push);
    NATIVE_MEMBER_FUNCTION_DESTROY(shift);
    NATIVE_MEMBER_FUNCTION_DESTROY(unshift);
    NATIVE_MEMBER_FUNCTION_DESTROY(reverse);
    NATIVE_MEMBER_FUNCTION_DESTROY(sort);
    NATIVE_MEMBER_FUNCTION_DESTROY(splice);
    NATIVE_MEMBER_FUNCTION_DESTROY(slice);
    NATIVE_MEMBER_FUNCTION_DESTROY(sortOn);
#endif
}

/**
 * Releases all the contained GC pointers. Called just before destruction, separate from the
 * destructor because the garbage collector owns the lifetime of the referenced values.
 */
void AptArray::DestroyGCPointers()
{
    AptObject::DestroyGCPointers();
    APT_ASSERT(mpValues != NULL || mnLength == 0);
    APT_ASSERT(mnLength <= mnCapacity);

    for (int i = 0; i < mnLength; i++)
    {
        APT_ASSERT(mpValues != NULL);
        AptValue *pValue = mpValues[i];
        if (pValue)
        {
            APT_DEC(pValue);
            mpValues[i] = NULL;
        }
    }

    if (mpValues)
        APT_FREE_ARRAY(mpValues, AptValue *, mnCapacity);

    mpValues   = NULL;
    mnLength   = 0;
    mnCapacity = 0;
    return;
}

/**
 * @param nIndex Index of the element to return.
 * @return The value at nIndex, or gpUndefinedValue if nIndex is out of range.
 */
AptValue *AptArray::GetAt(const int32_t nIndex) const
{
    APT_ASSERT(nIndex < mnLength);
    if (nIndex >= mnLength)
        return gpUndefinedValue;

    APT_ASSERT(mpValues != NULL);
    return mpValues[nIndex];
}

/**
 * Sets the value at a specified index, adjusting the refcounts of the old and new values.
 * @param nIndex Index to set.
 * @param pNewValue Value to store there.
 */
void AptArray::SetAt(const int32_t nIndex, AptValue *const pNewValue)
{
    APT_ASSERT(pNewValue != NULL);
    APT_ASSERT(nIndex < mnLength);
    APT_ASSERT(mpValues != NULL);

    AptValue *pOldValue = mpValues[nIndex];

    APT_INC(pNewValue);
    APT_DECSAFE(pOldValue);

    mpValues[nIndex] = pNewValue;
}

/** Calls APT_REGISTER_REFERENCE_SAFE on all of the array's element references. */
void AptArray::RegisterReferences()
{
    if (APT_REFERENCES_REGISTERED(this))
        return;

    AptObject::RegisterReferences();

    // Do our own thing.
    for (int i = 0; i < mnLength; i++)
    {
        APT_ASSERT(mpValues);
        APT_REGISTER_REFERENCE_SAFE(mpValues[i], "Element", APT_REFREG_IS_APTVALUE);
    }

    return;
}

/**
 * Grows the backing storage (in power-of-two steps) so that it can hold at least nSize elements.
 * @param nSize Anticipated number of elements.
 */
void AptArray::_reserve(int nSize)
{
    int nCapSize = 0;

    if (mnCapacity >= nSize)
        return;

    nSize--;
    while (nSize)
    {
        nCapSize++;
        nSize >>= 1;
    }
    nCapSize = 1 << (nCapSize);
    nCapSize = nCapSize < APTARRAY_START_SIZE ? APTARRAY_START_SIZE : nCapSize;

    AptValue **_aArray = APT_MALLOC_ARRAY(AptValue *, nCapSize);

    if (mpValues && _aArray != NULL)
    {
        memcpy(_aArray, mpValues, sizeof(AptValue *) * mnCapacity);
        APT_FREE_ARRAY(mpValues, AptValue *, mnCapacity);
    }

    // Initialize the newly grown (non-memcpy'd) portion of the array with gpUndefinedValue so that
    // slicing arrays that have been resized doesn't hit uninitialized memory.
    for (int32_t i = mnCapacity; i < nCapSize && _aArray != NULL; i++)
    {
        _aArray[i] = gpUndefinedValue;
    }

    mpValues   = _aArray;
    mnCapacity = nCapSize;
}

/**
 * Sets the item at the given index, growing the array and updating its length as needed.
 * @param nIndex Index to set.
 * @param pValue Value to put there.
 */
void AptArray::set(int nIndex, AptValue *pValue)
{
    if (nIndex < 0)
    {
        return;
    }

    _reserve(nIndex + 1);
    APT_ASSERT(nIndex < mnCapacity);

    mnLength = (nIndex + 1) > mnLength ? (nIndex + 1) : mnLength;

    SetAt(nIndex, pValue);
}

/**
 * @param nIndex Index of the item to return.
 * @return The item at nIndex, or gpUndefinedValue if nIndex is out of range or empty.
 */
AptValue *AptArray::get(int nIndex)
{
    if (nIndex < 0 || nIndex >= mnLength)
    {
        return gpUndefinedValue;
    }

    AptValue *pValue = GetAt(nIndex);
    if (pValue == NULL)
    {
        pValue = gpUndefinedValue;
    }
    return (pValue);
}

/**
 * Concatenates the array contents into a single string using the given separator.
 * @param szBuf Buffer to place the result into. WARNING: unbounded; prefer the AptNativeString overload.
 * @param szSeparator Separator string.
 */
void AptArray::toString(char *szBuf, const char *szSeparator)
{
    APT_ASSERT(szSeparator);
    AptNativeString sBuf;
    toString(sBuf, szSeparator);
    strcpy(szBuf, sBuf.ConstRawPtr());
}

/**
 * Concatenates the array contents into a single string using the given separator.
 * @param sBuf Buffer to place the result into.
 * @param szSeparator Separator string.
 */
void AptArray::toString(AptNativeString &sBuf, const char *szSeparator)
{
    APT_ASSERT(szSeparator);
    sBuf = "";

    for (int i = 0; i < mnLength; i++)
    {
        AptValue *pValue = GetAt(i);
        if (pValue != NULL)
        {
            TO_STRING(pValue, psVal);
            sBuf += *psVal;
        }
        if (i < mnLength - 1)
        {
            sBuf += szSeparator;
        }
    }
}

/**
 * Looks up a member on the array, resolving intrinsic properties/methods (length, concat, ...)
 * before falling back to numeric-index element access and generic property lookup.
 * @param pContext The context the lookup is performed against.
 * @param pName The member name being looked up.
 * @return The AptValue for the member, or NULL if it is not a known member.
 */
AptValue *AptArray::objectMemberLookup(AptValue *const pContext, const AptNativeString *const pName) const
{
    ArrayMembers *pProp = pContext ? ArrayMembersIndex::in_word_set(pName->c_str(), pName->Size()) : NULL;
    if (pProp)
    {
        switch (pProp->nIndex)
        {
        case (AptArray_length):
        {
            return AptInteger::Create(mnLength);
        }
#if APT_USE_ARRAY_METHODS
        break;
        case AptArray_concat:

        {
            NATIVE_MEMBER_FUNCTION_DISPATCH(concat);
        }
        break;
        case AptArray_join:

        {
            NATIVE_MEMBER_FUNCTION_DISPATCH(join);
        }
        break;
        case AptArray_toString:

        {
            // toString() functions the same as join(), so it doesn't need its own function.
            NATIVE_MEMBER_FUNCTION_DISPATCH(join);
        }
        break;
        case AptArray_pop:

        {
            NATIVE_MEMBER_FUNCTION_DISPATCH(pop);
        }
        break;
        case AptArray_push:

        {
            NATIVE_MEMBER_FUNCTION_DISPATCH(push);
        }
        break;
        case AptArray_shift:

        {
            NATIVE_MEMBER_FUNCTION_DISPATCH(shift);
        }
        break;
        case AptArray_unshift:

        {
            NATIVE_MEMBER_FUNCTION_DISPATCH(unshift);
        }
        break;
        case AptArray_reverse:

        {
            NATIVE_MEMBER_FUNCTION_DISPATCH(reverse);
        }
        break;
        case AptArray_sort:

        {
            NATIVE_MEMBER_FUNCTION_DISPATCH(sort);
        }
        break;
        case AptArray_splice:

        {
            NATIVE_MEMBER_FUNCTION_DISPATCH(splice);
        }
        break;
        case AptArray_slice:

        {
            NATIVE_MEMBER_FUNCTION_DISPATCH(slice);
        }
        break;
        case AptArray_sortOn:

        {
            NATIVE_MEMBER_FUNCTION_DISPATCH(sortOn);
        }
#endif
        }
    }

#if defined(EA_DEBUG)
    if (
        (pName->EqualNoCase("length"))
#if APT_USE_ARRAY_METHODS
        || (pName->EqualNoCase("concat")) ||
        (pName->EqualNoCase("join")) ||
        (pName->EqualNoCase("pop")) ||
        (pName->EqualNoCase("push")) ||
        (pName->EqualNoCase("shift")) ||
        (pName->EqualNoCase("unshift")) ||
        (pName->EqualNoCase("reverse")) ||
        (pName->EqualNoCase("sort")) ||
        (pName->EqualNoCase("splice")) ||
        (pName->EqualNoCase("slice")) ||
        (pName->EqualNoCase("sortOn")) ||
        (pName->EqualNoCase("toString"))
#endif
    )
    {
        APT_DEBUGPRINT(APT_DEBUG_MSG_WARNING_LVL, "AptArray: Incorrect case for '%s'.\n", pName->c_str());
        APT_ASSERT(0);
    }
#endif

    char *pEnd = 0;
    int nRet   = strtol(pName->c_str(), &pEnd, 10);
    int nLen   = pName->Size();
    if (pContext && nLen > 0 && (pEnd == pName->c_str() + nLen)) // if it ended at the null termination, and was non-empty, it was a number as a string
    {
        return pContext->c_array()->get(nRet);
    }

    // Return NULL (rather than gpUndefinedValue) to allow the caller to detect an unknown member.
    return Lookup(pName);
}

static int _isIndex(const char *szName)
{
    int nNum = atoi(szName);
    if (nNum == 0)
    {
        if (szName[0] == '0')
            return 1;
        return 0;
    }
    return 1;
}

/**
 * Sets a member of the array; for an AptArray the member name must be a numeric index.
 * @param pContext The context the set is performed against.
 * @param pName The member name being set.
 * @param pValue The value to set.
 * @return true if the member was successfully set (i.e. pName was an index), false otherwise.
 */
bool AptArray::objectMemberSet(AptValue *const pContext, const AptNativeString *const pName, AptValue *const pValue)
{
    APT_ASSERT(pContext->isArray());

    if (_isIndex(pName->c_str()))
    {
        int nNum = atoi(pName->c_str());
        pContext->c_array()->set(nNum, pValue == 0 ? gpUndefinedValue : pValue);
        return (true);
    }
    return (false);
}

#if APT_USE_ARRAY_METHODS
NATIVE_MEMBER_FUNCTION(AptArray, concat)
{
    if (pThis->isArray())
    {
        AptArray *pArray    = pThis->c_array();
        AptArray *pNewArray = new AptArray();
        int i;

        // copy original array
        for (i = 0; i < pArray->mnLength; i++)
        {
            pNewArray->set(pNewArray->mnLength, pArray->GetAt(i));
        }
        // flatten concat parameters
        for (i = 0; i < nParams; i++)
        {
            AptValue *pValue = gAptActionInterpreter.stackAt(i);

            if (pValue->isArray())
            {
                AptArray *pArray2 = pValue->c_array();
                for (int j = 0; j < pArray2->mnLength; j++)
                {
                    pNewArray->set(pNewArray->mnLength, pArray2->GetAt(j));
                }
            }
            else
            {
                pNewArray->set(pNewArray->mnLength, pValue);
            }
        }

        return pNewArray;
    }

    return gpUndefinedValue;
}

NATIVE_MEMBER_FUNCTION(AptArray, join)
{
    if (pThis->isArray())
    {
        AptArray *pArray = pThis->c_array();
        AptNativeString sBuf;
        AptNativeString sSeparator;

        if (nParams > 0)
        {
            gAptActionInterpreter.stackAt(0)->toString(sSeparator);
            pArray->toString(sBuf, sSeparator.ConstRawPtr());
        }
        else
        {
            pArray->toString(sBuf);
        }

        AptString *pString = AptString::Create();
        pString->str       = sBuf;
        return pString;
    }

    return gpUndefinedValue;
}

NATIVE_MEMBER_FUNCTION(AptArray, pop)
{
    AptValue *pRet = gpUndefinedValue;

    if (pThis->isArray())
    {
        AptArray *pArray = pThis->c_array();

        if (pArray->mnLength > 0)
        {
            pRet = pArray->get(pArray->mnLength - 1);
            // We don't decrement here otherwise the object would be destroyed before being added to
            // the stack; the dec happens outside this function in callMethod.
            pArray->mnLength--;
            pArray->mpValues[pArray->mnLength] = NULL;
        }
    }

    return pRet;
}

NATIVE_MEMBER_FUNCTION(AptArray, push)
{
    if (pThis->isArray())
    {
        AptArray *pArray = pThis->c_array();
        for (int i = 0; i < nParams; i++)
        {
            pArray->set(pArray->mnLength, gAptActionInterpreter.stackAt(i));
        }

        return AptInteger::Create(pArray->mnLength);
    }

    return gpUndefinedValue;
}

NATIVE_MEMBER_FUNCTION(AptArray, shift)
{
    AptValue *pRet = gpUndefinedValue;

    if (pThis->isArray())
    {
        AptArray *pArray = pThis->c_array();

        if (pArray->mnLength > 0)
        {
            pRet = pArray->get(0);
            pArray->mnLength--;
            // We don't decrement here otherwise the object would be destroyed before being added to
            // the stack; the dec happens outside this function in callMethod.
            if (pArray->mnLength)
            {
                memmove(&pArray->mpValues[0], &pArray->mpValues[1], sizeof(AptValue *) * pArray->mnLength);
            }
            pArray->mpValues[pArray->mnLength] = NULL;
        }
    }
    return pRet;
}

NATIVE_MEMBER_FUNCTION(AptArray, unshift)
{
    if (pThis->isArray())
    {
        AptArray *pArray = pThis->c_array();
        pArray->_reserve(pArray->mnLength + nParams);
        if (nParams)
        {
            // Shift pArray->mnLength (not nParams) elements to make room for the new ones.
            memmove(&pArray->mpValues[nParams], &pArray->mpValues[0], sizeof(AptValue *) * pArray->mnLength);
            pArray->mnLength += nParams;
            for (int i = 0; i < nParams; i++)
            {
                pArray->mpValues[i] = NULL;
                pArray->set(i, gAptActionInterpreter.stackAt(i));
            }
        }

        return AptInteger::Create(pArray->mnLength);
    }
    return gpUndefinedValue;
}

/**
 * Default comparator used by sort(): Flash compares elements in string form
 * (e.g. 11 < 2 < 'Z' < 'a'), so the elements are converted to strings for comparison.
 */
int AptArray::defaultSortCompareFunc(const void *a, const void *b)
{
    AptValue *pA = *(AptValue **)a;
    AptValue *pB = *(AptValue **)b;

    TO_STRING(pA, psA);
    TO_STRING(pB, psB);
    return strcmp(*psA, *psB);
}

AptNativeString strFieldName; // Field name used by the sortOn() compare function.

static AptValue *gpfnScriptSortFunctionFunction = 0;
static AptCIH *gpfnScriptSortFunctionContext    = 0;

/** Comparator used by sort() when a custom script comparison function was supplied. */
int AptArray::scriptFunctionSortFunc(const void *a, const void *b)
{
    if (!gpfnScriptSortFunctionFunction)
        return 0;

    AptValue *pA = *(AptValue **)a;
    AptValue *pB = *(AptValue **)b;

    APT_DEFINE_ACTION_SETUP(gpfnScriptSortFunctionContext, gpfnScriptSortFunctionFunction,
                            "AptArraySortScriptFunction", AptActionType_ArraySortFunction);
    void *pSavedValue = gAptActionInterpreter.PrepareForExecution(&oActionSetup);

    gAptActionInterpreter.stackPush(pB);
    gAptActionInterpreter.stackPush(pA);
    gAptActionInterpreter.callFunction(gpfnScriptSortFunctionContext, gpfnScriptSortFunctionFunction, 2);

    int nResult = gAptActionInterpreter.stackAt(0)->toInteger();
    gAptActionInterpreter.stackPop();

    gAptActionInterpreter.CleanupAfterExecution(pSavedValue, &oActionSetup); // oActionSetup is defined by the macro

    return (nResult);
}

NATIVE_MEMBER_FUNCTION(AptArray, sort)
{
    if (pThis->isArray())
    {
        AptArray *pArray = pThis->c_array();
        if (nParams == 0)
        {
            qsort(pArray->mpValues, pArray->mnLength, sizeof(AptValue *), defaultSortCompareFunc);
        }
        else
        {
            gpfnScriptSortFunctionFunction = gAptActionInterpreter.stackAt(0);
            gpfnScriptSortFunctionContext  = gpfnScriptSortFunctionFunction->c_scriptfunction()->mpCIH;
            qsort(pArray->mpValues, pArray->mnLength, sizeof(AptValue *), scriptFunctionSortFunc);
        }
    }
    return gpUndefinedValue;
}

/**
 * Compares two AptValues, either directly or via a named property of each.
 * @param value First value to compare.
 * @param equals Second value to compare against (directly, or against a property of value).
 * @param equalsStr If equals is a string, its contained string (avoids repeated conversion).
 * @param property Property name to compare value.property against equals, or NULL to compare value itself.
 * @return 0 if the values are equal, non-zero otherwise.
 */
int AptArray::objectFindComparator(AptValue *value, AptValue *equals, const AptNativeString *equalsStr, const AptNativeString *property)
{
    if (property) // What we are comparing is a property of the value, not the value itself of the object
    {
        if (!value->isObject())
        {
            return 1; // not an object; comparison fails
        }

        value = ((AptObject *)value)->Lookup(property);
        if (value == NULL)
        {
            return 1; // no such property; comparison fails
        }
    }

    if (value == equals)
    {
        return 0; // value being compared is the value being compared against, so they must be equal.
    }

    AptVirtualFunctionTable_Indices valueType  = value->getVtblIndex();
    AptVirtualFunctionTable_Indices equalsType = equals->getVtblIndex();

    // Booleans
    if (valueType == AptVFT_Boolean && equalsType == AptVFT_Boolean)
    {
        return (int)value->toBool() - (int)equals->toBool();
    }

    // Floats or integers
    if ((valueType == AptVFT_Float || valueType == AptVFT_Integer) &&
        (equalsType == AptVFT_Float || equalsType == AptVFT_Integer))
    {
        float diff = AptValueHelper::NumberToFloat(value) - AptValueHelper::NumberToFloat(equals);

        if (diff > APT_EPSILON)
        {
            return 1;
        }
        else if (diff < -APT_EPSILON)
        {
            return -1;
        }
        else
        {
            return 0;
        }
    }

    // Strings
    if (valueType == AptVFT_StringValue && equalsType == AptVFT_StringValue)
    {
        TO_STRING(value, valueStr);
        return strcmp(valueStr->GetBuffer(), equalsStr->GetBuffer());
    }

    // not comparable; comparison fails
    return 1;
}

/**
 * Looks for the element in an array with a given value.
 * @param comparator Function used to compare (usually AptArray::objectFindComparator).
 * @param start First index to compare in the array.
 * @param size Array size.
 * @param values Array of AptValues to look in.
 * @param equals Second value to compare against (directly with values[i], or with a property of it).
 * @param equalsStr If equals is a string, its contained string (avoids repeated conversion).
 * @param propertyStr Property name to compare, or NULL to compare values[i] directly.
 * @param reverse True to search in reverse order ([start ... 0], decreasing).
 * @return Index of the matching element, or -1 if not found.
 */
static int find(AptArray::fnFindComparator_t comparator, int start, int size, AptValue **values,
                AptValue *equals, const AptNativeString *equalsStr, const AptNativeString *propertyStr, bool reverse)
{
    if (!reverse)
    {
        if (start < 0)
        {
            start = 0;
        }
        for (int32_t i = start; i < size; i++)
        {
            if (comparator(values[i], equals, equalsStr, propertyStr) == 0)
            {
                return i;
            }
        }
    }
    else
    {
        if (start >= size)
        {
            start = size - 1;
        }
        for (int32_t i = start; i >= 0; --i)
        {
            if (comparator(values[i], equals, equalsStr, propertyStr) == 0)
            {
                return i;
            }
        }
    }
    return -1;
}

int AptArray::find(fnFindComparator_t comparator, int start, AptValue *equals, AptValue *property, bool reverse)
{
    TO_STRING(equals, equalsStr);
    if (property && !property->isUndefined())
    {
        TO_STRING(property, propertyStr);
        return ::find(comparator, start, mnLength, mpValues, equals, equalsStr, propertyStr, reverse);
    }
    else
    {
        return ::find(comparator, start, mnLength, mpValues, equals, equalsStr, 0, reverse);
    }
}

/** Default comparator used by sortOn(): compares the named property (strFieldName) of two elements. */
int AptArray::defaultSortOnCompareFunc(const void *a, const void *b)
{
    AptValue *pA = *(AptValue **)a;
    AptValue *pB = *(AptValue **)b;

    if (pA->isObject() && pB->isObject())
    {
        AptObject *pAObject = pA->c_object();
        AptObject *pBObject = pB->c_object();

        AptValue *pAValue = NULL;
        AptValue *pBValue = NULL;

        pAValue = pAObject->Lookup(&strFieldName);
        if (pAValue)
        {
            pBValue = pBObject->Lookup(&strFieldName);
            if (pBValue)
            {
                // Call this function to prevent code duplication; see its definition for more info.
                return defaultSortCompareFunc(&pAValue, &pBValue);
            }
        }
    }
    else if (pA->isArray() && pB->isArray())
    {
        AptValue *pAValue = NULL;
        AptValue *pBValue = NULL;
        pAValue           = pA->c_array()->get(atoi(strFieldName));
        pBValue           = pB->c_array()->get(atoi(strFieldName));

        // Call this function to prevent code duplication; see its definition for more info.
        return defaultSortCompareFunc(&pAValue, &pBValue);
    }
    else
    {
        // Array items that are not objects don't have field names, so we don't compare them at all.
        // Flash defaults to array.sort in this case, but we just return equal (undefined result),
        // and assert so we know about it.
        APT_ASSERT(NOT_REACHED);
        return 0;
    }
    // Reached when the field name is not present on the object: return 0 (equal), so no sorting happens.
    return 0;
}

NATIVE_MEMBER_FUNCTION(AptArray, sortOn)
{
    if (pThis->isArray())
    {
        AptArray *pArray = pThis->c_array();
        if (nParams > 0)
        {
            // At this time, Apt does not support other parameters like Array.NUMERIC to this function.
            APT_ASSERT(nParams == 1)                                  //  Array.NUMERIC and related Array.sortOn options values are undefined at run-time.
            gAptActionInterpreter.stackAt(0)->toString(strFieldName); // get fieldname and store it globally
            qsort(pArray->mpValues, pArray->mnLength, sizeof(AptValue *), defaultSortOnCompareFunc);
            strFieldName.Clear(); //  Clear the temporary string afterward
        }
    }
    return gpUndefinedValue;
}

NATIVE_MEMBER_FUNCTION(AptArray, reverse)
{
    if (pThis->isArray())
    {
        AptArray *pArray = pThis->c_array();
        AptValue *pTemp;

        for (int i = 0; i < pArray->mnLength / 2; i++)
        {
            pTemp                                      = pArray->mpValues[i];
            pArray->mpValues[i]                        = pArray->mpValues[pArray->mnLength - i - 1];
            pArray->mpValues[pArray->mnLength - i - 1] = pTemp;
        }
        return pThis;
    }
    return gpUndefinedValue;
}

NATIVE_MEMBER_FUNCTION(AptArray, splice)
{
    if (pThis->isArray())
    {
        AptArray *pArray = pThis->c_array();

        if (nParams > 0)
        {
            AptValue *pStackValue = gAptActionInterpreter.stackAt(0);
            if (pStackValue->isUndefined())
            {
                return gpUndefinedValue;
            }

            int nStart = pStackValue->toInteger();
            if (nStart < 0)
            {
                // convert to non-negative index
                nStart = pArray->mnLength + nStart;
                if (nStart < 0)
                {
                    nStart = 0;
                }
            }
            APT_ASSERT(nStart >= 0);

            if (nStart >= pArray->mnLength)
            {
                nStart = pArray->mnLength;
            }

            // check how many items should be deleted

            APT_ASSERT(nStart <= pArray->mnLength);
            int nNumToDelete = pArray->mnLength - nStart;

            if (nParams > 1)
            {
                AptValue *pStackValue = gAptActionInterpreter.stackAt(1);
                if (pStackValue->isUndefined())
                {
                    return gpUndefinedValue;
                }
                else
                {
                    nNumToDelete = pStackValue->toInteger();
                    if (nNumToDelete > (pArray->mnLength - nStart))
                    {
                        nNumToDelete = pArray->mnLength - nStart;
                    }
                }
            }

            APT_ASSERT(nStart >= 0);
            APT_ASSERT(nStart <= pArray->mnLength);
            APT_ASSERT(nNumToDelete <= (pArray->mnLength - nStart));

            // if deleteCount param was negative, we return undefined
            if (nNumToDelete >= 0)
            {
                int i               = 0;
                AptArray *pNewArray = new AptArray();

                if (nNumToDelete > 0)
                {
                    APT_ASSERT(nStart < pArray->mnLength);

                    for (i = 0; i < nNumToDelete; i++)
                    {
                        AptValue *pValue = pArray->GetAt(nStart + i);
                        if (pValue == NULL)
                        {
                            pValue = gpUndefinedValue;
                        }
                        // Store deleted elements in the returned array.
                        pNewArray->set(pNewArray->mnLength, pValue);

                        APT_DECSAFE(pValue);
                    }

                    // close the gap left by the deleted items
                    memmove(&pArray->mpValues[nStart], &pArray->mpValues[nStart + nNumToDelete], sizeof(AptValue *) * (pArray->mnLength - (nStart + nNumToDelete)));
                    for (i = 0; i < nNumToDelete; i++)
                    {
                        pArray->mpValues[(pArray->mnLength - nNumToDelete) + i] = NULL;
                    }
                    pArray->mnLength -= nNumToDelete;
                }

                if (nParams > 2)
                {
                    // check how many items should be inserted
                    int nNumToAdd = nParams - 2;
                    APT_ASSERT(nNumToAdd >= 1);

                    pArray->_reserve(pArray->mnLength + nNumToAdd);

                    APT_ASSERT(nStart >= 0);
                    if (pArray->mnLength - nStart > 0)
                    {
                        // everything to the right of (and including) nStart is shifted to the right, to make room for the inserted items
                        memmove(&pArray->mpValues[nStart + nNumToAdd], &pArray->mpValues[nStart], sizeof(AptValue *) * (pArray->mnLength - nStart));
                    }

                    // set the inserted items in the array
                    pArray->mnLength += nNumToAdd;

                    APT_ASSERT(nStart < pArray->mnLength);
                    APT_ASSERT((nStart + nNumToAdd) <= pArray->mnLength);
                    for (i = 0; i < nNumToAdd; i++)
                    {
                        pArray->mpValues[nStart + i] = NULL;
                        pArray->set(nStart + i, gAptActionInterpreter.stackAt(i + 2));
                    }
                }

                APT_ASSERT(nNumToDelete >= 0);
                APT_ASSERT(pNewArray->mnLength == nNumToDelete);
                return pNewArray;
            }
        }
    }

    return gpUndefinedValue;
}

NATIVE_MEMBER_FUNCTION(AptArray, slice)
{
    if (pThis->isArray())
    {
        AptArray *pArray    = pThis->c_array();
        AptArray *pNewArray = NULL;

        int i;

        // Supports slice() with no arguments, and negative arguments (both valid in Flash).

        int nStart = 0;                // Default is 0
        int nEnd   = pArray->mnLength; // Default is the end of the string.

        if (nParams > 0)
        {
            nStart = gAptActionInterpreter.stackAt(0)->toInteger();
            // Negative offsets go backwards from the end of the array.
            if (nStart < 0)
            {
                nStart = pArray->mnLength + nStart; // actually subtracts (nStart is negative)
            }
        }
        if (nParams > 1)
        {
            nEnd = gAptActionInterpreter.stackAt(1)->toInteger();
            // Negative offsets go backwards from the end of the array.
            if (nEnd < 0)
            {
                nEnd = pArray->mnLength + nEnd; // actually subtracts (nEnd is negative)
            }
            else if (nEnd > pArray->mnLength)
            {
                nEnd = pArray->mnLength;
            }
        }

        // nStart cannot be smaller then nEnd.
        // neither value can be negative (should have already handled that).
        // nStart == nEnd is valid and returns an empty array.
        if (nStart > nEnd || nStart < 0 || nEnd < 0)
        {
            return gpUndefinedValue;
        }

        // now create a new array.
        pNewArray = new AptArray();

        // copy from original array
        for (i = nStart; i < nEnd; i++) // do not include last element
        {
            pNewArray->set(pNewArray->mnLength, pArray->GetAt(i));
        }

        return pNewArray;
    }
    return gpUndefinedValue;
}
#endif
