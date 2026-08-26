
#pragma once
#include "AptValue/AptValue.h"

class AptString : public AptValueNoGC
{
  protected:
    // Put these in protected Scope to force Create() and Delete() usage.
    static void *operator new(size_t size);
    static void operator delete(void *p, size_t size);
    static void *operator new[](size_t size);
    static void operator delete[](void *p);

  public:

    // Cannot inline these here as they need to have seen eastring.inl, thus the definitions are in aptstring.inl
    APT_INLINE void cpy(const AptNativeString *pStr)
    {
        str = *pStr;
    }

    APT_INLINE void cpy(const char *pStr)
    {
        str = pStr;
    }

    APT_INLINE void cat(const AptNativeString *pStr)
    {
        str += *pStr;
    }

    APT_INLINE void cat(const char *pStr)
    {
        str += pStr;
    }

    void printf(const char *szFormat, ...);

    APT_INLINE AptNativeString *GetInternalString()
    {
        return (&str);
    }

    /** @brief Create an empty string */
    APT_INLINE static AptString *Create()
    {
        return Create("");
    }

    /** @brief Create a string based on the passed value */
    static AptString *Create(const char *szValue);

    APT_ACCESS_INTERNAL :

        /** @brief Override from AptValue */
        virtual void DeleteThis()
    {
        Destroy();
    }

    /** @brief Override from AptValue */
    virtual void ForceDelete()
    {
        Destroy();
    }

    /** @brief Either deletes the object or returns it to the pool, depending on how we are configured. */
    void Destroy();

    virtual AptValue *objectMemberLookup(AptValue *const pContext, const AptNativeString *const pName) const;

    static void CleanNativeFunctions();

    NATIVE_MEMBER_FUNCTION_DECL(charAt)
    NATIVE_MEMBER_FUNCTION_DECL(charCodeAt)
    NATIVE_MEMBER_FUNCTION_DECL(concat)
    NATIVE_MEMBER_FUNCTION_DECL(fromCharCode)
    NATIVE_MEMBER_FUNCTION_DECL(indexOf)
    NATIVE_MEMBER_FUNCTION_DECL(lastIndexOf)
    NATIVE_MEMBER_FUNCTION_DECL(slice)
    NATIVE_MEMBER_FUNCTION_DECL(split)
    NATIVE_MEMBER_FUNCTION_DECL(substr)
    NATIVE_MEMBER_FUNCTION_DECL(substring)
    NATIVE_MEMBER_FUNCTION_DECL(toLowerCase)
    NATIVE_MEMBER_FUNCTION_DECL(toUpperCase)

    /**
     * @brief This really does not belong in public scope. Please use "GetInternalString" instead!
     * This wasn't removed for now because it saves on an inc and dec when used because
     * GetInternalString returns a pointer, Doh! ...
     */
    AptNativeString str;

  protected:
    void SetNext(AptString *const pNext)
    {
        mpNext = pNext;
    }

    AptString *GetNext() const
    {
        return (mpNext);
    }

    /** @brief Hide the destructor */
    virtual ~AptString()
    {
    }

  private:
    AptString() : AptValueNoGC(AptVFT_StringValue),
                  str(),
                  mpNext(NULL)
    {
        //  Do nothing...
    }

    AptString(const char *szValue) : AptValueNoGC(AptVFT_StringValue),
                                     str(szValue),
                                     mpNext(NULL)
    {
    }

    APT_INLINE bool IsConstantString();

    AptString *mpNext;

    enum
    {
        MAX_SIZE_KEPT_TEMP_STRING = 32,
    };

    friend class StringPool;
};
