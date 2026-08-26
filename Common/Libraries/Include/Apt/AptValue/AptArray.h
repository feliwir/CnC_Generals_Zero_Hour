
#pragma once
#include "AptObject/AptObject.h"

class AptArray : public AptObject
{
  public:
    using fnFindComparator_t = int (*)(AptValue *value, AptValue *equals, const AptNativeString *equalsStr, const AptNativeString *property);

    void set(int nIndex, AptValue *pValue);
    AptValue *get(int nIndex);
    void toString(char *szBuf, const char *szSeparator = ",");
    int find(fnFindComparator_t comparator, int start, AptValue *equals, AptValue *property, bool reverse = false);
    APT_INLINE int length()
    {
        return mnLength;
    }

    static int objectFindComparator(AptValue *value, AptValue *equals, const AptNativeString *equalsStr, const AptNativeString *property);

    // the contructor and new/delete operators should not really be public, but legacy client code requires access
    static void *operator new(size_t size);
    void *operator new(size_t t, void *p);
    static void operator delete(void *p, size_t size);
    AptArray();

    APT_ACCESS_INTERNAL :

                          AptArray(int nElements, AptValue **pAptValue);

    static void CleanNativeFunctions();

    virtual AptValue *objectMemberLookup(AptValue *const pContext, const AptNativeString *const pName) const;
    virtual bool objectMemberSet(AptValue *const pContext, const AptNativeString *const pName, AptValue *const pValue);
    void toString(AptNativeString &sBuf, const char *szSeparator = ",");

    AptValue *GetAt(const int32_t nIndex) const;
    void SetAt(const int32_t nIndex, AptValue *const pNewValue);

    NATIVE_MEMBER_FUNCTION_DECL(concat)
    NATIVE_MEMBER_FUNCTION_DECL(join)
    NATIVE_MEMBER_FUNCTION_DECL(pop)
    NATIVE_MEMBER_FUNCTION_DECL(push)
    NATIVE_MEMBER_FUNCTION_DECL(shift)
    NATIVE_MEMBER_FUNCTION_DECL(unshift)
    NATIVE_MEMBER_FUNCTION_DECL(reverse)
    NATIVE_MEMBER_FUNCTION_DECL(sort)
    NATIVE_MEMBER_FUNCTION_DECL(splice)
    NATIVE_MEMBER_FUNCTION_DECL(slice)
    NATIVE_MEMBER_FUNCTION_DECL(sortOn)

  protected:
    virtual ~AptArray();
    virtual void RegisterReferences();
    virtual void DestroyGCPointers();

  private:
    void _reserve(int nSize);
    AptValue **mpValues;
    int mnCapacity; // Num Items Allocated In the Array.
    int mnLength;   // Num Items In the Array.

    static int defaultSortCompareFunc(const void *a, const void *b);
    static int defaultSortOnCompareFunc(const void *a, const void *b);
    static int scriptFunctionSortFunc(const void *a, const void *b);

  private:
    // Do not call. Garbage collected objects cannot be created as C++ arrays.
    static void *operator new[](size_t size);
    static void operator delete[](void *p);
};
