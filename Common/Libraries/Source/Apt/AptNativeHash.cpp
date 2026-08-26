/**
 * Manages the AptNativeHash: an open hash-table keyed by case-insensitive strings, storing
 * AptValue pointers. On collision the data is stored in a neighboring cell rather than expanding
 * the table. "prototype" and "__proto__" are special-cased and stored directly as pointers.
 */

#define hashmask(n) ((n) - 1)

#include "_Apt.h"
#include "_AptActions.h"
#include "AptSpriteMembersConsts.h"
#include "MainInline.h"

static const unsigned int HASH_VALUE_PROTOTYPE = 0x0699;
static const unsigned int HASH_VALUE_PROTO     = 0x6BBD;


AptNativeHash::AptNativeHash(const int32_t nTotalSize)
    : mnTotalSize(nTotalSize),
      mpData(NULL),
      mp__proto__(NULL),
      mpPrototype(NULL)

{
    // If mnTotalSize is not a power of two, make it one (or bad things will happen).
    if ((mnTotalSize & (mnTotalSize - 1)) != 0)
    {
        // Take mnTotalSize up to the next higher power of two.
        int NextHighestPowerOfTwo = 1;
        while (NextHighestPowerOfTwo < mnTotalSize)
        {
            NextHighestPowerOfTwo <<= 1;
        }
        mnTotalSize = NextHighestPowerOfTwo;
    }

    //  Check some constants
#if defined(APT_DEBUG)
    const uint32_t LENGTH_PROTOTYPE = 9;
    const uint32_t LENGTH_PROTO     = 9;
    APT_ASSERT(StringPool::GetString(SC___proto__)->GetLength() == LENGTH_PROTOTYPE);
    APT_ASSERT(StringPool::GetString(SC_prototype)->GetLength() == LENGTH_PROTO);
    APT_ASSERT(LENGTH_PROTOTYPE == LENGTH_PROTO);
#endif // APT_DEBUG
    APT_ASSERT(StringPool::GetString(SC___proto__)->UpdateHashValue() == HASH_VALUE_PROTO);
    APT_ASSERT(StringPool::GetString(SC_prototype)->UpdateHashValue() == HASH_VALUE_PROTOTYPE);
    APT_ASSERT((mnTotalSize & (mnTotalSize - 1)) == 0 && "Total Size was not made a power of Two!!!");
}

AptNativeHash::~AptNativeHash()
{
    if (mpData)
    {
        APT_ASSERT(false && "Apt GC Pointers were not destroyed before deletion. ");
        DestroyGCPointers();
    }
    mpData      = NULL;
    mp__proto__ = NULL;
    mpPrototype = NULL;
}

/** Breaks the links to garbage-collected objects. After this call there must be no references left. */
void AptNativeHash::DestroyGCPointers()
{
    UnsetPrototype();
    Unset__Proto__();

    if (IsEmpty())
    {
        return;
    }

    nEventHandlers = 0;

    for (int i = 0; i < mnTotalSize; i++)
    {
        if (mpData[i].mValue)
        {
            APT_DEC(mpData[i].mValue);
            mpData[i].mValue = NULL;
        }
        // considering that key is not garbage collected.
        if (mpData[i].Key.IsValid())
        {
            mpData[i].Key.Invalidate();
        }
    }

    // No need to hold on to this anymore, just give the memory away.
    APT_FREE_ARRAY_TRACKER(AptHashBlocks, mpData, AptHashItem, mnTotalSize);
    mpData = NULL;
}

/** Sets a new AptValue at @p nIndex, taking care of reference counting. */
void AptNativeHash::SetAt(const int32_t nIndex, AptValue *const pValue)
{
    APT_ASSERT(pValue != NULL);
    APT_ASSERT(mpData != NULL);
    AptValue *pOldValue = mpData[nIndex].mValue;
    APT_INC(pValue);
    APT_DECSAFE(pOldValue);
    mpData[nIndex].mValue = pValue;
}

/** Sets a new AptValue at @p pItem, taking care of reference counting. */
void AptNativeHash::SetAt(AptHashItem *const pItem, AptValue *const pValue)
{
    APT_ASSERT(pValue != NULL);
    AptValue *pOldValue = pItem->mValue;
    APT_INC(pValue);
    APT_DECSAFE(pOldValue);
    pItem->mValue = pValue;
}

/** Sets a new AptValue at @p nIndex, taking care of reference counting (no previous value expected). */
void AptNativeHash::OverwriteAt(const int32_t nIndex, AptValue *pValue)
{
    APT_ASSERT(pValue != NULL);
    APT_ASSERT(mpData != NULL);
    APT_ASSERT(nIndex < mnTotalSize && nIndex >= 0);

    APT_INC(pValue);
    mpData[nIndex].mValue = pValue;
}

/** Sets a new AptValue at @p pHashItem, taking care of reference counting (no previous value expected). */
void AptNativeHash::OverwriteAt(AptHashItem *const pHashItem, AptValue *pValue)
{
    APT_ASSERT(pValue != NULL);
    APT_INC(pValue);
    pHashItem->mValue = pValue;
}

/** Clears the AptValue at @p pHashItem, taking care of reference counting. */
void AptNativeHash::UnsetAt(AptHashItem *const pHashItem)
{
    APT_ASSERT(pHashItem != NULL);
    APT_ASSERT(pHashItem->mValue != NULL);
    APT_DEC(pHashItem->mValue);
    pHashItem->mValue = NULL;
}

/**
 * Sets @p pValue for @p pKey only if the key does not already exist. This will expand the hash if
 * needed.
 */
void AptNativeHash::SetIfNotExists(const AptNativeString *const pKey, AptValue *const pValue)
{
    // this can be probably be optimized to do everything in one pass
    if (Lookup(pKey) == NULL)
    {
        Set(pKey, pValue);
    }
}

/** Sets the ActionScript __proto__ member. */
void AptNativeHash::Set__Proto__(AptValue *const pValue)
{
    APT_INCSAFE(pValue);
    APT_DECSAFE(mp__proto__);
    mp__proto__ = pValue;
}

/** Clears the ActionScript __proto__ member. */
void AptNativeHash::Unset__Proto__()
{
    if (mp__proto__)
    {
        APT_DEC(mp__proto__);
        mp__proto__ = NULL;
    }
}

/** Sets the ActionScript prototype member. */
void AptNativeHash::SetPrototype(AptValue *const pValue)
{
    APT_INCSAFE(pValue);
    APT_DECSAFE(mpPrototype);
    mpPrototype = pValue;
}

/** Clears the ActionScript prototype member. */
void AptNativeHash::UnsetPrototype()
{
    if (mpPrototype)
    {
        APT_DEC(mpPrototype);
        mpPrototype = NULL;
    }
}

/** Sets @p pValue for @p pKey in the table. This will expand the hash if needed. */
void AptNativeHash::Set(const AptNativeString *const pKey, AptValue *const pValue)
{
    if (pValue == NULL)
    {
        Unset(pKey);
        return;
    }

    if (pKey->IsEmpty())
    {
        return;
    }

    uint32_t h = pKey->UpdateHashValue();

    if ((HASH_VALUE_PROTOTYPE == h) && (pKey->EqualNoCase(*StringPool::GetString(SC_prototype))))
    {
        SetPrototype(pValue);
        return;
    }
    else if ((HASH_VALUE_PROTO == h) && (pKey->EqualNoCase(*StringPool::GetString(SC___proto__))))
    {
        Set__Proto__(pValue);
        return;
    }

    if (IsEmpty())
    {
        //  hash empty, allocate before add a new key / value
        FirstAllocation();
    }

    HashSet(pKey, pValue);
}

/** Removes @p pKey and its value if found, as if the item never existed. */
void AptNativeHash::Unset(const AptNativeString *const pKey)
{
    if (pKey->IsEmpty())
    {
        return;
    }

    uint32_t h = pKey->UpdateHashValue();

    if (IsEmpty() == false)
    {
        AptHashItem *pHashItem;

        pHashItem = HashFindKey(pKey);
        if (pHashItem != NULL)
        {
            //  We unset a key / value
            //  We clear the key (it's still valid) So we know that a key was here
            //  This is actually very important because the search function stops searching at an invalid
            //  string! So if we invalidate here the search will stop early... Thankfully we will insert into
            //  locations with a cleared string, so nothing is really lost.
            pHashItem->Key.Clear();
            UnsetAt(pHashItem);
            return;
        }
    }

    if ((HASH_VALUE_PROTOTYPE == h) && (pKey->EqualNoCase(*StringPool::GetString(SC_prototype))))
    {
        UnsetPrototype();
    }
    else if ((HASH_VALUE_PROTO == h) && (pKey->EqualNoCase(*StringPool::GetString(SC___proto__))))
    {
        Unset__Proto__();
    }
}

/**
 * Gets the value associated with @p pKey.
 * @return the value found, or NULL if there is no corresponding key.
 */
AptValue *AptNativeHash::Lookup(const AptNativeString *const pKey) const
{
    APT_ASSERT(pKey);

    AptHashItem *pItem;

    // Calculate hash of key for the search
    uint32_t h = pKey->UpdateHashValue();

    if (IsEmpty() == false)
    {
        pItem = HashFindKey(pKey);
        if (pItem != NULL)
        {
            return pItem->mValue;
        }
    }

    if ((HASH_VALUE_PROTOTYPE == h) && (pKey->EqualNoCase(*StringPool::GetString(SC_prototype))))
    {
        return mpPrototype;
    }
    else if ((HASH_VALUE_PROTO == h) && (pKey->EqualNoCase(*StringPool::GetString(SC___proto__))))
    {
        return mp__proto__;
    }
    return (NULL);
}

/** Erases the content of the entire hash-table. */
void AptNativeHash::ClearData()
{
    if (mpPrototype != NULL)
    {
        APT_DEC(mpPrototype);
        mpPrototype = NULL;
    }
    if (mp__proto__ != NULL)
    {
        APT_DEC(mp__proto__);
        mp__proto__ = NULL;
    }

    if (mpData != NULL)
    {
        for (int i = 0; i < mnTotalSize; i++)
        {
            if (mpData[i].Key.IsValid())
            {
                if (mpData[i].mValue)
                {
                    APT_DEC(mpData[i].mValue);
                    mpData[i].mValue = NULL;
                }
                // considering that key is not garbage collected.
                mpData[i].Key.Invalidate();
            }
        }
        APT_FREE_ARRAY_TRACKER(AptHashBlocks, mpData, AptHashItem, mnTotalSize);
        mpData = NULL;
    }
    nEventHandlers = 0;
}

/** Erases the content of the entire hash-table, without releasing the underlying memory. */
void AptNativeHash::ClearDataNoDelete()
{
    if (mpPrototype != NULL)
    {
        APT_DEC(mpPrototype);
        mpPrototype = NULL;
    }
    if (mp__proto__ != NULL)
    {
        APT_DEC(mp__proto__);
        mp__proto__ = NULL;
    }

    if (mpData != NULL)
    {
        for (int i = 0; i < mnTotalSize; i++)
        {
            if (mpData[i].Key.IsValid())
            {
                if (mpData[i].mValue)
                {
                    APT_DEC(mpData[i].mValue);
                    mpData[i].mValue = NULL;
                }
                // considering that key is not garbage collected.
                mpData[i].Key.Invalidate();
            }
        }
    }
    nEventHandlers = 0;
}

/**
 * Returns the first item in the hash, or NULL if empty. Used to iterate in "for each" situations:
 * call GetFirstItem then loop on GetNextItem until NULL is returned.
 * @note There is no guarantee of order, or that it stays the same across calls -- only that every
 * item will eventually be returned before NULL.
 */
AptHashItem *AptNativeHash::GetFirstItem()
{
    if (IsEmpty())
        return NULL;
    for (int i = 0; i < mnTotalSize; ++i)
    {
        APT_ASSERT(mpData);
        if (mpData[i].Key.IsValid() && (mpData[i].Key.IsEmpty() == false))
            return &mpData[i];
    }
    return NULL;
}

/** Returns the item after @p pItem in the hash table. See GetFirstItem() for more info. */
AptHashItem *AptNativeHash::GetNextItem(AptHashItem *pItem)
{
    if (IsEmpty())
        return NULL;

    APT_ASSERT(mpData);
    APT_ASSERT((pItem >= &mpData[0]) && (pItem < &mpData[mnTotalSize]));

    while (++pItem < &mpData[mnTotalSize])
    {
        if (pItem->Key.IsValid() && (pItem->Key.IsEmpty() == false))
            return pItem;
    }
    return NULL;
}

/** Allocates the buffer of the hash-table. Usually done just before the first value is set. */
void AptNativeHash::FirstAllocation()
{
    APT_ASSERT(IsEmpty());
    mpData = APT_MALLOC_ARRAY_TRACKER(AptHashBlocks, AptHashItem, mnTotalSize);
    APT_ASSERT(mpData != NULL);
    memset(mpData, 0, mnTotalSize * sizeof(AptHashItem));
}

/** Expands the hash-table, usually after several collisions. */
void AptNativeHash::Expand()
{
#if (APT_DEBUG_LEVEL >= APT_DEBUG_MAXIMUM)
    APT_ASSERT(CheckConsistency()); // Very time consuming, only do on maximum testing.
#endif

    //  First create a bigger AptNativeHash
    AptNativeHash TempHash(mnTotalSize * 2);
    TempHash.FirstAllocation(); //  We will add items, don't forget to allocate ;)
    AptNativeString *pKey;
    int32_t i;

    //  Insert the elements in the larger hash-table
    //  There should not be a lot of collision
    //  Even if this happen, there will be another recursive expansion
    for (i = 0; i < mnTotalSize; ++i)
    {
        APT_ASSERT(mpData);
        pKey = &mpData[i].Key;
        if (pKey->IsValid() == false)
        {
            //  No key to add
            continue;
        }
        if (pKey->IsEmpty())
        {
            //  There is no interest to copy erased key state as we start a new hash
            continue;
        }

        AptValue *pValue = mpData[i].mValue;
        TempHash.HashSet(pKey, pValue);
    }

    //  We swap the buffer and the size
    AptHashItem *pOldItems = mpData;
    mpData                 = TempHash.mpData;
    TempHash.mpData        = pOldItems;
    int32_t iTemp;                               // It's possible that we grow several times instead only one
    iTemp                = TempHash.mnTotalSize; //  So we take the TempHash size to be sure that we have the right size
    TempHash.mnTotalSize = mnTotalSize;
    mnTotalSize          = iTemp;

#if (APT_DEBUG_LEVEL >= APT_DEBUG_MAXIMUM)
    APT_ASSERT(CheckConsistency()); // Very time consuming, only do on maximum testing.
#endif

    //  The temporary hash is destroyed at the end of the method, so the old buffer is released
    TempHash.DestroyGCPointers();
}

#if APT_DEBUG_LEVEL >= APT_DEBUG_MAXIMUM
/**
 * Checks that the hash-table is consistent with its content.
 * @note Slow -- for debugging purposes only.
 */
bool AptNativeHash::CheckConsistency() const
{
    if (IsEmpty())
    {
        //  Empty nothing to scan
        return (true);
    }

    int32_t i, j;
    AptHashItem *pItem;
    AptHashItem *pItem2;
    //  Check that __proto__ and prototype don't exist in the table
    if ((SlowFindKey(StringPool::GetString(SC___proto__)) != NULL) ||
        (SlowFindKey(StringPool::GetString(SC_prototype)) != NULL))
    {
        return (false);
    }

    //  Third, we check that there is only one entry of each key
    for (i = 0; i < mnTotalSize; ++i)
    {
        pItem = &mpData[i];
        if (pItem->Key.IsValid() == false)
        {
            continue;
        }
        if (pItem->Key.IsEmpty())
        {
            continue;
        }

        if (Lookup(&pItem->Key) == NULL)
        {
            //  We didn't found the key!
            return (false);
        }

        for (j = i + 1; j < mnTotalSize; ++j)
        {
            pItem2 = &mpData[j];
            if (pItem2->Key.IsValid() == false)
            {
                //  Last key for a vector
                break;
            }
            if (pItem2->Key.IsEmpty())
            {
                continue;
            }
            if (pItem2->Key.EqualNoCaseHash(pItem->Key))
            {
                //  Found the same key
                return (false);
            }
        }
    }

    //  We could add more test like if the h probing is correct and so on

    //  No error found
    return (true);
}
#endif

/** Sets @p pKey / @p pValue in the internal hash-table. */
void AptNativeHash::HashSet(const AptNativeString *const pKey, AptValue *const pValue)
{
    APT_ASSERT(pKey);
    APT_ASSERT(mpData);
#if (APT_DEBUG_LEVEL >= APT_DEBUG_MAXIMUM)
    APT_ASSERT(CheckConsistency()); // Very time consuming, only do on maximum testing.
#endif

    int nBoundMin, nBoundMax;
    int nFirstFit = -1;
    uint32_t h    = pKey->GetHashValue() & hashmask(mnTotalSize);

    //  First we search for the best hash-value
    if (mpData[h].Key.IsValid() == false)
    {
        //  First time we add a key here, we can add it without testing everything else
        mpData[h].Key.Validate(*pKey); //  Validate the string with the new copy
        OverwriteAt(h, pValue);
        return;
    }

    if (mpData[h].Key.IsEmpty())
    {
        //  A key was erased here, this place is the best for an insertion
        nFirstFit = h;
    }
    else
    {
        //  There is already a key, we need to check if it's the same key
        if (mpData[h].Key.EqualNoCaseHash(*pKey))
        {
            // Same key, replace key + value
            SetAt(h, pValue);
            return;
        }
    }

    //  We need now to find on the sides

    //  We use nSize to limit the bounds
    //  We limit the bound on the hash-table so we reduce the probability of cache-miss
    //  While giving a big range for the search
    nBoundMin = h - NUM_PROBES;
    if (nBoundMin < 0)
    {
        nBoundMin = 0;
        nBoundMax = 2 * NUM_PROBES;
        if (nBoundMax >= mnTotalSize)
        {
            nBoundMax = mnTotalSize - 1;
        }
    }
    else
    {
        nBoundMax = h + NUM_PROBES;
        if (nBoundMax > mnTotalSize - 1)
        {
            nBoundMax = mnTotalSize - 1;
            nBoundMin = nBoundMax - (2 * NUM_PROBES);
            if (nBoundMin < 0)
            {
                nBoundMin = 0;
            }
        }
    }
    APT_ASSERT(nBoundMin >= 0);
    APT_ASSERT(nBoundMax < mnTotalSize);
    APT_ASSERT(nBoundMin < nBoundMax);

    //  Now do the same research after
    int32_t i, nCurrentH;

    nCurrentH = h;
    i         = nBoundMax - h;
    while (i--)
    {
        ++nCurrentH;
        if (mpData[nCurrentH].Key.IsValid() == false)
        {
            //  Even if it's not the best place we can insert the key here
            mpData[nCurrentH].Key.Validate(*pKey); //  Validate the string with the new copy
            OverwriteAt(nCurrentH, pValue);
            return;
        }
        if (mpData[nCurrentH].Key.IsEmpty())
        {
            // want to set if it was not previously set (this causes
            // items to never back fill cleared spots in the hash otherwise).
            if (nFirstFit == -1)
            {
                //  Set the best fit, if not already set
                nFirstFit = nCurrentH;
            }
        }
        else
        {
            if (mpData[nCurrentH].Key.EqualNoCaseHash(*pKey))
            {
                // Same key, replace key + value
                SetAt(nCurrentH, pValue);
                return;
            }
        }
    }

    //  Still not, try before
    nCurrentH = h;
    i         = h - nBoundMin;
    while (i--)
    {
        --nCurrentH;
        if (mpData[nCurrentH].Key.IsValid() == false)
        {
            //  Even if it is not the best place we can insert the key here
            mpData[nCurrentH].Key.Validate(*pKey); //  Validate the string with the new copy
            OverwriteAt(nCurrentH, pValue);
            return;
        }
        if (mpData[nCurrentH].Key.IsEmpty())
        {
            // want to set if it was not previously set (this causes
            // items to never back fill cleared spots in the hash otherwise).
            if (nFirstFit == -1)
            {
                //  Set the best fit, if not already set
                nFirstFit = nCurrentH;
            }
        }
        else
        {
            if (mpData[nCurrentH].Key.EqualNoCaseHash(*pKey))
            {
                // Same key, replace key + value
                SetAt(nCurrentH, pValue);
                return;
            }
        }
    }

//  We need to insert the key as it is not there
#if (APT_DEBUG_LEVEL >= APT_DEBUG_MAXIMUM)
    APT_ASSERT(SlowFindKey(pKey) == NULL); // Very time consuming, only do on maximum testing.
#endif

    if (nFirstFit == -1)
    {
        //  We didn't find enough room for the insertion, expand it
        Expand();
        //  Call recursively this set until there is enough room
        HashSet(pKey, pValue);
        return;
    }

    //  We found a place to put it
    mpData[nFirstFit].Key = *pKey;
    OverwriteAt(nFirstFit, pValue);
}

/**
 * Finds a key/value in the internal hash-table.
 * @return the AptHashItem for @p pKey, or NULL if not found.
 */
AptHashItem *AptNativeHash::HashFindKey(const AptNativeString *const pKey) const
{
    APT_ASSERT(pKey != NULL);
    APT_ASSERT(mpData);

    int nBoundMin, nBoundMax;
    uint32_t h = pKey->GetHashValue() & hashmask(mnTotalSize);

    //  First we search for the best hash-value
    if (mpData[h].Key.IsValid() == false)
    {
        //  There was never a key here, therefore the key we are looking for is not here
        return (NULL);
    }

    if (mpData[h].Key.IsEmpty())
    {
        //  A key was erased here, we need to continue the search
    }
    else
    {
        //  There is already a key, we need to check if it's the same key
        if (mpData[h].Key.EqualNoCaseHash(*pKey))
        {
            //  Got it!
            return (&mpData[h]);
        }
    }

    //  We need now to find on the sides

    //  We use nSize to limit the bounds
    //  We limit the bound on the hash-table so we reduce the probability of cache-miss
    //  While given a big range for the search
    nBoundMin = h - NUM_PROBES;
    if (nBoundMin < 0)
    {
        nBoundMin = 0;
        nBoundMax = 2 * NUM_PROBES;
        if (nBoundMax >= mnTotalSize)
        {
            nBoundMax = mnTotalSize - 1;
        }
    }
    else
    {
        nBoundMax = h + NUM_PROBES;
        if (nBoundMax > mnTotalSize - 1)
        {
            nBoundMax = mnTotalSize - 1;
            nBoundMin = nBoundMax - (2 * NUM_PROBES);
            if (nBoundMin < 0)
            {
                nBoundMin = 0;
            }
        }
    }
    APT_ASSERT(nBoundMin >= 0);
    APT_ASSERT(nBoundMax < mnTotalSize);
    APT_ASSERT(nBoundMin < nBoundMax);

    //  Now do the same research after
    int32_t i, nCurrentH;

    nCurrentH = h;
    i         = nBoundMax - h;
    while (i--)
    {
        ++nCurrentH;
        if (mpData[nCurrentH].Key.IsValid() == false)
        {
            //  Never a key here, don't need to continue
            return (NULL);
        }
        if (mpData[nCurrentH].Key.IsEmpty())
        {
            //  Need to continue
        }
        else
        {
            if (mpData[nCurrentH].Key.EqualNoCaseHash(*pKey))
            {
                return (&mpData[nCurrentH]);
            }
        }
    }

    //  Still not, try before
    nCurrentH = h;
    i         = h - nBoundMin;
    while (i--)
    {
        --nCurrentH;
        if (mpData[nCurrentH].Key.IsValid() == false)
        {
            //  Never a key here, don't need to continue
            return (NULL);
        }
        if (mpData[nCurrentH].Key.IsEmpty())
        {
            //  Need to continue
        }
        else
        {
            if (mpData[nCurrentH].Key.EqualNoCaseHash(*pKey))
            {
                return (&mpData[nCurrentH]);
            }
        }
    }

    return (NULL);
}

#if APT_DEBUG_LEVEL >= APT_DEBUG_MAXIMUM
/**
 * Searches for a key without using the hash value.
 * @note Slow -- for debugging purposes only.
 * @return the AptHashItem pointer, or NULL if not found.
 */
AptHashItem *AptNativeHash::SlowFindKey(const AptNativeString *const pKey) const
{
    int32_t i;
    AptHashItem *pItem;
    for (i = 0; i < mnTotalSize; ++i)
    {
        pItem = &mpData[i];
        if (pItem->Key.IsValid() == false)
        {
            continue;
        }
        if (pItem->Key.IsEmpty())
        {
            continue;
        }
        if (pItem->Key.EqualNoCaseHash(*pKey))
        {
            //  We found the item
            return (pItem);
        }
    }
    return (NULL);
}
#endif

/** Updates the event handler flags for @p pContext when @p pVar is added or removed (@p bRemove). */
void AptNativeHash::UpdateObjectMethods(AptValue *pContext, const AptNativeString *pVar, int bRemove)
{
    if (pContext->isCIH())
        return;

    SpriteMembers *pProp = SpriteMembersIndex::in_word_set(pVar->c_str(), pVar->Size());
    if (pProp)
    {
        if (pProp->nIndex < 200)
            return;
        int nTableIndex = pProp->nIndex - AptSpriteMethod_onData;
        APT_ASSERT(nTableIndex >= 0 && nTableIndex < APT_ARRAYSIZE(aSpriteGperfToActionFlag));
        int nFlag = aSpriteGperfToActionFlag[nTableIndex];
        if (nFlag == -1)
        {
            return;
        }
        if (bRemove)
        {
            RemoveEventHandler(nFlag);
        }
        else
        {
            SetEventHandler(nFlag);
        }
    }
}

#if defined(APT_XML_MEMORY_DUMP_SUPPORTED)
/** Registers all references for XML memory dumps, using @p pFromRefC as the base for key names. */
void AptNativeHash::RegisterReferences(const AptValue *pFromRef, char *pFromRefC)
{
    APT_REGISTER_REFERENCE_FROM_SAFE(pFromRef, mp__proto__, *StringPool::GetString(SC___proto__), APT_REFREG_IS_APTVALUE);
    APT_REGISTER_REFERENCE_FROM_SAFE(pFromRef, mpPrototype, *StringPool::GetString(SC_prototype), APT_REFREG_IS_APTVALUE);

    if (IsEmpty())
    {
        return;
    }

    for (int i = 0; i < mnTotalSize; i++)
    {
        APT_ASSERT(mpData);
        if (mpData[i].mValue != NULL)
        {
            AptNativeString sTemp = pFromRefC + mpData[i].Key;
            APT_REGISTER_REFERENCE_FROM_SAFE(pFromRef, mpData[i].mValue, sTemp.c_str(), APT_REFREG_IS_APTVALUE);
        }
    }

    return;
}
#endif

/** Registers all references for XML memory dumps using the APT_REGISTER_REFERENCE_FROM_SAFE macro. */
void AptNativeHash::RegisterReferences(const AptValue *pFromRef)
{
    APT_REGISTER_REFERENCE_FROM_SAFE(pFromRef, mp__proto__, *StringPool::GetString(SC___proto__), APT_REFREG_IS_APTVALUE);
    APT_REGISTER_REFERENCE_FROM_SAFE(pFromRef, mpPrototype, *StringPool::GetString(SC_prototype), APT_REFREG_IS_APTVALUE);

    if (IsEmpty())
    {
        return;
    }

    for (int i = 0; i < mnTotalSize; i++)
    {
        APT_ASSERT(mpData);
        APT_REGISTER_REFERENCE_FROM_SAFE(pFromRef, mpData[i].mValue, mpData[i].Key.c_str(), APT_REFREG_IS_APTVALUE);
    }

    return;
}
