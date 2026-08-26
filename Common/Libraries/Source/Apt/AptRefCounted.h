#pragma once

#include "AptDefine.h"

template <typename T>
class AptRefCounted : public T
{
    mutable int mnRefCount{1};

  public:
    inline AptRefCounted() {}

    void AddRef() const
    {
        mnRefCount++;
    }
    void Release() const
    {
        if (!--mnRefCount)
        {
            AptRefCounted<T> *pTemp = const_cast<AptRefCounted<T> *>(this);
            delete pTemp;
        }
    }
};

template <typename T>
inline T *SafeAddRef(T *p)
{
    if (p)
        p->AddRef();
    return p;
}
template <typename T>
inline T *SafeRelease(T *p)
{
    if (p)
        p->Release();
    return 0;
}

