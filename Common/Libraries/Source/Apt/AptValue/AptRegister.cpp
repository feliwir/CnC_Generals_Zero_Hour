#include "AptRegister.h"

#include "MainInline.h"
#include "_Apt.h" // for AptGetLib()->mInitParms


AptRegister *AptRegister::s_RegisterArray = NULL;

int32_t AptRegister::s_nMaxRegisters = 0;

/** Allocates and initializes the global AptRegister array. */
void AptRegister::Initialize()
{
    s_nMaxRegisters = AptGetLib()->mInitParms.iRegArraySize;
    if (s_RegisterArray == NULL)
    {
        // we need to allocate some AptRegister objects and initialize them
        s_RegisterArray = new AptRegister[s_nMaxRegisters];
        for (int32_t i = 0; i < s_nMaxRegisters; i++)
        {
            s_RegisterArray[i].nVal = i;
        }
    }
}

/** Deletes the global AptRegister array. */
void AptRegister::Shutdown()
{
    if (s_RegisterArray != NULL)
    {
        // we need to first delete all AptRegisters and then delete array
#if (APT_DEBUG_LEVEL >= APT_DEBUG_NORMAL)
        for (int32_t i = 0; i < s_nMaxRegisters; i++)
        {
            s_RegisterArray[i].SetDestroyedGC();
        }
#endif
        delete[] s_RegisterArray;
        s_RegisterArray = NULL;
    }
}

/**
 * Returns the AptRegister for @p nVal from the global array.
 * @param nVal register value
 */
AptRegister *AptRegister::Create(int32_t nVal)
{
    if (s_RegisterArray != NULL)
    {
        if (nVal < s_nMaxRegisters)
        {
            return (&s_RegisterArray[nVal]);
        }
        else
        {
            APT_ASSERTM((nVal < s_nMaxRegisters), "Apt fixed-size buffer overflow; increase the corresponding AptInitParams size");
            return NULL;
        }
    }
    else
    {
        return NULL;
    }
}
