/**
 * A simple shared-pointer implementation. Although the class is generic, it is only intended to
 * be used with the AptFile and AptLinkerThingy data types.
 */

#pragma once
#include <stddef.h>
#include "AptDefine.h"

template <typename T>
int AptSharedPtrIncRef(T *p);
template <typename T>
int AptSharedPtrDecRef(T *p);
template <typename T>
void AptSharedPtrDelete(T *p);

// Ugliness for stupid compilers (this ought to be covered by the above templates)
#if defined(APT_DEBUG)
void AptSharedPtrSetFileSize(AptFile *p, int nSize);
int AptSharedPtrGetFileSize(AptFile *p);
#endif

struct AptFile;
int AptSharedPtrIncRef(AptFile *p);
void AptSharedPtrDispose(AptFile *p);

struct AptLinkerThingy;
int AptSharedPtrIncRef(AptLinkerThingy *p);
void AptSharedPtrDispose(AptLinkerThingy *p);

template <class T>
class AptSharedPtr
{
  public:
    explicit AptSharedPtr(T *p = 0) : pData(p)
    {
#if defined(REFCOUNT_DEBUGGING)
        if (pData)
            AptSharedPtrIncRef(pData, this);
#else
        if (pData)
            AptSharedPtrIncRef(pData);
#endif
    }

    AptSharedPtr(const AptSharedPtr &copy) : pData(copy.pData)
    {
#if defined(REFCOUNT_DEBUGGING)
        if (pData)
            AptSharedPtrIncRef(pData, this);
#else
        if (pData)
            AptSharedPtrIncRef(pData);
#endif
    }

    ~AptSharedPtr()
    {
        T *tempData = pData;
        pData       = NULL;
        AptSharedPtrDispose(tempData);
    }
    AptSharedPtr &operator=(const AptSharedPtr &r)
    {
        if (&r == this)
            return *this;

        T *tempData = pData;

        pData = r.pData;
#if defined(REFCOUNT_DEBUGGING)
        if (pData)
            AptSharedPtrIncRef(pData, this);
#else
        if (pData)
            AptSharedPtrIncRef(pData);
#endif

        AptSharedPtrDispose(tempData);
        return *this;
    }

    void Reset()
    {
        T *tempData = pData;
        pData       = 0;
        AptSharedPtrDispose(tempData);
    }

    T &operator*() const { return *pData; }
    T *operator->() const { return pData; }
    T *Get() const { return pData; }

    operator bool() const { return pData != 0; }
    bool operator!() const { return pData == 0; }

    T *pData;

#if defined(APT_DEBUG)
    void SetFileSize(int nSize)
    {
        AptSharedPtrSetFileSize(pData, nSize);
    }
    int GetFileSize()
    {
        return AptSharedPtrGetFileSize(pData);
    }
#endif
    APT_NEW_DELETE_OPERATORS
};

template <typename T, typename U>
inline bool operator==(const AptSharedPtr<T> &a, const AptSharedPtr<U> &b) { return a.Get() == b.Get(); }

template <typename T>
inline bool operator==(const AptSharedPtr<T> &a, const intptr_t b) { return a.Get() == (T *)b; }

template <typename T, typename U>
inline bool operator!=(const AptSharedPtr<T> &a, const AptSharedPtr<U> &b) { return a.Get() != b.Get(); }

template <typename T>
inline bool operator!=(const AptSharedPtr<T> &a, const intptr_t b) { return a.Get() != (T *)b; }
