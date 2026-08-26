
#pragma once
#include "AptValue/AptValue.h"

class AptFloat : public AptValueNoGC
{
  protected:
    //  The user can't call new and delete, need to call Create() and Destroy()
    static void *operator new(size_t size);
    static void operator delete(void *p, size_t size);
    static void *operator new[](size_t size);
    static void operator delete[](void *p);

  public:

    /** @brief Return the value of this object as a float */
    APT_INLINE float GetFloat() const
    {
        return (mfValue);
    }

    /** @brief public form of the constructor will use the Float pool if enabled. */
    static AptFloat *Create(const float fValue);
    APT_ACCESS_INTERNAL :

        /** @brief Clear the pool if enabled. */
        static void ClearPool();

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

  protected:
    /** @brief hide the destructor */
    virtual ~AptFloat()
    {
    }

  private:
    union
    {
        float mfValue;
        AptFloat *mpNextFree;
    };

    AptFloat();

    APT_INLINE explicit AptFloat(const float fValue) : AptValueNoGC(AptVFT_Float),
                                                       mfValue(fValue)
    {
        //  Do nothing...
    }

    static AptFloat *spFirstFree;
};
