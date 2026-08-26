
#pragma once
#include "AptDefine.h"
#include "AptValue/AptValue.h"

struct AptHashItem
{
    AptNativeString Key;
    AptValue *mValue;
};

class AptNativeHash
{
  public:
    AptNativeHash(const int32_t nTotalSize);
    ~AptNativeHash();

    void Set(const AptNativeString *const pKey, AptValue *const pValue);
    AptValue *Lookup(const AptNativeString *const pKey) const;
    void ClearDataNoDelete(); // Added for possible Frame stack optimizations, but could be used elsewhere.

    // Added for use with frame stack optimizations, but could be used elsewhere.
    // Returns the current size of the Hash Table, This is useful for predicting how
    // much a task will need and reserving that much in advance the next time.
    int GetHashSize()
    {
        return mnTotalSize;
    }
    APT_ACCESS_INTERNAL :

        void
        DestroyGCPointers(); // This is not virtual because AptNativeHash is not designed to be overridden.
#if defined(APT_XML_MEMORY_DUMP_SUPPORTED)
    void RegisterReferences(const AptValue *pFromRef, char *pFromRefC); // This is not virtual because AptNativeHash is not designed to be overridden.
#endif
    void RegisterReferences(const AptValue *pFromRef); // This is not virtual because AptNativeHash is not designed to be overridden.

    void Unset(const AptNativeString *const pKey);
    void SetIfNotExists(const AptNativeString *const pKey, AptValue *const pValue);
    void ClearData();

    AptHashItem *GetFirstItem();
    AptHashItem *GetNextItem(AptHashItem *pItem);

    void UpdateObjectMethods(AptValue *pContext, const AptNativeString *pVar, int bRemove);

    APT_INLINE AptValue *Get__Proto__() const
    {
        return mp__proto__;
    }

    void Set__Proto__(AptValue *const pValue);
    void Unset__Proto__();

    APT_INLINE AptValue *GetPrototype() const
    {
        return mpPrototype;
    }

    void SetPrototype(AptValue *const pValue);
    void UnsetPrototype();

    APT_INLINE bool IsEmpty() const
    {
        return (mpData == NULL);
    }

    APT_INLINE void SetEventHandler(int nFlag)
    {
        nEventHandlers |= nFlag;
    }

    APT_INLINE void RemoveEventHandler(int nFlag)
    {
        nEventHandlers &= ~nFlag;
    }

    APT_INLINE int HasEventHandler(int nFlag)
    {
        return (static_cast<int>(nEventHandlers) & nFlag);
    }

  private:
    void FirstAllocation();

    void Expand();

#if APT_DEBUG_LEVEL >= APT_DEBUG_MAXIMUM
    bool CheckConsistency() const;
    AptHashItem *SlowFindKey(const AptNativeString *const pKey) const;
#endif

    void HashSet(const AptNativeString *const pKey, AptValue *const pValue);

    AptHashItem *HashFindKey(const AptNativeString *const pKey) const;

    const static int32_t NUM_PROBES = 8;

    APT_INLINE
    AptValue *GetAt(const int32_t nIndex) const
    {
        return mpData[nIndex].mValue;
    }

    static APT_INLINE AptValue *GetAt(AptHashItem *const pItem)
    {
        return pItem->mValue;
    }

    void SetAt(const int32_t nIndex, AptValue *const pValue);
    static void SetAt(AptHashItem *const pItem, AptValue *const pValue);
    void OverwriteAt(const int32_t nIndex, AptValue *pValue);
    static void OverwriteAt(AptHashItem *const pHashItem, AptValue *pValue);
    void UnsetAt(AptHashItem *const pHashItem);

    APT_NEW_DELETE_OPERATORS
    // Added Metric defines

    int32_t mnTotalSize;
    AptHashItem *mpData;
    AptValue *mp__proto__;
    AptValue *mpPrototype;
    int32_t nEventHandlers{0};
};
