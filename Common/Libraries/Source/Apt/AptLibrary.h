#pragma once

/*** Include files ********************************************************************************/
#include "Apt.h"
#include "AptDefine.h"
#include "AptString/EAString.h"

/*** Defines **************************************************************************************/

/*** Macros ***************************************************************************************/

/*** Type Definitions *****************************************************************************/

class DOGMA_PoolManager;
class AptValueGC_PoolManager;
class AptTarget;
class AptValueVector;
class AptSavedInputCheckpoints;
class AptRenderingContext;
class AptExtern;
class AptNativeHash;
class AptNativeFunction;
class AptGlobal;
class AptGlobalExtensionObject;
class IAptXmlImpl;

/** Recorded state used to play back a previously saved input stream. */
struct SavedInputPlayback
{
    unsigned char *pSavedInputs;
    unsigned char *pCurSavedInput;
    int nInputFileSize;
    int nLoadedAnims;
    unsigned int nCurTick;
};

//! Aggregate owner of Apt's process-wide state.
//!
//! This is NOT a FreeType-style library handle, and should not be described as
//! one: gpAptLibrary below is a single ambient root, and code reaches the
//! library through it rather than by being passed a handle. A genuinely
//! ambient-free design is not reachable while the inline operator new/delete
//! bodies in the public headers (AptValueGCAllocator.h, AptExtObject.h) have to
//! find the user callbacks with zero arguments.
//!
//! What this buys is one root with one lifetime instead of ~27 independent
//! globals with 27 independent lifetimes, plus an assert in AptGetLib() that
//! turns use-after-shutdown into a loud failure rather than a silent read of
//! stale state.
//!
//! Only one library may be live at a time. The remaining blockers for true
//! multi-instance are the statics this struct deliberately does not own: the
//! ~206 psMethod_* native function caches minted by NATIVE_MEMBER_FUNCTION,
//! StringPool, AptGC, AptExtObjectRegistry, the AptInteger/AptFloat free-list
//! pools, gAptActionInterpreter, AptFastStack, AptMath's clip stack and
//! AptScriptFunctionBase::spFrameStack. All of those are correctly reset by the
//! existing teardown path, so they are left alone.
struct AptLibrary
{
    // --- Created by AptLibraryInitialize, before anything can allocate -------
    AptUserFunctions mFuncs;
    DOGMA_PoolManager *mpNonGCPoolManager;
    AptValueGC_PoolManager *mpGCPoolManager;

    // --- The target registry ------------------------------------------------
    // mpCurrentTargetSim / mpCurrentTargetRender are the ambient "which target"
    // selection that the APT_SETUP_*_TARGET_STATES macros in AptTarget.h save
    // and restore. mnTargetsCreated counts only targets made through
    // AptCreateTargetInstance - it deliberately does not track mpDefaultTarget.
    AptTarget *mpDefaultTarget;
    AptTarget *mpCurrentTargetSim;
    AptTarget *mpCurrentTargetRender;
    uint32_t mnTargetsCreated;

    // --- Init state ---------------------------------------------------------
    // The parameters are kept because AptReset needs them again later.
    AptInitParams mInitParms;
    unsigned long mOptFlags; // APT_OPT_*; see AptOptIsEnabled in _AptInternalDefines.h
    int mbUpdateInitialized;
    int mbRenderInitialized;
    int mbCommonInitialized;

    // --- Behaviour flags, both set from AptInitParams at init ---------------
    bool mbDefaultMouseWheel;
    bool mbPrintZombieReferences;

#if defined(APT_DEBUG) && defined(APT_CXFORM_MATRIX_PROFILE)
    uint32_t mCXFormMult;
    uint32_t mCXFormNoMult;
    uint32_t mMatrixMult;
    uint32_t mMatrixNoMult;
    uint32_t mMatrixMMult;
    uint32_t mMatrixMNoMult;
#endif

    // --- Frame scalars ------------------------------------------------------
    unsigned int mnCurTick;
    int32_t mnCurrUpdateTick; // similar to mnCurTick, but writable so we can frame skip
    unsigned int mnCurrRenderTick;
    bool mbGarbageCollectThisFrame;
    bool mbBackgroundColorSet; // bg colour can only be set once
    int mbSavedInputsEnabled;

    // --- Per-instance heap state (what actually leaks if teardown is wrong) --
    AptValueVector *mpValuesToRelease;
    AptValueVector *mpZombieVector; // animations in the zombie state
    AptSavedInputCheckpoints *mpSavedInputCheckpoints;
    AptRenderingContext *mpRenderingContext;
    AptExtern *mpExternValue;
    AptNativeHash *mpObjRegistrationHash; // lazily built by Object.registerClass()
    AptNativeFunction *mpObjRegistrationFunc;
    IAptXmlImpl *mpAptXmlImpl;
    AptGlobal *mpGlobalGlobalObject;
    AptGlobalExtensionObject *mpGlobalExtensionObject;

    // --- By-value state -----------------------------------------------------
    // Constructed with the library (placement new in AptLibraryInitialize)
    // rather than at static init, which is strictly better defined: the
    // IsValid()/Validate() dance still sitting in AptUpdateInitialize exists
    // because a PS2 linker did not reliably run mstrTempString's static
    // constructor. That cannot happen now, so the dance is redundant - it is
    // left in place rather than removed as unrelated churn.
    AptNativeString mstrTempString;
    SavedInputPlayback mSIPlayback;
};

/*** Variables ************************************************************************************/

//! The single ambient root.
//!
//! Written in exactly two places (library init and shutdown) and read only
//! through AptGetLib(). Deliberately declared in this internal header: exposing
//! it from a public header in order to inline AptGetUserFuncs() would recreate
//! the very global this exists to remove. Don't.
extern AptLibrary *gpAptLibrary;

/*** Functions ************************************************************************************/

//! @return the live library. Body in AptLibrary.inl.
APT_INLINE AptLibrary *AptGetLib();

// Unlike the other .inl files, this one is pulled in here rather than only from
// MainInline.h. AptGetLib() is reached from nearly every TU, and in release
// (APT_ENABLE_INLINE) an inline function must be defined in every TU that calls
// it - requiring each of those to remember to include MainInline.h is a link
// error waiting to happen. In debug APT_INLINE is empty, so the body is
// compiled exactly once, out of line, from AptLibrary.cpp.
#if defined(APT_ENABLE_INLINE)
#include "AptLibrary.inl"
#endif
