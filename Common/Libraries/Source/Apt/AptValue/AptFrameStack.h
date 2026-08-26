/**
 * A wrapper around AptValueWithHash used mainly for scoping variables. Provides functions with
 * better names plus a couple of new ones for chaining (i.e. it has a parent scope to chain up to).
 * Designed to work the way Flash does, not necessarily generically.
 */

#pragma once
#include "AptDefine.h"
// #include "AptValue/AptValue.h"
#include "_AptValue.h"
#include "AptNativeHash.h"
#include "AptGC.h"

#define FRAME_STACK_DEFAULT_HASH_SIZE 4

class AptFrameStack : public AptValueWithHash
{
  public:
    APT_VALUE_GC_NEW_DELETE_OPERATORS
    // Added Metric defines

    APT_INLINE
    explicit AptFrameStack(AptFrameStack *pParentScope) : AptValueWithHash(AptVFT_FrameStack, FRAME_STACK_DEFAULT_HASH_SIZE),
                                                          mpParentScope(pParentScope)
    {
        APT_INCSAFE(mpParentScope);
    }

    APT_INLINE
    explicit AptFrameStack(AptFrameStack *pParentScope, int nHashSize) : AptValueWithHash(AptVFT_FrameStack, nHashSize),
                                                                         mpParentScope(pParentScope)
    {
        APT_INCSAFE(mpParentScope);
    }

    APT_INLINE
    void Set(const AptNativeString *const pKey, AptValue *const pValue)
    {
        mNativeHash.Set(pKey, pValue);
    }

    APT_INLINE
    AptValue *Lookup(const AptNativeString *const pKey)
    {
        return (mNativeHash.Lookup(pKey));
    }

    APT_INLINE
    void ClearScope()
    {
        mNativeHash.ClearDataNoDelete();
    }

    APT_INLINE
    void GetHashSize()
    {
        mNativeHash.GetHashSize();
    }

    APT_INLINE
    bool ExistsInLocalScope(AptNativeString *pVarName)
    {
        return mNativeHash.Lookup(pVarName) != NULL;
    }

    APT_INLINE
    void SetInLocalScope(AptNativeString *pVarName, AptValue *pValue)
    {
        mNativeHash.Set(pVarName, pValue);
    }

    bool SetWhereExistsInScopeChain(AptNativeString *pVarName, AptValue *pValue)
    {
        AptFrameStack *pF = this;
        while (pF)
        {
            if (pF->mNativeHash.Lookup(pVarName) != NULL)
            {
                pF->mNativeHash.Set(pVarName, pValue);
                return true;
            }
            pF = pF->mpParentScope;
        }

        return false;
    }

    AptValue *GetInScopeChain(AptNativeString *pVarName)
    {
        AptFrameStack *pF = this;

        while (pF)
        {
            AptValue *pValue = pF->mNativeHash.Lookup(pVarName);
            if (pValue != NULL)
            {
                return pValue;
            }
            pF = pF->mpParentScope;
        }

        return NULL;
    }

    virtual void DestroyGCPointers()
    {
        APT_DECSAFE(mpParentScope);
        AptValueWithHash::DestroyGCPointers();
    }

    virtual void RegisterReferences()
    {
        if (APT_REFERENCES_REGISTERED(this))
            return;

        APT_REGISTER_REFERENCE_SAFE(mpParentScope, "ParentScope", APT_REFREG_IS_APTVALUE);
        AptValueWithHash::RegisterReferences();
    }

  protected:
    virtual ~AptFrameStack()
    {
        //  Do nothing...
    }

    AptFrameStack *mpParentScope;
};
