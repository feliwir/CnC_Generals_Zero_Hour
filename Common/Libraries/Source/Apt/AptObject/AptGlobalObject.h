/**
 * Implements the ActionScript "_global" object.
 */

#pragma once

#include "AptValue/AptValue.h"
#include "_Apt.h"

#define APTGLOBAL_OBJECTHASHSIZE 11

// AptGlobal now derives from AptObject instead of AptValueGlobalWithHash
class AptGlobal : public AptObject
{
  public:
    APT_VALUE_GC_NEW_DELETE_OPERATORS

    AptGlobal() : AptObject(AptVFT_Global, APTGLOBAL_OBJECTHASHSIZE)
    {
        setGCRoot(1);
    }

    virtual AptValue *objectMemberLookup(AptValue *const pContext, const AptNativeString *const pName) const;
    virtual bool objectMemberSet(AptValue *const pContext, const AptNativeString *const pName, AptValue *const pValue);

    APT_INLINE AptValue *Lookup(const AptNativeString *const pKey) const;
    APT_INLINE void Set(const AptNativeString *const pKey, AptValue *const pValue);

    /** @brief Lookup a member by name and return its gperf key from objects.gperf */
    int GetGperfIndex(const AptNativeString *const name) const;

    /** @brief Lookup a member when we already know its gperf index */
    APT_INLINE AptValue *Lookup(int idx) const;

    // These indices match objects.gperf. Unfortunately, support for _global members
    // is spread all over the Apt codebase. Eventually we want them to be encapsulated
    // by the AptGlobal class.
    static const int setIntervalIdx   = 50;
    static const int clearIntervalIdx = 51;

  protected:
    APT_INLINE
    virtual ~AptGlobal()
    {
    }
};

