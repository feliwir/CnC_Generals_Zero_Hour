#include "AptValue/AptLookup.h"

#include "_Apt.h" // for AptGetLib()->mInitParms


AptLookup *AptLookup::s_LookupArray = NULL;

int32_t AptLookup::s_nMaxLookups = 0;

/** Sets up the global lookup objects. */
void AptLookup::Initialize()
{
    s_nMaxLookups = AptGetLib()->mInitParms.iLookupArraySize;
    if (s_LookupArray == NULL)
    {
        // we need to allocate some AptLookup objects and initialize them
        s_LookupArray = new AptLookup[s_nMaxLookups];
        for (int32_t i = 0; i < s_nMaxLookups; i++)
        {
            s_LookupArray[i].nLookup = i;
        }
    }
}

/** Deletes the global lookup objects. */
void AptLookup::Shutdown()
{
    if (s_LookupArray != NULL)
    {
        // we need to first delete all Lookups and then delete array
#if (APT_DEBUG_LEVEL >= APT_DEBUG_NORMAL)
        for (int32_t i = 0; i < s_nMaxLookups; i++)
        {
            s_LookupArray[i].SetDestroyedGC();
        }
#endif
        delete[] s_LookupArray;
        s_LookupArray = NULL;
    }
}

/**
 * Creates an AptLookup object. Returns from the global array.
 * @param nVal lookup value
 */
AptLookup *AptLookup::Create(int32_t nVal)
{
    if (s_LookupArray != NULL)
    {
        APT_ASSERT(nVal < s_nMaxLookups && "Apt- Please increase the iLookupArraySize member in AptInitParams while calling initializing Apt");
        if (nVal < s_nMaxLookups)
        {
            return (&s_LookupArray[nVal]);
        }
        else
        {
            APT_ASSERTM((nVal < s_nMaxLookups), "Apt fixed-size buffer overflow; increase the corresponding AptInitParams size");
            return NULL;
        }
    }
    else
    {
        return NULL;
    }
}
