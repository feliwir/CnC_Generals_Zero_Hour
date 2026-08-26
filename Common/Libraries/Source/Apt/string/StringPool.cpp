/**
 * Defines a pool of pre-defined, reserved strings. Avoids hashing.
 */

#include "Apt.h"
#include "string/StringPool.h"
#include "AptGlobal.h"
#include "AptValue/AptValueVector.h"

#include "MainInline.h"

AptNativeString StringPool::saConstant[SC_LAST];
AptString **StringPool::spPool     = NULL;
int32_t StringPool::spPoolSize     = 0;
AptString *StringPool::spFirstFree = NULL;

#if (APT_DEBUG_LEVEL >= APT_DEBUG_NORMAL)
int32_t sMaxOccurences             = 50;
int32_t sCurrentMemorySaved        = 0;
int32_t sMaxMemorySaved            = 0x80000000;
const int32_t sAptStringOldSize    = 8; //  Type + NativeString
const int32_t sStringOldBufferSize = 6; //  RefCount + Size + Capacity
const int32_t sStringNewBufferSize = 8; //  RefCount + Size + Capacity + Hash
int32_t sMaxSingle                 = 0;
int32_t sMaxTotal                  = 0;
int32_t sCurrentSingle             = 0;
int32_t sCurrentTotal              = 0;
#endif

// Building constant string data to use at runtime
#define BUILD_STATIC_STRING_DATA(_) {0x0001, static_cast<uint16_t>(strlen(_)), 0x0001, 0x0000}, _

EA_PREFIX_ALIGN(4)
EAStringC::StaticStringHelperT sStringPoolData[SC_LAST] EA_POSTFIX_ALIGN(4) =
    {
        {BUILD_STATIC_STRING_DATA("__proto__")},
        {BUILD_STATIC_STRING_DATA("_alpha")},
        {BUILD_STATIC_STRING_DATA("_currentframe")},
        {BUILD_STATIC_STRING_DATA("_down")},
        {BUILD_STATIC_STRING_DATA("_droptarget")},
        {BUILD_STATIC_STRING_DATA("_focusrect")},
        {BUILD_STATIC_STRING_DATA("_framesloaded")},
        {BUILD_STATIC_STRING_DATA("_global")},
        {BUILD_STATIC_STRING_DATA("_height")},
        {BUILD_STATIC_STRING_DATA("_highquality")},
        {BUILD_STATIC_STRING_DATA("_left")},
        {BUILD_STATIC_STRING_DATA("_name")},
        {BUILD_STATIC_STRING_DATA("_quality")},
        {BUILD_STATIC_STRING_DATA("_right")},
        {BUILD_STATIC_STRING_DATA("_rotation")},
        {BUILD_STATIC_STRING_DATA("_soundbuftime")},
        {BUILD_STATIC_STRING_DATA("_target")},
        {BUILD_STATIC_STRING_DATA("_totalframes")},
        {BUILD_STATIC_STRING_DATA("_type")},
        {BUILD_STATIC_STRING_DATA("_up")},
        {BUILD_STATIC_STRING_DATA("_url")},
        {BUILD_STATIC_STRING_DATA("_visible")},
        {BUILD_STATIC_STRING_DATA("_width")},
        {BUILD_STATIC_STRING_DATA("_x")},
        {BUILD_STATIC_STRING_DATA("_xmouse")},
        {BUILD_STATIC_STRING_DATA("_xscale")},
        {BUILD_STATIC_STRING_DATA("_y")},
        {BUILD_STATIC_STRING_DATA("_ymouse")},
        {BUILD_STATIC_STRING_DATA("_yscale")},
        {BUILD_STATIC_STRING_DATA("aa")},
        {BUILD_STATIC_STRING_DATA("ab")},
        {BUILD_STATIC_STRING_DATA("array")},
        {BUILD_STATIC_STRING_DATA("ba")},
        {BUILD_STATIC_STRING_DATA("bb")},
        {BUILD_STATIC_STRING_DATA("boolean")},
        {BUILD_STATIC_STRING_DATA("center")},
        {BUILD_STATIC_STRING_DATA("color")},
        {BUILD_STATIC_STRING_DATA("controller")},
        {BUILD_STATIC_STRING_DATA("date")},
        {BUILD_STATIC_STRING_DATA("error")},
        {BUILD_STATIC_STRING_DATA("false")},
        {BUILD_STATIC_STRING_DATA("function")},
        {BUILD_STATIC_STRING_DATA("ga")},
        {BUILD_STATIC_STRING_DATA("gb")},
        {BUILD_STATIC_STRING_DATA("getRGB")},
        {BUILD_STATIC_STRING_DATA("getTransform")},
        {BUILD_STATIC_STRING_DATA("left")},
        {BUILD_STATIC_STRING_DATA("loadvars")},
        {BUILD_STATIC_STRING_DATA("movieclip")},
        {BUILD_STATIC_STRING_DATA("MovieClipLoader")},
        {BUILD_STATIC_STRING_DATA("none")},
        {BUILD_STATIC_STRING_DATA("null")},
        {BUILD_STATIC_STRING_DATA("number")},
        {BUILD_STATIC_STRING_DATA("object")},
        {BUILD_STATIC_STRING_DATA("onData")},
        {BUILD_STATIC_STRING_DATA("onDragOut")},
        {BUILD_STATIC_STRING_DATA("onDragOver")},
        {BUILD_STATIC_STRING_DATA("onEnterFrame")},
        {BUILD_STATIC_STRING_DATA("onKeyDown")},
        {BUILD_STATIC_STRING_DATA("onKeyUp")},
        {BUILD_STATIC_STRING_DATA("onLoad")},
        {BUILD_STATIC_STRING_DATA("onMouseDown")},
        {BUILD_STATIC_STRING_DATA("onMouseMove")},
        {BUILD_STATIC_STRING_DATA("onMouseUp")},
        {BUILD_STATIC_STRING_DATA("onMouseWheel")},
        {BUILD_STATIC_STRING_DATA("onPress")},
        {BUILD_STATIC_STRING_DATA("onRelease")},
        {BUILD_STATIC_STRING_DATA("onReleaseOutside")},
        {BUILD_STATIC_STRING_DATA("onRollOut")},
        {BUILD_STATIC_STRING_DATA("onRollOver")},
        {BUILD_STATIC_STRING_DATA("onUnload")},
        {BUILD_STATIC_STRING_DATA("prototype")},
        {BUILD_STATIC_STRING_DATA("ra")},
        {BUILD_STATIC_STRING_DATA("rb")},
        {BUILD_STATIC_STRING_DATA("right")},
        {BUILD_STATIC_STRING_DATA("setRGB")},
        {BUILD_STATIC_STRING_DATA("setTransform")},
        {BUILD_STATIC_STRING_DATA("sound")},
        {BUILD_STATIC_STRING_DATA("string")},
        {BUILD_STATIC_STRING_DATA("super")},
        {BUILD_STATIC_STRING_DATA("textformat")},
        {BUILD_STATIC_STRING_DATA("this")},
        {BUILD_STATIC_STRING_DATA("true")},
        {BUILD_STATIC_STRING_DATA("undefined")},
        {BUILD_STATIC_STRING_DATA("xMax")},
        {BUILD_STATIC_STRING_DATA("xMin")},
        {BUILD_STATIC_STRING_DATA("XML")},
        {BUILD_STATIC_STRING_DATA("yMax")},
        {BUILD_STATIC_STRING_DATA("yMin")},
        {BUILD_STATIC_STRING_DATA("__New__")}
#if defined(APT_3D)
        ,
        {BUILD_STATIC_STRING_DATA("_xrotation")},
        {BUILD_STATIC_STRING_DATA("_yrotation")},
        {BUILD_STATIC_STRING_DATA("_z")}
#endif
        ,
        {BUILD_STATIC_STRING_DATA("filters")},
        {BUILD_STATIC_STRING_DATA("normal")},
        {BUILD_STATIC_STRING_DATA("layer")},
        {BUILD_STATIC_STRING_DATA("multiply")},
        {BUILD_STATIC_STRING_DATA("screen")},
        {BUILD_STATIC_STRING_DATA("lighten")},
        {BUILD_STATIC_STRING_DATA("darken")},
        {BUILD_STATIC_STRING_DATA("difference")},
        {BUILD_STATIC_STRING_DATA("add")},
        {BUILD_STATIC_STRING_DATA("subtract")},
        {BUILD_STATIC_STRING_DATA("invert")},
        {BUILD_STATIC_STRING_DATA("alpha")},
        {BUILD_STATIC_STRING_DATA("erase")},
        {BUILD_STATIC_STRING_DATA("overlay")},
        {BUILD_STATIC_STRING_DATA("hardlight")},
        {BUILD_STATIC_STRING_DATA("flash.filters.DropShadowFilter")},
        {BUILD_STATIC_STRING_DATA("flash.filters.BlurFilter")},
        {BUILD_STATIC_STRING_DATA("flash.filters.GlowFilter")},
        {BUILD_STATIC_STRING_DATA("flash.filters.BevelFilter")},
        {BUILD_STATIC_STRING_DATA("flash.filters.GradientGlowFilter")},
        {BUILD_STATIC_STRING_DATA("flash.filters.ConvolutionFilter")},
        {BUILD_STATIC_STRING_DATA("flash.filters.ColorMatrixFilter")},
        {BUILD_STATIC_STRING_DATA("flash.filters.GradientBevelFilter")},
        {BUILD_STATIC_STRING_DATA("alphas")},
        {BUILD_STATIC_STRING_DATA("angle")},
        {BUILD_STATIC_STRING_DATA("bias")},
        {BUILD_STATIC_STRING_DATA("blurX")},
        {BUILD_STATIC_STRING_DATA("blurY")},
        {BUILD_STATIC_STRING_DATA("clamp")},
        {BUILD_STATIC_STRING_DATA("colors")},
        {BUILD_STATIC_STRING_DATA("distance")},
        {BUILD_STATIC_STRING_DATA("divisor")},
        {BUILD_STATIC_STRING_DATA("filterType")},
        {BUILD_STATIC_STRING_DATA("full")},
        {BUILD_STATIC_STRING_DATA("hideObject")},
        {BUILD_STATIC_STRING_DATA("highlightAlpha")},
        {BUILD_STATIC_STRING_DATA("highlightColor")},
        {BUILD_STATIC_STRING_DATA("inner")},
        {BUILD_STATIC_STRING_DATA("knockout")},
        {BUILD_STATIC_STRING_DATA("matrix")},
        {BUILD_STATIC_STRING_DATA("matrixX")},
        {BUILD_STATIC_STRING_DATA("matrixY")},
        {BUILD_STATIC_STRING_DATA("outer")},
        {BUILD_STATIC_STRING_DATA("preserveAlpha")},
        {BUILD_STATIC_STRING_DATA("quality")},
        {BUILD_STATIC_STRING_DATA("shadowAlpha")},
        {BUILD_STATIC_STRING_DATA("shadowColor")},
        {BUILD_STATIC_STRING_DATA("strength")},
        {BUILD_STATIC_STRING_DATA("ratios")},
        {BUILD_STATIC_STRING_DATA("type")},
#if defined(APT_USE_DEBUG_NAMES)
        {BUILD_STATIC_STRING_DATA("_debugName")},
#endif
        {BUILD_STATIC_STRING_DATA("_CustomControlType")},
};

/**
 * Maps the static constant strings into the reserved-string table, memory-mapping them to save
 * dynamic allocations, and allocates the temporary string pool.
 */
void StringPool::Initialize(const int32_t iSize)
{
    int32_t i;
    for (i = 0; i < SC_LAST; ++i)
    {
        //  Sometimes static initialization doesn't happen in time; force it here if needed.
        if (saConstant[i].IsValid() == false)
        {
            saConstant[i].Validate();
        }

        saConstant[i] = sStringPoolData[i]; // Set Static string
    }

    spPool = APT_MALLOC_ARRAY(AptString *, iSize);
    memset(spPool, 0, sizeof(AptString *) * iSize);
    spPoolSize = iSize;
}

/**
 * Releases the temporary pool and the reserved constant strings.
 */
void StringPool::Teardown()
{
    int32_t i;
    AptString *pString;
    AptString *pNextString;
    bool bRemoved;

    //  Check all the clamped strings
    for (i = 0; i < spPoolSize; ++i)
    {
        bRemoved = false;
        pString  = spPool[i];
        while (pString != NULL)
        {
            //  There is a string!
            APT_ASSERT(pString->getGCRoot() == AptValue::MAX_GCROOT); //  Check that this is a clamped counter
            APT_ASSERT(pString->getRefCount() == 1);                  //  Check that this is the last reference

            pNextString = pString->GetNext();

#if (APT_DEBUG_LEVEL >= APT_DEBUG_NORMAL)
            //  We fix the saved size (it was counted as duplicated, it should be erased)

            //  Add duplicated (it was removed for last call)
            sCurrentMemorySaved += GetDeltaDuplicated(pString);
            //  Remove created
            sCurrentMemorySaved -= GetDeltaCreated(pString);
#endif

            APT_DEC(pString);
            pString  = pNextString;
            bRemoved = true;
        }

        if (bRemoved)
        {
            //  Collect the temporary destructed strings for that column of hash
            AptGetLib()->mpValuesToRelease->ReleaseValues();
        }
    }

#if (APT_DEBUG_LEVEL >= APT_DEBUG_NORMAL)
    APT_ASSERT(sCurrentMemorySaved == 0); //  We should reach 0 here as all the strings are removed
#endif

    APT_FREE_ARRAY(spPool, AptString *, spPoolSize);
    spPoolSize = 0;

    for (i = 0; i < SC_LAST; ++i)
    {
        //  There should be one reference only on the string
        //  This assert is not important but it can help to detect some memory leak
        APT_ASSERT(saConstant[i].GetInternalRefCount() == 1);
        saConstant[i].ClearConstantValue(); //  Free the string
    }
}

/**
 * Looks up a reserved string by text, comparing lengths first to speed up the search.
 */
const AptNativeString *StringPool::GetString(const char *const pText)
{
    int32_t length = static_cast<uint16_t>(strlen(pText));
    int32_t i;

    for (i = SC_FIRST; i < SC_LAST; ++i)
    {
        if (saConstant[i].Size() != length)
        {
            //  Not the same size, continue
            continue;
        }

        if (saConstant[i].EqualNoCase(pText))
        {
            return (&saConstant[i]);
        }
    }
    return (NULL);
}

/**
 * Retrieves a free string from the temporary pool of strings, creating a new one if none is free.
 */
AptString *StringPool::GetFromPool(const char *const pText)
{
    uint16_t uHash  = AptNativeString::CalculateHashValue(pText);
    uint16_t uIndex = uHash % spPoolSize;
    AptString *pString;

#if (APT_DEBUG_LEVEL >= APT_DEBUG_NORMAL)
    ++sCurrentTotal;
    if (sCurrentTotal > sMaxTotal)
    {
        sMaxTotal = sCurrentTotal;
    }
#endif

    pString = spPool[uIndex];
    while (pString != NULL)
    {
        AptNativeString *pNativeString;

        pNativeString = pString->GetInternalString();
        if (pNativeString->GetHashValue() == uHash)
        {
            //  The text could be the same
            if (pNativeString->Equal(pText))
            {
                //  Found it!
                uint32_t iCurrentGCRoot;
                iCurrentGCRoot = pString->getGCRoot();
                if (iCurrentGCRoot != AptValue::MAX_GCROOT)
                {
                    //  We didn't reach yet uMaxGCroot, we can increase
                    //  The counter is clamped to uMaxGCRoot, so if we reach this number
                    //  we are unable to know the real value
                    pString->incGCRoot();
                }

#if (APT_DEBUG_LEVEL >= APT_DEBUG_NORMAL)
                //  Add the delta memory for duplication
                sCurrentMemorySaved += GetDeltaDuplicated(pString);

                if (sCurrentMemorySaved > sMaxMemorySaved)
                {
                    sMaxMemorySaved = sCurrentMemorySaved;
                }
#endif
                return (pString);
            }
        }
        pString = pString->GetNext();
    }

    //  We didn't find it, we need to create a new one.
    //  Never use new AptString directly, always go through AptString::Create.
    pString      = AptString::Create();
    pString->str = pText;
    pString->str.CalculateHashValue();
    APT_ASSERT(uHash == pString->str.GetHashValue()); //  Check that hash-value is still the same

    //  Now add it to the list
    pString->SetNext(spPool[uIndex]);
    spPool[uIndex] = pString;

    APT_INC(pString);                      //  Increase the reference counting, so it won't be destroyed
    APT_ASSERT(pString->getGCRoot() == 0); //  Check that GCRoot is not used for AptString
    pString->incGCRoot();

#if (APT_DEBUG_LEVEL >= APT_DEBUG_NORMAL)
    //  Add the delta memory for creation
    sCurrentMemorySaved += GetDeltaCreated(pString);

    if (sCurrentMemorySaved > sMaxMemorySaved)
    {
        sMaxMemorySaved = sCurrentMemorySaved;
    }

    ++sCurrentSingle;
    if (sCurrentSingle > sMaxSingle)
    {
        sMaxSingle = sCurrentSingle;
    }
#endif
    return (pString);
}

/**
 * Removes a string from the temporary pool once its pool-relative reference count reaches zero.
 */
void StringPool::RemoveFromPool(AptString *const pString)
{
#if (APT_DEBUG_LEVEL >= APT_DEBUG_NORMAL)
    {
        uint16_t uHash = pString->str.GetHashValue(); //  If the string has been changed (!) and the hash-value is not recalculated
                                                      //  This will fire an assert (and help us to detect the issue)
        uint16_t uIndex         = uHash % spPoolSize;
        AptString *pLocalString = spPool[uIndex];
        bool bFound             = false;

        while (pLocalString != NULL)
        {
            if (pLocalString == pString)
            {
                bFound = true;
                break;
            }
            pLocalString = pLocalString->GetNext();
        }
        APT_ASSERT(bFound); //  Check that we remove an added string
                            //  This also helps to detect if the string has been changed (constant modification!)
                            //  Because the hash-table should not be the same
    }

    --sCurrentTotal;
#endif

    uint32_t iCurrentGCRoot = pString->getGCRoot();
    if (iCurrentGCRoot == AptValue::MAX_GCROOT)
    {
        //  This string reaches the max GCRoot (a lot of dup, certainly "this", "_global", ...)
        //  We remove it only at AptShutdowm because there is no way for us to know the real number

#if (APT_DEBUG_LEVEL >= APT_DEBUG_NORMAL)
        //  Remove the delta memory for duplication
        sCurrentMemorySaved -= GetDeltaDuplicated(pString);

        if (sCurrentMemorySaved > sMaxMemorySaved)
        {
            sMaxMemorySaved = sCurrentMemorySaved;
        }
#endif
        return;
    }
    pString->decGCRoot();
    if (iCurrentGCRoot != 1)
    {
        //  The counter was bigger than 1, we need to keep the string in memory

#if (APT_DEBUG_LEVEL >= APT_DEBUG_NORMAL)
        //  Remove the delta memory for duplication
        sCurrentMemorySaved -= GetDeltaDuplicated(pString);

        if (sCurrentMemorySaved > sMaxMemorySaved)
        {
            sMaxMemorySaved = sCurrentMemorySaved;
        }
#endif
        return;
    }

    //  The pool counter is null here, we need to remove it from the string
    uint16_t uHash          = pString->str.GetHashValue();
    uint16_t uIndex         = uHash % spPoolSize;
    AptString *pLocalString = spPool[uIndex];

#if (APT_DEBUG_LEVEL >= APT_DEBUG_NORMAL)
    //  Remove the delta memory for creation
    sCurrentMemorySaved -= GetDeltaCreated(pString);

    if (sCurrentMemorySaved > sMaxMemorySaved)
    {
        sMaxMemorySaved = sCurrentMemorySaved;
    }
    --sCurrentSingle;
#endif

    if (pLocalString == pString)
    {
        spPool[uIndex] = pString->GetNext();
        APT_DEC(pString);
        return;
    }
    while (pLocalString->GetNext() != pString)
    {
        pLocalString = pLocalString->GetNext();
        APT_ASSERT(pLocalString != NULL);
    }

    pLocalString->SetNext(pString->GetNext());

    APT_DEC(pString);
}

/**
 * Deletes all strings currently sitting in the temporary free list.
 */
void StringPool::ClearTemporaryPool()
{
    while (spFirstFree != NULL)
    {
        AptString *pNext = spFirstFree->mpNext;
        spFirstFree->DestroyGCPointers();
        delete spFirstFree; //  The only place where we do a real destruction
        spFirstFree = pNext;
    }
}

/**
 * @return false if any reserved string is empty or two consecutive reserved strings are equal.
 */
bool StringPool::CheckContent()
{
    int32_t i;
    for (i = 0; i < SC_LAST; ++i)
    {
        if (saConstant[i].IsEmpty())
        {
            return (false);
        }
        if (i + 1 < SC_LAST)
        {
            if (saConstant[i].CompareNoCase(saConstant[i + 1]) == 0)
            {
                return (false);
            }
        }
    }
    return (true);
}

#if (APT_DEBUG_LEVEL >= APT_DEBUG_NORMAL)
/**
 * @return the memory delta (in bytes, negative means saved) from creating a new pooled string.
 */
int32_t StringPool::GetDeltaCreated(AptString *const pString)
{
    int32_t iSize = 0;

    iSize -= sizeof(AptString) - sAptStringOldSize;
    int32_t iNewBufferSize = sStringNewBufferSize + pString->str.GetLength() + 1;
    iNewBufferSize         = (iNewBufferSize + 3) & -4;
    int32_t iOldBufferSize = sStringOldBufferSize + pString->str.GetLength() + 1;
    iOldBufferSize         = (iOldBufferSize + 3) & -4;
    iSize -= iNewBufferSize - iOldBufferSize;
    return (iSize);
}

/**
 * @return the memory delta (in bytes, positive means saved) from reusing a duplicated pooled string.
 */
int32_t StringPool::GetDeltaDuplicated(AptString *const pString)
{
    int32_t iSize = 0;

    iSize += sAptStringOldSize;
    int32_t iBufferSize = sStringOldBufferSize + pString->str.GetLength() + 1;
    iBufferSize         = (iBufferSize + 3) & -4;
    iSize += iBufferSize;
    return (iSize);
}
#endif
