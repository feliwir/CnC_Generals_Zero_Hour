/*** Include files ********************************************************************************/
#include "_Apt.h"
#include "AptLibrary.h"

#if !defined(APT_ENABLE_INLINE)
#include "AptLibrary.inl"
#endif

/*** Defines **************************************************************************************/

/*** Macros ***************************************************************************************/

/*** Type Definitions *****************************************************************************/

/*** Variables ************************************************************************************/

// The single ambient root.
//
// Written in exactly two places - AptLibraryInitialize() and
// AptLibraryShutdown(), both in Apt.cpp next to the pool code they bracket - and
// read only through AptGetLib(). NULL outside a library's lifetime, which is
// what makes the assert in AptGetLib() able to catch use-before-init and
// use-after-shutdown.
AptLibrary *gpAptLibrary = NULL;

/*** Private Functions ****************************************************************************/

/*** Public Functions *****************************************************************************/

void AptOptEnable(AptLibraryHandle hLib, unsigned long nFlags)
{
    APT_ASSERT(hLib == gpAptLibrary && "Apt supports one live library at a time");
    hLib->mOptFlags |= nFlags;
}

void AptOptDisable(AptLibraryHandle hLib, unsigned long nFlags)
{
    APT_ASSERT(hLib == gpAptLibrary && "Apt supports one live library at a time");
    hLib->mOptFlags &= ~nFlags;
}

unsigned long AptOptGetFlags(AptLibraryHandle hLib)
{
    APT_ASSERT(hLib != NULL);
    return hLib->mOptFlags;
}
