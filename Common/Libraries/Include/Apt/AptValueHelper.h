/** Helper class exposing functions to look up/set members from hash tables inside AptValue. */

#pragma once

#include "AptString/EAString.h"
#include "AptValue/AptValue.h"
#include "AptValue/AptArray.h"
#include <cmath>

class AptValueHelper
{
  public:
    // All these functions return true only if pValue has a hash table inside it.
    // Last parameters are OUT parameters which will return right value back.
    static bool GetMember(AptValue *pValue, const char *szValue, int32_t *pOutInteger);
    static bool GetMember(AptValue *pValue, const char *szValue, float_t *pOutFloat);
    static bool GetMember(AptValue *pValue, const char *szValue, bool *pOutBool);
    static bool GetMember(AptValue *pValue, const char *szValue, char *pOutString, uint32_t nMaxChars);

    static bool GetMember(AptValue *pValue, const AptNativeString *pStringKey, int32_t *pOutInteger);
    static bool GetMember(AptValue *pValue, const AptNativeString *pStringKey, float_t *pOutFloat);
    static bool GetMember(AptValue *pValue, const AptNativeString *pStringKey, bool *pOutBool);
    static bool GetMember(AptValue *pValue, const AptNativeString *pStringKey, char *pOutString, uint32_t nMaxChars);

    static bool GetArrayMember(AptValue *pValue, int32_t nIndex, int32_t *pOutInteger);
    static bool GetArrayMember(AptValue *pValue, int32_t nIndex, float_t *pOutFloat);
    static bool GetArrayMember(AptValue *pValue, int32_t nIndex, bool *pOutBool);
    static bool GetArrayMember(AptValue *pValue, int32_t nIndex, char *pOutString, uint32_t nMaxChars);

    // Array index can also be a string.
    static bool GetArrayMember(AptValue *pValue, const char *szValue, int32_t *pOutInteger);
    static bool GetArrayMember(AptValue *pValue, const char *szValue, float_t *pOutFloat);
    static bool GetArrayMember(AptValue *pValue, const char *szValue, bool *pOutBool);
    static bool GetArrayMember(AptValue *pValue, const char *szValue, char *pOutString, uint32_t nMaxChars);

    /**
     * Exposes AptArray access without needing _Apt.h, for external developers who only get the
     * public Apt headers.
     */
    static AptValue *GetFromAptArray(AptValue *pArray, int nIndex);
    static void SetInAptArray(AptValue *pArray, int nIndex, AptValue *pValue);
    static int GetArrayLength(AptValue *pArray);

    static AptValue *GetMemberAsAptValue(AptValue *pValue, const char *szValue);
    static AptValue *GetMemberAsAptValue(AptValue *pValue, const AptNativeString *pKey);

    /**
     * @return whether a MovieClip is visible: true only if the _visible property of all the
     * MovieClip's parents and its own _visible property are true; false if pValue isn't a MovieClip.
     */
    static bool IsMovieClipVisible(AptValue *pValue);

    // Generic translation functions to convert AptValues to various primitive types.
    static int NumberToInteger(AptValue *value);
    static float NumberToFloat(AptValue *value);
    static bool GetBoolean(AptValue *value);
    static const char *GetString(AptValue *value, int *length);

    /**
     * Iterates the members of an Apt Object.
     * @code
     * for (AptHashItem* item = AptValueHelper::GetObjectItemFirst(object); item; item = AptValueHelper::GetObjectItemNext(object, item))
     * { ... }
     * @endcode
     */
    static AptHashItem *GetObjectItemFirst(AptValue *value);
    static AptHashItem *GetObjectItemNext(AptValue *value, AptHashItem *item);

    /** @return the rendering unit of the first shape found in the given MovieClip, or NULL. */
    static AptAssetRenderingUnit GetGeometryFromMovieClip(AptValue *pValue);

    /** @return the texture ID of the first shape found in the given MovieClip, or -1 if none. */
#if defined(APT_DECOUPLED_RENDERING)
    static int32_t GetTextureIdFromMovieClip(AptValue *pValue);
#endif

    // These return an AptValue* (unlike AptGetInternalVariable). Call APT_DEC on *ppOut when done with it.

    /** Returns the "undefined" value. */
    static void GetUndefined(AptValue **ppOut);

    /** Returns _global. */
    static void GetGlobal(AptValue **ppOut);

    /** Gets this context. Only available when AptDebugger is enabled. */
    static void GetThis(AptValue *currentContext, AptValue **output, int level);

    /** Gets the super context. Only available when AptDebugger is enabled. */
    static void GetSuper(AptValue *currentContext, AptValue **output, int callStackLevel);

    /** Returns _leveln, where n is a number; NULL if _level n does not yet exist. */
    static void GetLevel(int n, AptValue **ppOut);

    /**
     * Returns a child member of pValue, or NULL if it doesn't exist. Recurses; member name szValue
     * can have a period in it. Unlike GetMemberAsAptValue, calls APT_INC on the return value before
     * returning; call APT_DEC on *ppOut when done with it.
     */
    static void GetMember(AptValue *pValue, const char *szValue, AptValue **ppOut, int callStackLevel = 0);

    /** Returns a variable value specified by name as a member of "this". */
    static AptValue *GetThisMember(AptValue *context, const char *strName, int isFunctionScope, int level);

    /**
     * Gets member names of pValue, if it's an AptCIH or AptObject. Only names of members not
     * intrinsic to Apt (e.g. _x and _y are excluded). Allocates no memory: buf must be at least
     * maxChars, and on return holds the members separated by ':'.
     * @return the number of chars written to buf (excluding trailing null).
     */
    static int GetExtrinsicMemberNames(AptValue *pValue, char *buf, int maxChars, bool isSuper);

    /** Gets the value of an intrinsic member of an AptCIH (e.g., _x, _y, _alpha). */
    static bool GetIntrinsicMember(AptValue *pValue, const char *szValue, int *pOutInteger);
    static bool GetIntrinsicMember(AptValue *pValue, const char *szValue, float *pOutFloat);
    static bool GetIntrinsicMember(AptValue *pValue, const char *szValue, bool *pOutBool);
    static bool GetIntrinsicMember(AptValue *pValue, const char *szValue, char *pOutString, int nMaxChars);

    /** Gets the list of local variables' names for a specific function runtime context. */
    static int GetLocalNameList(AptValue *currentContext, char *buf, int maxChars);

    // These set a child member of pValue. Member name szValue can have a period in it.
    static void SetMember(AptValue *pValue, const char *szValue, int newVal);
    static void SetMember(AptValue *pValue, const char *szValue, float newVal);
    static void SetMember(AptValue *pValue, const char *szValue, bool newVal);
    static void SetMember(AptValue *pValue, const char *szValue, const char *newVal);
    static void SetMember(AptValue *pValue, const char *szValue, AptValue *newValue);
};
