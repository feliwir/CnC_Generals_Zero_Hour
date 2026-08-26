/**
@addtogroup g_referenceguide        Reference Guides
*/
//@{
/**
Module AptCore     Apt Core Library
*/
//@}

/*** Include files ********************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include "_Apt.h" // Include after stdio and stdlib.
#include "AptAnimationTarget.h"
#include "Display/AptDisplayListState.h"
#include "Display/AptDisplayList.h"
#include "AptFastStack.h"
#include "AptStd/AptMath.h"
#include "AptValue/AptValueVector.h"
#include "AptObject/AptStringObject.h"
#include "AptObject/AptDate.h"
#include "AptObject/AptExternalFunction.h"
#include "AptObject/AptMathObj.h"
#include "AptObject/AptKey.h"
#include "AptObject/AptLoadVars.h"
#include "AptObject/AptSound.h"
#include "AptObject/AptTextFormat.h"
#include "AptObject/AptMouse.h"
#include "AptObject/AptError.h"
#include "AptObject/AptStage.h"
#include "AptObject/AptScriptColour.h"
#include "AptObject/AptMovieClip.h"
#include "AptObject/AptMovieClipLoader.h"
#include "AptObject/AptUtil.h"
#include "AptObject/AptXmlNode.h"
#include "AptObject/AptXml.h"
#include "AptObject/AptXmlAttributes.h"
#include "AptObject/AptGlobalObject.h"
#include "AptObject/AptGlobalExtensionObject.h"
#include "AptExtObject.h"
#include "AptFrameStack.h"
#include "MainInline.h"
#include "AptCharacterInst.h"
#include "AptRenderItem.h"
#include "AptBCRenderTree.h"
#include "AptTarget.h"
#include "AptRenderList.h"
#include "AptSafeQueueFixed.h"
#include "AptRenderableCustomControl.h"
#include "SDK/AptExtObjectRegistry.h"

/*** Defines **************************************************************************************/
#if defined(APT_SYSTEM_BIG_ENDIAN)
#define APT_SAVEINPUTS_RECORD_SIZE_SAVER 8
#else
#define APT_SAVEINPUTS_RECORD_SIZE_SAVER 5
#endif

/*** Macros ***************************************************************************************/

/*** Type Definitions *****************************************************************************/

/*** Function Prototypes **************************************************************************/
void AptInitializeGC();
void CleanAllNativeFunctions();

void AptUpdateShutdown(AptLibraryHandle hLib, int bQuiet);
void AptCommonShutdown();
void AptRenderShutdown(AptLibraryHandle hLib);

void AptUpdateInitialize(AptLibraryHandle hLib, const AptInitParams *pAptInitParms);
void AptCommonInitialize(const AptInitParams *pAptInitParms);
void AptRenderInitialize(AptLibraryHandle hLib);

/*** Variables ************************************************************************************/


#if defined(APT_USE_STAGE_OBJECT)
#endif
#if defined(APT_USE_DATE_OBJECT)
#endif
#if defined(APT_USE_SOUND_OBJECT)
#endif
#if APT_USE_MATH_OBJECT
#endif
#if defined(APT_USE_MOUSE)
#endif
#if APT_USE_LOADVARS_OBJECT
#endif
#if defined APT_USE_BUTTONS
#endif
#if APT_USE_UTILITY
#endif
#if defined(APT_ALTERNATE_INPUT)
#endif

#include APT_INC_THREAD_H
#include APT_INC_THREAD_MUTEX_H
#include APT_INC_THREAD_FUTEX_H

// Private variables

APT_THREAD_NAMESPACE::Futex gAptSimThreadMutex;

#ifndef APT_SINGLE_THREADED
APT_THREAD_NAMESPACE::ThreadId gAptSimThreadId = APT_THREAD_NAMESPACE::kThreadIdInvalid;
#endif

// We want to ensure custom control zIDs are destroyed on the render thread.
// So we queue up the cust ctrl zID instead of freeing it immediately when its
// lifetime has expired.  Later, in the render thread, we dequeue and free the item.
//
// Not moved onto AptLibrary with the other by-value state: AptSafeQueueFixed.h
// includes _Apt.h, which includes AptLibrary.h, so a by-value member would be
// circular. Untangling that header is a separate cleanup with no behavioural
// benefit - the queue is Initialize()d and Uninitialize()d by
// AptRenderInitialize/AptRenderShutdown, so it holds nothing across a cycle.
static AptSafeQueueFixed sFreeCustomCtrlQueue;
APT_THREAD_NAMESPACE::Futex gAptFreeCustCtrlMutex;

// Public variables


AptInput gNullInput      = 0;


// we can save the incoming parameters to AptInitialize in this as we might need them later for AptReset

// Added Global Pool Variables.
// The pool managers and the user callbacks now live on the library
// (AptLibrary::mpNonGCPoolManager / mpGCPoolManager / mFuncs). These accessors
// are the only way to reach them, from this file and everywhere else.
AptValueGC_PoolManager *GetGCPoolManager()
{
    return AptGetLib()->mpGCPoolManager;
}

DOGMA_PoolManager *GetNonGCPoolManager()
{
    return AptGetLib()->mpNonGCPoolManager;
}


AptUserFunctions &AptGetUserFuncs(void)
{
    return AptGetLib()->mFuncs;
}

AptGlobalExtensionObject *AptGetGlobalExtensionObject(void)
{
    return AptGetLib()->mpGlobalExtensionObject;
}

// The AptGetLib()->mpZombieVector Vector keeps track of animations in the zombie state



#if defined(APT_GATHER_MOVIECLIP_METRICS)
AptMovieclipInformation gAptMovieclipInformation;
#endif

// This is the name we give to the AptFile associated with the Flash filters
// bytecode (the bytecode that contructs classes in flash.filters.*). No
// file is actually loaded-- as you'll see in AptPreloadFilterSWFFile, the
// code is just an array of bytes compiled into Apt.
static const char sFiltersPseudoFileName[] = "Apt-Flash-Native-Filters";

void AptUpdateLock();
void AptUpdateUnlock();

//-----------------------------------------------------------------------------

/**
    @brief AptUpdateLock() and AptUpdateUnlock() are used by the APT_INC/APT_DEC macros to ensure that
    external APT_INC and APT_DEC operations on AptValues are thread-safe.  The internal macros that
    correspond to these, _APT_INC and _APT_DEC are not thread-safe, and do not use AptUpdateLock()
    and AptUpdateUnlock().
*/
void AptUpdateLock()
{
    gAptSimThreadMutex.Lock();
}

void AptUpdateUnlock()
{
    gAptSimThreadMutex.Unlock();
}

void AptMovieclipInformation::Reset()
{
    memset((void *)this, 0, sizeof(AptMovieclipInformation));
}

void AptDebuggerPrint(int32_t nMsgLvl, const char *szFormat, ...)
{
    va_list args;
    char szBuf[4096];

    va_start(args, szFormat);
    vsprintf(szBuf, szFormat, args);
    va_end(args);

#if (APT_DEBUG_LEVEL > APT_DEBUG_NONE) || defined(APT_ENABLE_ZOMBIE_OUTPUT)
    AptGetUserFuncs().pfnDebugPrint("%s", szBuf);
#endif
}
// #endif

/*** Private Functions ****************************************************************************/

static void _passthruToMemFree(void *p, size_t size)
{
    AptGetUserFuncs().pfnMemFree(p);
}

/*** Public Functions *****************************************************************************/

AptUserFunctions::AptUserFunctions()
{
    pfnMemAlloc    = 0;
    pfnMemFree     = 0;
    pfnMemFreeSize = 0;
#if APT_USE_TEMPORARY_ALLOCATORS
    pfnTempAlloc    = 0;
    pfnTempFree     = 0;
    pfnTempFreeSize = 0;
#endif
    pfnRenderListAlloc     = 0;
    pfnRenderListFree      = 0;
    pfnRenderListFreeSize  = 0;
    pfnAssertFail          = 0;
    pfnAssertFailMsg       = 0;
    pfnSetBackgroundColour = 0;
    pfnDebugPrint          = 0;
    pfnDebugAddSavedInput        = 0;
    pfnDebugSetScreenGrabPending = 0;
    pfnLoadAnimation             = 0;
    pfnFreeAnimation             = 0;
    pfnFreeConstantTable         = 0;
    pfnLoadAnimationCompleted    = 0;
    pfnLoadImage                 = 0;
    pfnFreeImage                 = 0;
    pfnCommand                   = 0;
    pfnLoadVariables             = 0;
    pfnLoadVariablesNULL         = 0;
    pfnSetExternVariable         = 0;
    pfnGetExternVariable         = 0;
    pfnSendVariables             = 0;
    pfnAllocateString            = 0;
    pfnDeallocateString          = 0;
    pfnDrawString                = 0;
#if defined(APT_USE_SOUND_OBJECT)
    pfnLoadSound        = 0;
    pfnFreeSound        = 0;
    pfnStartSound       = 0;
    pfnStartSoundStream = 0;
#endif
    pfnLoadTexture             = 0;
    pfnFreeTexture             = 0;
    pfnBindTexture             = 0;
    pfnLoadRenderingUnit       = 0;
    pfnFreeRenderingUnit       = 0;
    pfnSetVertexMatrix         = 0;
    pfnSetColourTransform      = 0;
    pfnDrawRenderingUnit       = 0;
    pfnCustomControlRender     = 0;
    pfnOnUnload                = 0;
    pfnPointHitTest            = 0;
    pfnGetRealTimeClock        = 0;
    pfnGetBytesTotal           = 0;
    pfnGetBytesLoaded          = 0;
    pfnUninitializedVarAccess  = 0;
    pfnGetStageHeight          = 0;
    pfnGetStageWidth           = 0;
    pfnCustomControlUpdate     = 0;
    pfnPlaySavedInputsDone     = 0;
    pfnCustomSavedInputHandler = 0;
#if defined(APT_CUSTOM_CONTROL_USE_ZID)
    pfnCreateCustomControlZid     = 0;
    pfnDestroyCustomControlZid    = 0;
    pfnCustomControlRenderWithZid = 0;
#endif // APT_CUSTOM_CONTROL_USE_ZID
#if defined(APT_RENDER_FLAGS)
    pfnPushRenderFlags = 0;
    pfnPopRenderFlags  = 0;
#endif // APT_RENDER_FLAGS
    pfnGetCurrentTime    = 0;
    pfnHandleZombieState = 0;

    pfnCreateEffect  = 0;
    pfnDestroyEffect = 0;
    pfnPushEffect    = 0;
    pfnPopEffect     = 0;

    pfnRenderDispatch      = 0;
    pfnResetTargetInstance = 0;

    pfnUpperCaseString = 0;
    pfnLowerCaseString = 0;
}

void AptInitialize(AptLibraryHandle hLib, const AptInitParams *pAptInitParms, bool bCreateDefaultTarget)
{
    AptUpdateInitialize(hLib, pAptInitParms, bCreateDefaultTarget);
    AptRenderInitialize(hLib);

#ifdef APT_DEBUGGER_ENABLE
    AptDebugger::Initialize();
#endif
}

void AptUpdateInitialize(AptLibraryHandle hLib, const AptInitParams *pAptInitParms, bool bCreateDefaultTarget)
{
    APT_ASSERT(!AptGetLib()->mbUpdateInitialized);
    AptUpdateAutoLock lock;

    AptInitParams defParms;
    const AptInitParams &initParms = (pAptInitParms) ? (*pAptInitParms) : defParms;
    AptGetLib()->mInitParms                  = initParms;


    APT_ASSERT((initParms.iRegArraySize > 4) && "Register Array Size must be at least 4. Flash regularly uses these");

    // Added string initialization to fix issue with PS2 linker
    if (AptGetLib()->mstrTempString.IsValid() == false)
    {
        AptGetLib()->mstrTempString.Validate();
    }
#if defined(APT_USE_MOUSE)
    AptGetLib()->mbDefaultMouseWheel = initParms.bDefaultMouseWheelFlag;
#else
    AptGetLib()->mbDefaultMouseWheel = false;
#endif
    AptGetLib()->mbPrintZombieReferences = initParms.bPrintZombieDump;
    // APT_ASSERT(AptGetUserFuncs().pfnAssertFail); // APT_ASSERT defined in terms of pfnAssertFail
    APT_ASSERT(AptGetUserFuncs().pfnMemAlloc);
    APT_ASSERT(AptGetUserFuncs().pfnMemFree);
    if (AptGetUserFuncs().pfnMemFreeSize == 0)
    {
        AptGetUserFuncs().pfnMemFreeSize = _passthruToMemFree;
    }
#if APT_USE_TEMPORARY_ALLOCATORS
    APT_ASSERT(AptGetUserFuncs().pfnTempAlloc);
    APT_ASSERT(AptGetUserFuncs().pfnTempFree);
    APT_ASSERT(AptGetUserFuncs().pfnTempFreeSize);
#endif
    APT_ASSERT(AptGetUserFuncs().pfnSetBackgroundColour);
    APT_ASSERT(AptGetUserFuncs().pfnDebugPrint);
    APT_ASSERT(AptGetUserFuncs().pfnDebugAddSavedInput);
    APT_ASSERT(AptGetUserFuncs().pfnDebugSetScreenGrabPending);
    APT_ASSERT(AptGetUserFuncs().pfnLoadAnimation);
    APT_ASSERT(AptGetUserFuncs().pfnFreeConstantTable);
    APT_ASSERT(AptGetUserFuncs().pfnFreeAnimation);
    APT_ASSERT(AptGetUserFuncs().pfnCommand);
    APT_ASSERT(AptGetUserFuncs().pfnLoadVariables);
    APT_ASSERT(AptGetUserFuncs().pfnSetExternVariable);
    APT_ASSERT(AptGetUserFuncs().pfnGetExternVariable);
    APT_ASSERT(AptGetUserFuncs().pfnAllocateString);
    APT_ASSERT(AptGetUserFuncs().pfnDeallocateString);
    APT_ASSERT(AptGetUserFuncs().pfnDrawString);
#if defined(APT_USE_SOUND_OBJECT)
    APT_ASSERT(AptGetUserFuncs().pfnLoadSound);
    APT_ASSERT(AptGetUserFuncs().pfnFreeSound);
    APT_ASSERT(AptGetUserFuncs().pfnStartSound);
    APT_ASSERT(AptGetUserFuncs().pfnStartSoundStream);
#endif
    APT_ASSERT(AptGetUserFuncs().pfnLoadTexture);
    APT_ASSERT(AptGetUserFuncs().pfnFreeTexture);
    APT_ASSERT(AptGetUserFuncs().pfnBindTexture);
    // APT_ASSERT(AptGetUserFuncs().pfnSetVertexMatrix);    // now deprecated in favor of AptRenderInfo
    // APT_ASSERT(AptGetUserFuncs().pfnSetColourTransform); // now deprecated in favor of AptRenderInfo
    APT_ASSERT(AptGetUserFuncs().pfnLoadRenderingUnit);
    APT_ASSERT(AptGetUserFuncs().pfnFreeRenderingUnit);
    APT_ASSERT(AptGetUserFuncs().pfnDrawRenderingUnit);
    APT_ASSERT(AptGetUserFuncs().pfnCustomControlRender);
    APT_ASSERT(AptGetUserFuncs().pfnGetStageHeight);
    APT_ASSERT(AptGetUserFuncs().pfnGetStageWidth);
    // AptGetUserFuncs().pfnPointHitTest;    // do not assert on this right now as everybody might not be implementing it.

#ifndef APT_SINGLE_THREADED
    gAptSimThreadId = APT_THREAD_NAMESPACE::GetThreadId();
#endif

    // Moved rearranged initialization to make things more straightforward.
    AptInitializeGC();

    if (!AptGetLib()->mbCommonInitialized)
    {
        AptCommonInitialize(&AptGetLib()->mInitParms);
    }

    if (initParms.iZombieVectorSize == 0)
    {
        AptGetLib()->mpZombieVector = NULL;
    }
    else
    {
        AptGetLib()->mpZombieVector = new AptValueVector(initParms.iZombieVectorSize);
    }

    // Create the manager before any CIH object might be created.
    // Initialize global variables like gpundefined var and others.
    AptValueInitialize();
    // Initialize the action interpreter variables.
    gAptActionInterpreter.initialize(initParms);

    AptGetLib()->mpSavedInputCheckpoints = new AptSavedInputCheckpoints();

    if (bCreateDefaultTarget)
    {
        // pool sizes are parameterized.
        AptGetLib()->mpDefaultTarget       = new AptTarget(&AptGetLib()->mInitParms);
        AptGetLib()->mpCurrentTargetSim    = AptGetLib()->mpDefaultTarget;
        AptGetLib()->mpCurrentTargetRender = AptGetLib()->mpDefaultTarget;

        // preload the filters SWF file.
        AptPreloadFilterSWFFile();
    }
    else
    {
        AptGetLib()->mpDefaultTarget       = NULL;
        AptGetLib()->mpCurrentTargetSim    = NULL;
        AptGetLib()->mpCurrentTargetRender = NULL;
    }

    AptGetLib()->mbUpdateInitialized = 1;

#if defined(APT_GENERATE_SIZE_CHECK_CODE)
    // Define this to generate the code below with the current sizes (then copy from TTL to the source code).
#define APT_SIZE_CHECK(_obj, oldsize) \
    APT_DEBUGPRINT(APT_DEBUG_MSG_DEFAULT_LVL, "APT_SIZE_CHECK( %30.30s, /*OLD_SIZE*/ 0x%4.4x ); // This warns of object sizes changing \n", #_obj, sizeof(_obj))

#elif defined(APT_VERIFY_OBJECT_SIZES)
    // ASSERT if the sizes don't match.
#define APT_SIZE_CHECK(_obj, oldsize) \
    APT_ASSERT(sizeof(_obj) == oldsize)
#else
    // By default we ignore this.
#define APT_SIZE_CHECK(_obj, oldsize)
#endif

    // Note that all garbage collected objects are 8 bytes larger then this for GC overhead.
    APT_SIZE_CHECK(AptValue, /*OLD_SIZE*/ 0x0008);                       // This warns of object sizes changing
    APT_SIZE_CHECK(AptCIH, /*OLD_SIZE*/ 0x0060);                         // This warns of object sizes changing
    APT_SIZE_CHECK(AptFrameStack, /*OLD_SIZE*/ 0x0020);                  // This warns of object sizes changing
    APT_SIZE_CHECK(AptObject, /*OLD_SIZE*/ 0x001c);                      // This warns of object sizes changing
    APT_SIZE_CHECK(AptNativeHash, /*OLD_SIZE*/ 0x0014);                  // This warns of object sizes changing
    APT_SIZE_CHECK(AptNativeString, /*OLD_SIZE*/ 0x0004);                // This warns of object sizes changing
    APT_SIZE_CHECK(AptString, /*OLD_SIZE*/ 0x0010);                      // This warns of object sizes changing
    APT_SIZE_CHECK(AptBoolean, /*OLD_SIZE*/ 0x000c);                     // This warns of object sizes changing
    APT_SIZE_CHECK(AptNativeFunction, /*OLD_SIZE*/ 0x0024);              // This warns of object sizes changing
    APT_SIZE_CHECK(AptScriptFunctionBase, /*OLD_SIZE*/ 0x0030);          // This warns of object sizes changing
    APT_SIZE_CHECK(AptScriptFunction1, /*OLD_SIZE*/ 0x0034);             // This warns of object sizes changing
    APT_SIZE_CHECK(AptScriptFunction2, /*OLD_SIZE*/ 0x0034);             // This warns of object sizes changing
    APT_SIZE_CHECK(AptScriptFunctionByteCodeBlock, /*OLD_SIZE*/ 0x0044); // This warns of object sizes changing
    APT_SIZE_CHECK(AptLookup, /*OLD_SIZE*/ 0x000c);                      // This warns of object sizes changing
    APT_SIZE_CHECK(AptInteger, /*OLD_SIZE*/ 0x000c);                     // This warns of object sizes changing
    APT_SIZE_CHECK(AptRegister, /*OLD_SIZE*/ 0x000c);                    // This warns of object sizes changing
    APT_SIZE_CHECK(AptFloat, /*OLD_SIZE*/ 0x000c);                       // This warns of object sizes changing
    APT_SIZE_CHECK(AptArray, /*OLD_SIZE*/ 0x0028);                       // This warns of object sizes changing
    APT_SIZE_CHECK(AptSound, /*OLD_SIZE*/ 0x0028);                       // This warns of object sizes changing
    APT_SIZE_CHECK(AptKey, /*OLD_SIZE*/ 0x001c);                         // This warns of object sizes changing
    APT_SIZE_CHECK(AptMathObj, /*OLD_SIZE*/ 0x001c);                     // This warns of object sizes changing
    APT_SIZE_CHECK(AptNone, /*OLD_SIZE*/ 0x0008);                        // This warns of object sizes changing
    APT_SIZE_CHECK(AptScriptColour, /*OLD_SIZE*/ 0x0020);                // This warns of object sizes changing
    APT_SIZE_CHECK(AptObject, /*OLD_SIZE*/ 0x001c);                      // This warns of object sizes changing
    APT_SIZE_CHECK(AptPrototype, /*OLD_SIZE*/ 0x0020);                   // This warns of object sizes changing
    APT_SIZE_CHECK(AptDate, /*OLD_SIZE*/ 0x0060);                        // This warns of object sizes changing
    APT_SIZE_CHECK(AptGlobal, /*OLD_SIZE*/ 0x001c);                      // This warns of object sizes changing
    APT_SIZE_CHECK(AptTextFormat, /*OLD_SIZE*/ 0x0040);                  // This warns of object sizes changing
    APT_SIZE_CHECK(AptMovieClip, /*OLD_SIZE*/ 0x0020);                   // This warns of object sizes changing
    APT_SIZE_CHECK(AptXmlNode, /*OLD_SIZE*/ 0x0028);                     // This warns of object sizes changing
    APT_SIZE_CHECK(AptXml, /*OLD_SIZE*/ 0x0028);                         // This warns of object sizes changing
    APT_SIZE_CHECK(AptXmlAttributes, /*OLD_SIZE*/ 0x0024);               // This warns of object sizes changing
    APT_SIZE_CHECK(AptValueVector, /*OLD_SIZE*/ 0x0010);                 // This warns of object sizes changing
    APT_SIZE_CHECK(AptLoadVars, /*OLD_SIZE*/ 0x0024);                    // This warns of object sizes changing
    APT_SIZE_CHECK(AptStage, /*OLD_SIZE*/ 0x001c);                       // This warns of object sizes changing
    APT_SIZE_CHECK(AptError, /*OLD_SIZE*/ 0x0024);                       // This warns of object sizes changing
    APT_SIZE_CHECK(AptStringObject, /*OLD_SIZE*/ 0x0020);                // This warns of object sizes changing
}

void AptRenderInitialize(AptLibraryHandle hLib)
{
    AptUpdateAutoLock lock;

    APT_ASSERT(!AptGetLib()->mbRenderInitialized);

    if (!AptGetLib()->mbCommonInitialized)
    {
        AptCommonInitialize(&AptGetLib()->mInitParms);
    }

    // Create the manager before any CIH object might be created.
    // create the Rendertree manager
    AptMath::ClipStackInit(128);
    AptGetLib()->mbRenderInitialized = 1;

    sFreeCustomCtrlQueue.Initialize((uint32_t)AptRenderableCustomControl::MAX_FREE_CUSTCTRLS, &gAptFreeCustCtrlMutex);
    AptRenderableCustomControl::SetFreeQueue(&sFreeCustomCtrlQueue);

    AptRenderList::Initialize();
}

void AptCommonInitialize(const AptInitParams *pAptInitParms)
{
    AptGetLib()->mbCommonInitialized = 1;

    AptAnimationTarget::SetupStaticData(*pAptInitParms);
    AptNativeString::MemInitialize(&AptGetUserFuncs());
    StringPool::Initialize(pAptInitParms->iStringPoolSize);
    AptGetLib()->mpValuesToRelease = new AptValueVector(pAptInitParms->iDeferedVectorSize);
}

void AptReset(AptLibraryHandle hLib)
{
    AptUpdateAutoLock lock;

    APT_ASSERT(GetTarget()->GetAnimationTarget());

    gAptActionInterpreter.bShutDown = true;
    // AptMath::ClipStackShutdown();
    AptScriptFunctionBase::ShutdownStaticData();
    GetTarget()->GetAnimationTarget()->PreDestroy();

    AptUpdateZombieVector(true); // Clean out the zombie vector

    GetTarget()->Reset();
    delete AptGetLib()->mpSavedInputCheckpoints;

    AptValueShutdown(true);
    CleanAllNativeFunctions();

    AptGC::CleanAll();
    AptValueShutdownRemaining();

    // this has to be done later after garbage collection
    AptGetLib()->mpValuesToRelease->ReleaseValues();

    AptGetLib()->mpDefaultTarget       = new AptTarget(&AptGetLib()->mInitParms);
    AptGetLib()->mpCurrentTargetSim    = AptGetLib()->mpDefaultTarget;
    AptGetLib()->mpCurrentTargetRender = AptGetLib()->mpDefaultTarget;

    if (AptGetUserFuncs().pfnResetTargetInstance)
    {
        AptGetUserFuncs().pfnResetTargetInstance(AptGetLib()->mpDefaultTarget);
    }

    GetTarget()->GetAnimationTarget()->GetDisplayList()->pState = new AptDisplayListState();
    AptValueInitialize();
    AptScriptFunctionBase::InitializeStaticData(AptGetLib()->mInitParms);

    AptGetLib()->mpSavedInputCheckpoints = new AptSavedInputCheckpoints();

    GetTarget()->GetAnimationTarget()->Reset();
    gAptActionInterpreter.bShutDown = false;

    // preload the filters SWF file.
    AptPreloadFilterSWFFile();
}

void AptShutdown(AptLibraryHandle hLib, int bQuiet)
{
#ifdef APT_DEBUGGER_ENABLE
    AptDebugger::Shutdown();
#endif
    AptUpdateShutdown(hLib, bQuiet);
    AptRenderShutdown(hLib);
    AptGetLib()->mbCommonInitialized = 0;
}

void AptUpdateShutdown(AptLibraryHandle hLib, int bQuiet)
{
    AptUpdateAutoLock lock;

    AptRenderList::Stop();

    APT_ASSERT(AptGetLib()->mbUpdateInitialized);
    gAptActionInterpreter.bShutDown = true;
    AptGetLib()->mbUpdateInitialized             = 0;

    //  Before Garbage Collection, we clean all the native functions
    CleanAllNativeFunctions();

    AptGetLib()->mpCurrentTargetSim    = AptGetLib()->mpDefaultTarget;
    AptGetLib()->mpCurrentTargetRender = AptGetLib()->mpDefaultTarget;

    if (AptGetLib()->mpDefaultTarget)
    {
        APT_SETUP_SIM_TARGET_STATES(AptGetLib()->mpDefaultTarget);
        APT_SETUP_RENDER_TARGET_STATES(AptGetLib()->mpDefaultTarget);
        AptGetLib()->mpDefaultTarget->GetAnimationTarget()->PreDestroy();
        AptGetLib()->mpDefaultTarget->GetAnimationTarget()->CleanRemList();
        APT_TEARDOWN_RENDER_TARGET_STATES;
        APT_TEARDOWN_SIM_TARGET_STATES;
    }

    AptValueShutdown(bQuiet);

    AptUpdateZombieVector(true); // Clean out the zombie vector

    delete AptGetLib()->mpSavedInputCheckpoints;
    AptGetLib()->mpSavedInputCheckpoints = NULL;

    AptGC::CleanAll();
    // AptValueShutdownRemaining();

    // this has to be done later after garbage collection
    AptGetLib()->mpValuesToRelease->ReleaseValues();

    if (AptGetLib()->mpZombieVector != NULL)
    {
        delete AptGetLib()->mpZombieVector;
    }
    AptGetLib()->mpZombieVector = NULL;

    gAptActionInterpreter.shutdown();


    if (AptGetLib()->mbRenderInitialized == 0)
    {
        // that means render has already been shutdown and now update is last to shutdown.
        // so clean up remaining things.
        AptCommonShutdown();
    }

#ifndef APT_SINGLE_THREADED
    gAptSimThreadId = APT_THREAD_NAMESPACE::kThreadIdInvalid;
#endif
}

static void AptFreeOutstandingCustomControls()
{
    if (AptGetUserFuncs().pfnDestroyCustomControlZid)
    {
        sFreeCustomCtrlQueue.DequeueAndFreeAll(AptGetUserFuncs().pfnDestroyCustomControlZid);
    }
}

void AptRenderShutdown(AptLibraryHandle hLib)
{
    AptUpdateAutoLock lock;

    AptRenderList::Stop();

    APT_ASSERT(AptGetLib()->mbRenderInitialized);
    AptGetLib()->mbRenderInitialized = 0;
    AptMath::ClipStackShutdown();

    if (AptGetLib()->mbUpdateInitialized == 0)
    {
        // this means that update has already shutdown and now render is last one to shutdown.
        // so cleanup things which are common.
        AptCommonShutdown();
    }

    AptFreeOutstandingCustomControls();
    AptRenderableCustomControl::SetFreeQueue(NULL);
    sFreeCustomCtrlQueue.Uninitialize();
}

void AptCommonShutdown()
{
    AptGetLib()->mbRenderInitialized = 0;

    AptAnimationTarget::CleanupStaticData();

    if (AptGetLib()->mpDefaultTarget)
    {
        delete AptGetLib()->mpDefaultTarget;
    }

    AptGetLib()->mpDefaultTarget       = NULL;
    AptGetLib()->mpCurrentTargetSim    = NULL;
    AptGetLib()->mpCurrentTargetRender = NULL;
    APT_ASSERT(AptGetLib()->mnTargetsCreated == 0 && "Forgot to remove some AptTargetInstance's")

    AptRenderList::Shutdown();

#if defined(APT_RI_LIST_VERIFY)
    AptRenderItem::CleanUp();
    APT_ASSERT(AptRenderItem::spHeadItem == NULL);
#endif

    AptValueShutdownRemaining();
    AptGetLib()->mstrTempString.Clear(); //  Clear the temporary string

    // Must ensure Free List pools are emptied at shutdown!
    StringPool::Teardown();

    //  We need to destruct AptGetLib()->mpValuesToRelease after StringPool::Teardown()
    //  Because the clamped strings will be destructed here and added in the AptGetLib()->mpValuesToRelease vector
    AptGetLib()->mpValuesToRelease->ReleaseValues();
    delete AptGetLib()->mpValuesToRelease;
    AptGetLib()->mpValuesToRelease = NULL;

    AptInteger::ClearPool();
    AptFloat::ClearPool();
    StringPool::ClearTemporaryPool();

    AptNativeString::MemUninitialize();
    AptGetLib()->mbCommonInitialized = 0;
}

//-------------------------------------------------------------
#if defined(APT_ALLOCATION_TRACKING)
// Added Tracking Callbacks to print out data about an allocated memory chuck. Feel free to add to these.
void PrintNonGCTrackedAllocation(TrackedAllocation *pTracker)
{
    APT_DEBUGPRINT(APT_DEBUG_MSG_DEFAULT_LVL, "  ptr:%p Size:0x%6.6X File:%s Line:%d\n", pTracker->pAllocated, pTracker->nAllocSize, pTracker->szFileName, pTracker->nLineNumber);
}
void PrintGCTrackedAllocation(TrackedAllocation *pTracker)
{
    APT_DEBUGPRINT(APT_DEBUG_MSG_DEFAULT_LVL, "  AptValueGC:%p VFT:%d Size:0x%6.6X File:%s Line:%d\n", pTracker->pAllocated, ((AptValueGC *)pTracker->pAllocated)->getVtblIndex(), pTracker->nAllocSize, pTracker->szFileName, pTracker->nLineNumber);
}
#endif

AptLibraryHandle AptLibraryInitialize(const AptLibraryInitParams *pAptLibInitParms)
{
    AptUpdateAutoLock lock;

    // Deliberately plain tests rather than APT_ASSERT: asserting routes through
    // pfnAssertFail, which is part of the very callback block being validated,
    // and there is no library to reach it through yet.
    if (pAptLibInitParms == NULL ||
        pAptLibInitParms->Funcs.pfnMemAlloc == NULL ||
        pAptLibInitParms->Funcs.pfnMemFree == NULL)
    {
        return NULL;
    }
    if (gpAptLibrary != NULL)
    {
        return NULL; // one live library at a time
    }

    // The library cannot use APT_NEW_DELETE_OPERATORS: those route through the
    // pool managers this function is about to create. Allocate it with the
    // caller's own allocator instead.
    void *pMem = pAptLibInitParms->Funcs.pfnMemAlloc(APT_ALLOC_PARAMS(sizeof(AptLibrary)));
    if (pMem == NULL)
    {
        return NULL;
    }

    AptLibrary *pLib = new (pMem) AptLibrary();
    pLib->mFuncs     = pAptLibInitParms->Funcs;
    pLib->mOptFlags  = pAptLibInitParms->nOptFlags;

    // Must be patched before the pool managers exist: DOGMA_PoolManager's
    // operator delete and destructor both call pfnMemFreeSize.
    if (pLib->mFuncs.pfnMemFreeSize == 0)
    {
        pLib->mFuncs.pfnMemFreeSize = _passthruToMemFree;
    }

    // Publish before creating the pools: their operator new reaches the
    // callbacks through AptGetUserFuncs(), which reads gpAptLibrary.
    gpAptLibrary = pLib;

    AptValueGC_PoolManager::StaticInitialize();
    pLib->mpNonGCPoolManager = new DOGMA_PoolManager(pAptLibInitParms->nNonGCMainPoolSize, pAptLibInitParms->nNonGCOverflowPoolSize, 4, 256, 0, false, 0, true);
    pLib->mpGCPoolManager    = new AptValueGC_PoolManager(static_cast<uint32_t>(pAptLibInitParms->nGCMainPoolSize), static_cast<uint32_t>(pAptLibInitParms->nGCOverflowPoolSize));


#if defined(APT_ALLOCATION_TRACKING)
    GetNonGCPoolManager()->mnTrackers      = 0x2000;
    GetNonGCPoolManager()->mnTrackThisSize = GetNonGCPoolManager()->TRACK_SIZE_ALL;
    GetGCPoolManager()->mnTrackers         = 0x2000;
    GetGCPoolManager()->mnTrackThisSize    = GetNonGCPoolManager()->TRACK_SIZE_ALL;
#endif

    return pLib;
}

void AptLibraryShutdown(AptLibraryHandle hLib)
{
    AptUpdateAutoLock lock;

    APT_ASSERT(hLib != NULL);
    APT_ASSERT(hLib == gpAptLibrary && "Apt supports one live library at a time");
    APT_ASSERT(AptGetLib()->mbUpdateInitialized == 0 && "AptShutdown must be called before AptLibraryShutdown!");

    // Don't print this crap in Coverage Builds, unless there are leaks.
#if defined(DO_COVERAGE)
    if (GetNonGCPoolManager()->mnItemsAllocated != 0)
#endif
    {
        APT_DEBUGPRINT(APT_DEBUG_MSG_DEFAULT_LVL, "--+-Apt NonGC Items Pool Shutdown\n");

#if !defined(DO_COVERAGE) && defined(DOGMA_EXTRA_MEMORY_COUNTERS)
        {
            // Even if there are leaks, don't display mutable data in coverage build.
            APT_DEBUGPRINT(APT_DEBUG_MSG_DEFAULT_LVL, "  +- Items In Free Lists:0x%X\n", GetNonGCPoolManager()->mnItemsFreed);
        }
#endif

#if defined(APT_ALLOCATION_TRACKING)
        if (GetNonGCPoolManager()->mpTrackers != NULL)
        {
            for (uint32_t i = 0; i < GetNonGCPoolManager()->mnTrackers; i++)
            {
                if (GetNonGCPoolManager()->mpTrackers[i].pAllocated != NULL)
                {
                    PrintNonGCTrackedAllocation(&GetNonGCPoolManager()->mpTrackers[i]);
                }
            }
        }

        if (GetGCPoolManager()->mpTrackers != NULL)
        {
            for (uint32_t i = 0; i < GetGCPoolManager()->mnTrackers; i++)
            {
                if (GetGCPoolManager()->mpTrackers[i].pAllocated != NULL)
                {
                    PrintGCTrackedAllocation(&GetGCPoolManager()->mpTrackers[i]);
                }
            }
        }
        APT_ASSERT(GetNonGCPoolManager()->mnBytesAllocatedInDogma == 0 && "MEMORY LEAK!!! Apt is shutting down with bytes allocated in Dogma!!!");
#endif

        if (GetNonGCPoolManager()->mnBytesAllocatedInDogma != 0)
        {
            APT_DEBUGPRINT(APT_DEBUG_MSG_DEFAULT_LVL, "  +- Bytes Allocated    :0x%X   <-- !Warning! Shutting down with bytes allocated!!!\n", GetNonGCPoolManager()->mnBytesAllocatedInDogma);
        }

        if (GetNonGCPoolManager()->mnBytesAllocatedOutsideDogma != 0 && GetNonGCPoolManager()->GetFirstOutsideAllocation() != NULL)
        {
            void *pCurrent = GetNonGCPoolManager()->GetFirstOutsideAllocation();
            int count      = 1;
            while (pCurrent != NULL)
            {

#if !defined(DO_COVERAGE)
                {
                    APT_DEBUGPRINT(APT_DEBUG_MSG_DEFAULT_LVL, "  +- Apt Non-GC Still Allocated! p:0x%p\n", pCurrent);
                }
#else
                {
                    // Don't print pointers in coverage build.
                    APT_DEBUGPRINT(APT_DEBUG_MSG_DEFAULT_LVL, "  +- Apt Non-GC Still Allocated! p:0x%x\n", -1);
                }
#endif

                pCurrent = GetNonGCPoolManager()->GetNextOutsideAllocation(pCurrent);
                count++;
            }
        }

#if defined(DOGMA_EXTRA_MEMORY_COUNTERS)
        {
            // Give more detailed results if this is defined.
            if (GetNonGCPoolManager()->mnBytesAllocatedInDogma != 0)
            {
                APT_DEBUGPRINT(APT_DEBUG_MSG_DEFAULT_LVL, "  +- Bytes Allocated Non-GC in DOGMA  :0x%X\n", GetNonGCPoolManager()->mnBytesAllocatedInDogma);
            }

            if (GetNonGCPoolManager()->mnBytesAllocatedOutsideDogma != 0)
            {
                APT_DEBUGPRINT(APT_DEBUG_MSG_DEFAULT_LVL, "  +- Bytes Allocated Non-GC Outside DOGMA   :0x%X\n", GetNonGCPoolManager()->mnBytesAllocatedOutsideDogma);
            }

#if !defined(DO_COVERAGE)
            {
                // Even if there are leaks, don't display mutable data in coverage build.
                APT_DEBUGPRINT(APT_DEBUG_MSG_DEFAULT_LVL, "  +- Bytes In Free Lists:0x%X\n", GetNonGCPoolManager()->mnBytesInFreeLists);
            }
#endif
        }
#endif

#if !defined(DO_COVERAGE)
        {
            // Even if there are leaks, don't display mutable data in coverage build.
            APT_DEBUGPRINT(APT_DEBUG_MSG_DEFAULT_LVL, "  +- Total Bytes Used   :0x%X\n", GetNonGCPoolManager()->GetTotalBytesUsed());
            APT_DEBUGPRINT(APT_DEBUG_MSG_DEFAULT_LVL, "  +- Number of Overflow Pools Created: %d\n", GetNonGCPoolManager()->GetNumOverflowPools());
        }
#endif
    }

#if defined(DO_COVERAGE)
    if (GetGCPoolManager()->mnItemsAllocated != 0)
#endif
    {
        APT_DEBUGPRINT(APT_DEBUG_MSG_DEFAULT_LVL, "--+-Apt Garbage Collected Items Pool Shutdown\n");
#if !defined(DO_COVERAGE) && defined(DOGMA_EXTRA_MEMORY_COUNTERS)
        {
            // Even if there are leaks, don't display mutable data in coverage build.
            APT_DEBUGPRINT(APT_DEBUG_MSG_DEFAULT_LVL, "  +- Items In Free Lists:0x%X\n", GetGCPoolManager()->mnItemsFreed);
        }
#endif

        if (GetGCPoolManager()->mnBytesAllocatedInDogma != 0)
        {
            APT_DEBUGPRINT(APT_DEBUG_MSG_DEFAULT_LVL, "  +- Bytes Allocated    :0x%X   <-- !Warning! Shutting down with bytes allocated!!!\n", GetGCPoolManager()->mnBytesAllocatedInDogma);
        }

        if (GetGCPoolManager()->mnBytesAllocatedOutsideDogma != 0 && GetGCPoolManager()->GetFirstOutsideAllocation() != NULL)
        {
            AptValue *pCurrent = GetGCPoolManager()->GetFirstAptValue();
            int count          = 1;
            while (pCurrent != NULL)
            {

#if !defined(DO_COVERAGE)
                {
                    APT_DEBUGPRINT(APT_DEBUG_MSG_DEFAULT_LVL, "  +- AptValue Still Allocated! this:0x%p Type:0x%X Size:0x%X\n", pCurrent, pCurrent->getVtblIndex(), AptGetSizeOfAptValue(pCurrent));
                }
#else
                {
                    // Don't print pointers in coverage build.
                    APT_DEBUGPRINT(APT_DEBUG_MSG_DEFAULT_LVL, "  +- AptValue Still Allocated! this:0x%x Type:0x%X Size:0x%X\n", -1, pCurrent->getVtblIndex(), AptGetSizeOfAptValue(pCurrent));
                }
#endif

                pCurrent = GetGCPoolManager()->GetNextAptValue(pCurrent);
                count++;
            }
            APT_DEBUGPRINT(APT_DEBUG_MSG_DEFAULT_LVL, "  +- Total Items Allocated    :0x%X   <-- !Warning! Shutting down with items allocated!!!\n", count);
        }

#if defined(DOGMA_EXTRA_MEMORY_COUNTERS)
        {
            // Give more detailed results if this is defined.
            if (GetGCPoolManager()->mnBytesAllocatedInDogma != 0)
            {
                APT_DEBUGPRINT(APT_DEBUG_MSG_DEFAULT_LVL, "  +- Bytes Allocated    :0x%X\n", GetGCPoolManager()->mnBytesAllocatedInDogma);
            }

            if (GetGCPoolManager()->mnBytesAllocatedOutsideDogma != 0)
            {
                APT_DEBUGPRINT(APT_DEBUG_MSG_DEFAULT_LVL, "  +- Bytes Allocated    :0x%X\n", GetGCPoolManager()->mnBytesAllocatedOutsideDogma);
            }

#if !defined(DO_COVERAGE)
            {
                // Even if there are leaks, don't display mutable data in coverage build.
                APT_DEBUGPRINT(APT_DEBUG_MSG_DEFAULT_LVL, "  +- Bytes In Free Lists:0x%X\n", GetGCPoolManager()->mnBytesInFreeLists);
            }
#endif
        }
#endif

#if !defined(DO_COVERAGE)
        {
            // Even if there are leaks, don't display mutable data in coverage build.
            APT_DEBUGPRINT(APT_DEBUG_MSG_DEFAULT_LVL, "  +- Total Bytes Used   :0x%X\n", GetGCPoolManager()->GetTotalBytesUsed());
            APT_DEBUGPRINT(APT_DEBUG_MSG_DEFAULT_LVL, "  +- Number of Overflow Pools Created: %d\n", GetGCPoolManager()->GetNumOverflowPools());
        }
#endif
    }

#if defined(APT_ALLOCATION_TRACKING)
    // One final assert if there are outstanding allocations, after all the above output
    if (GetNonGCPoolManager()->mnBytesAllocatedInDogma != 0)
    {
        APT_ASSERTM(false, "AptLibraryShutdown :: Non-garbage-collected memory (DOGMA) has been leaked.");
    }
#endif


    delete AptGetLib()->mpNonGCPoolManager;
    delete AptGetLib()->mpGCPoolManager;

    AptGetLib()->mpNonGCPoolManager = 0;
    AptGetLib()->mpGCPoolManager    = 0;

    // Take a copy: freeing the library destroys the callbacks it holds, and the
    // pool teardown above needed to reach them through gpAptLibrary.
    AptUserFunctions funcs = hLib->mFuncs;

    hLib->~AptLibrary();
    gpAptLibrary = NULL;
    funcs.pfnMemFree(hLib);
}

// registers all the callback functions for garbage collected classes.
void AptInitializeGC()
{
    AptGC::Initialize();
}

void CleanAllNativeFunctions()
{
    AptArray::CleanNativeFunctions();
#if defined(APT_USE_DATE_OBJECT)
    AptDate::CleanNativeFunctions();
#endif
#if APT_USE_MATH_OBJECT
    AptMathObj::CleanNativeFunctions();
#endif
    AptKey::CleanNativeFunctions();
#if defined(APT_USE_MOUSE)
    AptMouse::CleanNativeFunctions();
#endif
#if APT_USE_LOADVARS_OBJECT
    AptLoadVars::CleanNativeFunctions();
#endif
#if defined(APT_USE_SOUND_OBJECT)
    AptSound::CleanNativeFunctions();
#endif
#if APT_USE_SCRIPTCOLOUR_OBJECT
    AptScriptColour::CleanNativeFunctions();
#endif
    AptXml::CleanNativeFunctions();
    AptXmlNode::CleanNativeFunctions();
    AptCIH::CleanNativeFunctions();
    AptString::CleanNativeFunctions();
    AptError::CleanNativeFunctions(); // Added for Flash Player 7 / AS 2.0 support. (Release 17.0)
    AptMovieClipLoader::CleanNativeFunctions();
#if defined(APT_USE_STAGE_OBJECT)
    AptStage::CleanNativeFunctions();
#endif
#if APT_USE_UTILITY
    AptUtil::CleanNativeFunctions();
#endif
#if defined(APT_ALTERNATE_INPUT)
    AptAlternateInput::CleanNativeFunctions();
#endif
}

void AptCompleteAnimationAsyncLoad(AptFilePtr file, void *pData, void *pConstTable, void *pUserData)
{
    AptUpdateAutoLock lock;

    GetTargetSim()->GetLoader()->CompleteAnimationLoad(file, pData, pConstTable, pUserData);
}

void AptCompleteImageAsyncLoad(AptFilePtr file, AptAssetTexture texture, int width, int height, void *userData)
{
    AptUpdateAutoLock lock;

    GetTargetSim()->GetLoader()->CompleteImageLoad(file, texture, width, height, userData);
}

AptCIH *_AptGetAnimationAtLevel(int32_t nLevel)
{
    AptUpdateAutoLock lock;

    if (GetTargetSim() == NULL || (!GetTargetSim()->GetAnimationTarget() || !GetTargetSim()->GetAnimationTarget()->GetDisplayList()->pState))
    {
        return NULL;
    }

    AptCIH *pCur = GetTargetSim()->GetAnimationTarget()->GetDisplayList()->pState->GetFirstItem();

    while (pCur)
    {
        if (pCur->GetDepth() == nLevel)
        {
            return pCur;
        }
        pCur = pCur->GetDisplayListNext();
    }

    AptCIH *pEmpty = new AptCIH(NULL, NULL);

    pEmpty->SetDepth(nLevel);
    pEmpty->setGCRoot(1);
    GetTargetSim()->GetAnimationTarget()->GetDisplayList()->pState->insert(nLevel, pEmpty);

    return pEmpty;
}

void AptGetAnimationSize(int *pnWidth, int *pnHeight)
{
    AptUpdateAutoLock lock;

    APT_ASSERT(AptGetLib()->mbUpdateInitialized);
    if (!GetTarget()->GetAnimationTarget() ||
        !GetTarget()->GetAnimationTarget()->GetDisplayList()->pState)
    {
        return;
    }

    AptCIH *pNext = GetTarget()->GetAnimationTarget()->GetDisplayList()->pState->GetFirstItem();

    if (pNext != NULL && pNext->GetCharacterInst() && pNext->IsAnimationInst())
    {
        AptCharacterAnimationInst *pSprInst = pNext->GetAnimationInst();

        if (pnWidth)
            *pnWidth = pSprInst->GetCharacterConst()->animation.nWidth;

        if (pnHeight)
            *pnHeight = pSprInst->GetCharacterConst()->animation.nHeight;
    }
    else
    {
        if (pnWidth)
            *pnWidth = 0;
        if (pnHeight)
            *pnHeight = 0;
    }
}

void AptSetValidFocusButton()
{
#if defined APT_USE_BUTTONS
    AptUpdateAutoLock lock;

    GetTargetSim()->GetAnimationTarget()->SetValidFocusButton();
#else
    APT_ASSERT(0 && "DEFINE APT_USE_BUTTONS TO USE THIS");
#endif
}

bool _AptValidate(void)
{
    return true;
    /*  GetTarget()->GetAnimationTarget()->GetDisplayList()->validate(0);
        for (int i = 0; i < GetTarget()->GetAnimationTarget()->mBILCount; i++)
        {
    //      APT_ASSERT(GetTarget()->GetAnimationTarget()->aButtonInstanceList[i].pCIH->IsButtonInst());
        }
        return true;*/
}

void AptLoadAnimation(const char *szBaseName, const char *szTarget)
{
    AptUpdateAutoLock lock;

    APT_ASSERT(AptGetLib()->mbUpdateInitialized);

    AptNativeString sTarget(szTarget);
    APT_ASSERT(sTarget != AptNativeString("#import") && "Please use AptPreloadFile instead of a target of '#import' now.");

    // Each Animation can only have one background color. This value is reset
    // every time the game (or viewer) loads a new animation.
    AptGetLib()->mbBackgroundColorSet = false;

    // Animations always end in ".swf" so make sure the filename ends in .swf.
    AptNativeString fileName(szBaseName);
    if (!fileName.EndWithIgnoreCase(".swf"))
    {
        fileName += ".swf";
    }

    GetTargetSim()->GetLinker()->Load(fileName, sTarget);
}

AptFilePtr AptPreloadAnimation(const char *szBaseName)
{
    gAptSimThreadMutex.Lock();

    // Animations always end in ".swf" so make sure the filename ends in .swf.
    AptNativeString fileName(szBaseName);
    if (!fileName.EndWithIgnoreCase(".swf"))
    {
        fileName += ".swf";
    }

    AptFilePtr fp = GetTargetSim()->GetLoader()->Load(fileName, AptFileType_Animation);
    gAptSimThreadMutex.Unlock();
    return fp;
}

void AptAddCustomSavedInput(void *pBuffer, unsigned int nSize)
{
    AptUpdateAutoLock lock;

    if (AptGetLib()->mbSavedInputsEnabled)
    {
        AptSavedInputRecordCustom inputRecord;
        inputRecord.nTick      = AptGetLib()->mnCurTick;
        inputRecord.nInputType = INPUT_CUSTOM;
        APT_ASSERT(nSize <= 0xFFFF && "Saved Custom Event handler cannot handle a buffer of this size!");
        inputRecord.nInputBufferSize = static_cast<uint16_t>(nSize);

        AptGetUserFuncs().pfnDebugAddSavedInput(&inputRecord, sizeof(AptSavedInputRecordCustom));
        AptGetUserFuncs().pfnDebugAddSavedInput((AptSavedInputRecord *)pBuffer, nSize);
    }
}

void AptCancelPreloadedAnimation(const char *szBaseName)
{
    AptUpdateAutoLock lock;

    GetTargetSim()->GetLoader()->CancelPreloadedAnimation(AptNativeString(szBaseName));
}

bool AptIsFileLoaded(AptFilePtr file)
{
    gAptSimThreadMutex.Lock();
    bool bResult = file.pData != NULL && file->GetState() == AptFile::Resolved;
    gAptSimThreadMutex.Unlock();
    return bResult;
}

static void _addScreenGrabToSavedInputs()
{
    if (AptGetLib()->mbSavedInputsEnabled)
    {
        char szBuf[16];
        sprintf(szBuf, "%06d", AptGetLib()->mnCurTick);
        AptGetUserFuncs().pfnDebugSetScreenGrabPending(szBuf);

        AptSavedInputRecordInput inputRecord;
        inputRecord.nTick  = AptGetLib()->mnCurTick;
        inputRecord.nInput = SET_SCREENGRAB_INPUT();
#if defined(APT_SYSTEM_BIG_ENDIAN)
        inputRecord.nInput = ((inputRecord.nInput << 24) & 0xff000000);
#endif
        AptGetUserFuncs().pfnDebugAddSavedInput(&inputRecord, APT_SAVEINPUTS_RECORD_SIZE_SAVER);
    }
}

// - after 0.19.01 -added new parameter/s to do selective update/tick of animation levels. This will only work at highest root levels.
static int _tick(unsigned int nDeltaTime, AptAnimLevelE eAnimLevels)
{
    {
        int bAdvancedFrame = 0;

        AptAnimationTarget *pAnimTarget = GetTargetSim()->GetAnimationTarget();
        AptCIH *pRootItem               = GetTargetSim()->GetAnimationTarget()->GetDisplayList()->pState->GetFirstItem();

        if (!pRootItem->IsAnimationInst())
        {
            return bAdvancedFrame;
        }

        AptCharacterAnimationInst *pAnimInst = pRootItem->GetAnimationInst();

        nDeltaTime += pAnimInst->mnLeftoverTime;
        unsigned int nMillisecondsPerFrame = pAnimInst->GetCharacterConst()->animation.nMillisecondsPerFrame;

        while (nDeltaTime >= nMillisecondsPerFrame)
        {
            APT_ASSERT(gAptActionInterpreter.stack.GetSize() == 0);

            pAnimTarget->TickIntervalTimers(nMillisecondsPerFrame);
            pAnimTarget->GetDisplayList()->tick(eAnimLevels, true); // pass in the animLevels to do selective processing
            pAnimTarget->RunActions();
            pAnimTarget->ProcessInputs();

            GetTargetSim()->GetLinker()->Update();

            APT_ASSERT(gAptActionInterpreter.stack.GetSize() == 0);

            nDeltaTime -= nMillisecondsPerFrame;
            AptGetLib()->mnCurTick += nMillisecondsPerFrame;

            bAdvancedFrame = 1;

            if (!pAnimTarget->GetDisplayList()->pState->GetFirstItem()->IsAnimationInst())
            {
                return bAdvancedFrame;
            }

            if (AptGetLib()->mbSavedInputsEnabled)
                break;
        }

        // must check again, because it could have been unloaded
        if (GetTargetSim()->GetAnimationTarget()->GetDisplayList()->pState->GetFirstItem()->IsAnimationInst())
        {
            AptCharacterAnimationInst *pAnimInst = GetTargetSim()->GetAnimationTarget()->GetDisplayList()->pState->GetFirstItem()->GetAnimationInst();
            pAnimInst->mnLeftoverTime            = nDeltaTime;
        }

#if defined(APT_USE_BUTTONS)
        GetTargetSim()->GetAnimationTarget()->ClearBIL();
        // EATech 103381: Hit region ignores offset within movie clip
        // The AppendButtonToBIL() call in AptRenderItem.cpp was incorrectly placed and hence the precision to sense mouse over button was way off
        uint32_t (*pOldButtonValue)(AptCIH *pCIH, void *pVoid) = AptCIH::sCIHButtonProcessCb;
        AptCIH::sCIHButtonProcessCb                            = AptCIH::ProcessButtonState;
#endif

        uint32_t (*pOldValue1)(AptCIH *pCIH, void *pVoid) = AptCIH::sCIHProcessCb1;
        uint32_t (*pOldValue2)(AptCIH *pCIH, void *pVoid) = AptCIH::sCIHProcessCb2;
        AptCIH::sCIHProcessCb1                            = AptCIH::ProcessCustomControls;
        AptCIH::sCIHProcessCb2                            = AptCIH::ProcessMaskMatricies;

        AptDisplayList::GeneralisedProcess(GetTargetSim()->GetAnimationTarget()->GetDisplayList(), NULL, eAnimLevels, true);

        AptCIH::sCIHProcessCb1                            = pOldValue1;
        AptCIH::sCIHProcessCb2                            = pOldValue2;

#if defined(APT_USE_BUTTONS)
        AptCIH::sCIHButtonProcessCb = pOldButtonValue;
#endif

        return bAdvancedFrame;
    }
}

static void _playbackSavedInputs(unsigned int nDeltaTime, AptAnimLevelE eAnimLevels = AptAnimLevel_ALL)
{
    unsigned int nTargetTime = AptGetLib()->mSIPlayback.nCurTick + nDeltaTime;
    unsigned int nTick;

    for (; AptGetLib()->mSIPlayback.pSavedInputs;)
    {
        APT_ASSERT(AptGetLib()->mSIPlayback.nCurTick <= nTargetTime);
        if (AptGetLib()->mSIPlayback.nCurTick == nTargetTime)
            break;

        // make sure all the necessary animations are loaded before continuing
        if (AptGetLib()->mpSavedInputCheckpoints->CanContinueSavedInputs())
        {
            while ((nTick = GET_UNALIGNED_U32(AptGetLib()->mSIPlayback.pCurSavedInput)) <= AptGetLib()->mSIPlayback.nCurTick)
            {
                APT_ASSERT(nTick == AptGetLib()->mSIPlayback.nCurTick);
                AptGetLib()->mSIPlayback.pCurSavedInput += sizeof(unsigned int);

                switch (GET_TYPE_INPUT(AptGetLib()->mSIPlayback.pCurSavedInput))
                {
                case INPUT_MOUSE:
                case INPUT_KEYBOARD:
                {
                    unsigned int nInput = GET_UNALIGNED_U32(AptGetLib()->mSIPlayback.pCurSavedInput);
                    AptGetLib()->mSIPlayback.pCurSavedInput += 4;
#if defined(APT_SYSTEM_BIG_ENDIAN)
                    // Re-encode input
                    char *buf = (char *)&nInput;
                    for (int i = 0; i < 4; i += 2)
                    {
                        buf[i] ^= buf[i + 1];
                        buf[i + 1] ^= buf[i];
                        buf[i] ^= buf[i + 1];
                    }
                    short *buf2 = (short *)buf;
                    for (int i = 0; i < 2; i += 2)
                    {
                        buf2[i] ^= buf2[i + 1];
                        buf2[i + 1] ^= buf2[i];
                        buf2[i] ^= buf2[i + 1];
                    }
#endif
                    // AptGetUserFuncs().pfnDebugPrint("**-** %d Input: %x\n",nTick, nInput);
                    unsigned int nKeyCode = 0;
                    GET_ACTION_TYPE(nInput, nKeyCode);
                    switch (nKeyCode)
                    {
                    case AptInputType_LeftAnalogStick:
                    case AptInputType_RightAnalogStick:
                        GetTargetSim()->GetAnimationTarget()->AddAnalogInput(*((AptAnalogStickInfo *)AptGetLib()->mSIPlayback.pCurSavedInput));
                        AptGetLib()->mSIPlayback.pCurSavedInput += sizeof(AptAnalogStickInfo);
                        break;
                    default:
                        GetTargetSim()->GetAnimationTarget()->AddInput(nInput);
                        break;
                    }
                    break;
                }
                case INPUT_CHECKPOINT:
                {
                    APT_ASSERT(INPUT_IS_CHECKPOINT(AptGetLib()->mSIPlayback.pCurSavedInput));
                    char *szName = (char *)++AptGetLib()->mSIPlayback.pCurSavedInput;
                    AptGetLib()->mSIPlayback.pCurSavedInput += strlen(szName) + 1;

                    // AptGetUserFuncs().pfnDebugPrint("**-** %d Checkpoint: %s\n",nTick, szName);

#if defined(APT_SYSTEM_BIG_ENDIAN)
                    // Byte Align pCurSavedInputs
                    uintptr_t nTmp = (uintptr_t)AptGetLib()->mSIPlayback.pCurSavedInput;
                    if (nTmp & 0x03)
                    {
                        nTmp += 4;
                        nTmp &= ~0x03;
                    }
                    AptGetLib()->mSIPlayback.pCurSavedInput = (unsigned char *)nTmp;
#endif

                    AptNativeString fileName(szName);
#if (!APT_USE_FILTERS)
                    // special case: if the saved-input file wants us to set a checkpoint for
                    // the Flash filters bytecode and wait for it to be loaded, we can't--
                    // because it never will be loaded if APT_USE_FILTERS==0.
                    if (fileName == sFiltersPseudoFileName)
                    {
                        break;
                    }
#endif
                    AptGetLib()->mpSavedInputCheckpoints->Checkpoint(AptNativeString(szName));
                    break;
                }
                case INPUT_OTHER:
                {
                    switch (GET_OTHER_INPUT_TYPE(AptGetLib()->mSIPlayback.pCurSavedInput))
                    {
                    case INPUT_SCREENGRAB:
                    {
                        // AptGetUserFuncs().pfnDebugPrint("**-** %d ScreenGrab\n",nTick);

#if defined(APT_SYSTEM_BIG_ENDIAN)
                        AptGetLib()->mSIPlayback.pCurSavedInput += 4;
#else
                        AptGetLib()->mSIPlayback.pCurSavedInput++;
#endif
                        if (!AptGetLib()->mbSavedInputsEnabled) // if we're saving inputs, we want to do this at the normal saved input time
                        {
                            char szBuf[16];
                            sprintf(szBuf, "%06d", AptGetLib()->mSIPlayback.nCurTick);
                            AptGetUserFuncs().pfnDebugSetScreenGrabPending(szBuf);
                        }
                        break;
                    }
                    case INPUT_CUSTOM:
                    {
                        // AptGetUserFuncs().pfnDebugPrint("**-** %d Custom\n",nTick);
                        AptGetLib()->mSIPlayback.pCurSavedInput += 2;
                        uint32_t nCustomMessageBytes = GET_UNALIGNED_U16(AptGetLib()->mSIPlayback.pCurSavedInput);
                        AptGetLib()->mSIPlayback.pCurSavedInput += 2;

                        APT_ASSERT(AptGetUserFuncs().pfnCustomSavedInputHandler && "Custom Input handler found but not handler present!");

                        if (AptGetUserFuncs().pfnCustomSavedInputHandler)
                        {
                            AptGetUserFuncs().pfnCustomSavedInputHandler(AptGetLib()->mSIPlayback.pCurSavedInput, nCustomMessageBytes);
                        }

                        AptGetLib()->mSIPlayback.pCurSavedInput += nCustomMessageBytes;
                        break;
                    }
                    case INPUT_TRIGGER:
                    {
                        // added this extra type for trigger analog data. This is a special case because, for trigger button we just have normal
                        // keyup/key down messages, but now we also support analog values for triggers on xenon. These analog values are
                        // stored in static array called AptAnimationTarget::sgATriggers[AptInputController_NumControllers]
                        // We save those values to saved input file and replay them. There will be a corresponding AddInput for keyup/keydown
                        // along with these AddAnalogInput call. But it might be before this record or after this record, depending on how
                        // user has passed to Apt originally.
                        AptSavedInputRecordTriggers *pTriggerInfo = (AptSavedInputRecordTriggers *)(AptGetLib()->mSIPlayback.pCurSavedInput - sizeof(unsigned int));
                        GetTargetSim()->GetAnimationTarget()->AddAnalogInput(pTriggerInfo->mAnalogTriggerInfo);
                        AptGetLib()->mSIPlayback.pCurSavedInput += (sizeof(AptSavedInputRecordTriggers) - sizeof(unsigned int));
                        break;
                    }
                    default:
                        APT_ASSERT(false && "Unknown Saved Input Type Reached!!!");
                    }
                    break;
                }
                default:
                    // do nothing
                    APT_ASSERT(false && "Unknown Saved Input Type Reached!!!")
                    break;
                }

                APT_ASSERT((AptGetLib()->mSIPlayback.pCurSavedInput - AptGetLib()->mSIPlayback.pSavedInputs) <= AptGetLib()->mSIPlayback.nInputFileSize);
            }
        }
        else
        {
            // $HACK!!! We need to get this outta here with a revamp of the saved inputs. The problem is that interval timers are not tracked as it stands
            // so timing issues can (and did) arise.
            // If we couldn't tick because files weren't loaded, at least make sure we have played the interval timer (in case that is what loaded the file we are waiting on)
            nTick = GET_UNALIGNED_U32(AptGetLib()->mSIPlayback.pCurSavedInput);

            int timeToSkip = nTick - AptGetLib()->mSIPlayback.nCurTick;
            timeToSkip--;

            if (timeToSkip > 0)
            {
                GetTargetSim()->GetAnimationTarget()->TickIntervalTimers(16);
            }
        }

        if ((AptGetLib()->mSIPlayback.pCurSavedInput - AptGetLib()->mSIPlayback.pSavedInputs) >= AptGetLib()->mSIPlayback.nInputFileSize)
        {
            APT_DEBUGPRINT(APT_DEBUG_MSG_DEFAULT_LVL, "Playback of inputs completed.\n");
            AptGetLib()->mSIPlayback.pSavedInputs   = NULL; // we aren't responsible for this memory
            AptGetLib()->mSIPlayback.pCurSavedInput = NULL;

            if (AptGetUserFuncs().pfnPlaySavedInputsDone != NULL)
            {
                AptGetUserFuncs().pfnPlaySavedInputsDone(true, NULL);
            }

            if (AptGetLib()->mbSavedInputsEnabled)
            {
                APT_DEBUGPRINT(APT_DEBUG_MSG_DEFAULT_LVL, "  Turning off saved inputs too.\n");
                AptGetLib()->mbSavedInputsEnabled = 0;
            }
            break;
        }

        // make sure all the necessary animations are loaded before continuing

        int bCorrectAnimsLoaded = AptGetLib()->mpSavedInputCheckpoints->CanContinueSavedInputs();

        if (bCorrectAnimsLoaded && GetTargetSim()->GetAnimationTarget()->GetDisplayList()->pState->GetFirstItem() && GetTargetSim()->GetAnimationTarget()->GetDisplayList()->pState->GetFirstItem()->IsAnimationInst())
        {
            // Shouldn't be queuing frames in playback, but passing in an arbitrary number so that it has one.
            if (_tick(1, eAnimLevels))
            {
                _addScreenGrabToSavedInputs();
            }
            AptGetLib()->mSIPlayback.nCurTick++;
        }
        else
        {
            GetTargetSim()->GetLinker()->Update();
            break;
        }
    }
}

// - after 0.19.01 -added new parameter/s to do selective update/tick of animation levels. This will only work at highest root levels.
static void _AptInternalUpdate(unsigned int nDeltaTime, AptAnimLevelE eAnimLevels)
{

    APT_ASSERT(AptGetLib()->mbUpdateInitialized);

    // Grab a reference to all loaded files before we start doing the update
    // This is so that if a block of code, or a user callback unloads the animation
    // we're currently executing, we don't be freeing the data we're using
    // Don't make it a Vector, as it previously was, otherwise we're thrashing dynamic memory
    // while doing an Update.
    // The sole purpose of this is to increment the reference counts until the end of the function.
    AptFilePtr aLoadedFiles[APT_MAX_FILES_LOADED];
    GetTargetSim()->GetLoader()->GetFileVector(aLoadedFiles, APT_ARRAYSIZE(aLoadedFiles));

    if (AptGetLib()->mSIPlayback.pSavedInputs)
    {
        _playbackSavedInputs(nDeltaTime, eAnimLevels);
    }

    else
    {
        if (GetTargetSim()->GetAnimationTarget()->GetDisplayList()->pState->GetFirstItem() != NULL && GetTargetSim()->GetAnimationTarget()->GetDisplayList()->pState->GetFirstItem()->IsAnimationInst())
        {
            if (_tick(nDeltaTime, eAnimLevels))
            {
                _addScreenGrabToSavedInputs();
            }
        }
        else
        {
            GetTargetSim()->GetLinker()->Update();

            // If there isn't anything to do, inc the tick. This gets around timing issues in loading the first swf and
            // it loading other files. It is a Hack for now until we get time to investigate a better solution. It should
            // pose no real danger.
            AptGetLib()->mnCurTick++;

            // discard all inputs if we didn't have anywhere to send them.
            // this is slightly wrong, because we shouldn't discard inputs until the next "frame", but since nothings loaded we
            // can't tell when the next frame is. If the user is always calling AptUpdate/AptRender, and just leaving an empty
            // movie always loaded, and passing inputs to Apt, then they expect that input buffers shouldn't overflow
            // (reasonably), so we do this here.
            GetTargetSim()->GetAnimationTarget()->SetQueuedInputsSize(0);
        }
    }

    //  Try to release all the registered objects
    AptGetLib()->mpValuesToRelease->ReleaseValues();

    //  If needed garbage collect
    if (AptGetLib()->mbGarbageCollectThisFrame)
    {
        AptGC::CleanUnreachable();
        AptGetLib()->mbGarbageCollectThisFrame = false;
    }

    APT_ASSERT(gAptActionInterpreter.stack.GetSize() == 0);

}

extern void AptDecoupleTreeTraversal(const AptCIH *pCur, int32_t nCurrRenderTick, AptRenderingContext *pRenderingContext, AptMaskRenderOperation eMaskOperation, int nMaskDepth, AptAnimLevelE eAnimLevels, bool bUseDepthCompare);

// - after 0.19.01 -added new parameter/s to do selective display of animation levels. This will only work at highest root levels.
static void _AptInternalRender(AptAnimLevelE eAnimLevels)
{
    APT_ASSERT(AptGetLib()->mbRenderInitialized);

    AptRenderItem::suCurrentRenderAnimLevel = 0;

    if (!GetTargetRender()->GetAnimationTarget())
        return;

    AptFilePtr aLoadedFiles[APT_MAX_FILES_LOADED];
    if (GetTargetRender()->GetLoader())
        GetTargetRender()->GetLoader()->GetFileVector(aLoadedFiles, APT_ARRAYSIZE(aLoadedFiles));

    // push unit
    AptMath::ClipStackPushUnit();

    // start render at mask level 0.
    {
        const AptCIH *pCur = GetTargetRender()->GetAnimationTarget()->GetDisplayList()->pState->GetFirstItem();
        AptDecoupleTreeTraversal(pCur, AptGetLib()->mnCurrRenderTick, AptGetLib()->mpRenderingContext, AptMaskRenderOperation_None, 0, eAnimLevels, true);
    }

    // pop unit
    AptMath::ClipStackPop();

    APT_ASSERT(AptMath::ClipStackIsEmpty());
}

// - after 0.19.01 -added new parameter/s to do selective update/tick of animation levels. This will only work at highest root levels.
void AptUpdate(unsigned int nDeltaTime, AptAnimLevelE eAnimLevels)
{
    AptUpdateAutoLock lock;

#ifndef APT_SINGLE_THREADED
    gAptSimThreadId = APT_THREAD_NAMESPACE::GetThreadId();
#endif

    // use fast stack, if possible
    if (AptOptIsEnabled(APT_OPT_SPRSTACK))
    {
        AptFastStack::Begin(2, 16 * 1024);
    }

    // run the update
    _AptInternalUpdate(nDeltaTime, eAnimLevels);

    if (AptFastStack::IsActive())
    {
        // shutdown fast stack
        APT_ASSERT(AptOptIsEnabled(APT_OPT_SPRSTACK));
        AptFastStack::End();
    }

}

void AptUpdateRender(AptAnimLevelE eAnimLevels)
{
    AptUpdateAutoLock lock;

#ifndef APT_SINGLE_THREADED
    // write to render list
    if (AptRenderList::Open())
    {
        _AptInternalRender(eAnimLevels);
        AptRenderList::Close();
    }
#endif

}

// - after 0.19.01 -added new parameter/s to do selective display of animation levels. This will only work at highest root levels.
void AptRender(uint32_t nDeltaTimeMS, AptAnimLevelE eAnimLevels)
{
#ifndef APT_SINGLE_THREADED
    AptRenderList::StartRender();
    AptRenderList::Render(eAnimLevels);
    AptFreeOutstandingCustomControls();
    AptRenderList::EndRender();
#else
    _AptInternalRender(eAnimLevels);
    AptFreeOutstandingCustomControls();
#endif
}

void AptSetMousePosition(int nX, int nY)
{
#if defined(APT_USE_MOUSE)
    if (!AptGetLib()->mbUpdateInitialized)
    {
        APT_DEBUGPRINT(APT_DEBUG_MSG_DEFAULT_LVL, "WARNING: trying to set mouse position when Apt not initalized\n");
        return;
    }

    if (AptGetLib()->mSIPlayback.pSavedInputs)
        return;

    AptUpdateAutoLock lock;
    if (GetTargetSim()->GetAnimationTarget())
        GetTargetSim()->GetAnimationTarget()->AddInput(nX, nY);
#else
    APT_ASSERT(false && "MOUSE NOT SUPPORTED, define APT_USE_MOUSE to use the mouse");
#endif
}

#if defined(APT_ALTERNATE_INPUT)
void AptAddAlternateInput(const char *szEvent, AptValue *pValue)
{
    if (!AptGetLib()->mbUpdateInitialized)
    {
        APT_DEBUGPRINT(APT_DEBUG_MSG_DEFAULT_LVL, "WARNING: trying to add input when Apt not initalized\n");
        return;
    }

    AptUpdateAutoLock lock;
    if (GetTargetSim()->GetAnimationTarget())
    {
        AptAltInput input;
        input.sEvent = szEvent;
        input.pValue = pValue;
        GetTargetSim()->GetAnimationTarget()->AddAlternateInput(&input);
    }
}
#endif

void AptAddToInputQueue(AptInputType eInput, AptInputState eState, AptInputController eController)
{
    if (!AptGetLib()->mbUpdateInitialized)
    {
        APT_DEBUGPRINT(APT_DEBUG_MSG_DEFAULT_LVL, "WARNING: trying to add input when Apt not initalized\n");
        return;
    }

    if (AptGetLib()->mSIPlayback.pSavedInputs)
        return;

    AptUpdateAutoLock lock;
    if (GetTargetSim()->GetAnimationTarget())
    {
        GetTargetSim()->GetAnimationTarget()->AddInput(eInput, eState, eController);
    }
}

// added in 0.13.00
void AptAddToInputAnalogQueue(AptAnalogStickInfo pAnalogInput)
{
    if (!AptGetLib()->mbUpdateInitialized)
    {
        APT_DEBUGPRINT(APT_DEBUG_MSG_DEFAULT_LVL, "WARNING: trying to add input when Apt not initalized\n");
        return;
    }

    if (AptGetLib()->mSIPlayback.pSavedInputs)
        return;

    AptUpdateAutoLock lock;
    if (GetTargetSim()->GetAnimationTarget())
        GetTargetSim()->GetAnimationTarget()->AddAnalogInput(pAnalogInput);
}

void AptAddToInputGestureQueue(AptGestureInfo pGestureInput)
{
    if (!AptGetLib()->mbUpdateInitialized)
    {
        APT_DEBUGPRINT(APT_DEBUG_MSG_DEFAULT_LVL, "WARNING: trying to add input when Apt not initalized\n");
        return;
    }

    if (AptGetLib()->mSIPlayback.pSavedInputs)
        return;

    AptUpdateAutoLock lock;
    if (GetTargetSim()->GetAnimationTarget())
        GetTargetSim()->GetAnimationTarget()->AddGestureInput(pGestureInput);
}

void AptSetInternalVariable(const char *szName, const char *szValue)
{
    AptUpdateAutoLock lock;

    AptString *pStr = AptString::Create();
    APT_INC(pStr);
    pStr->cpy(szValue);
    AptNativeString strName(szName);
    gAptActionInterpreter.setVariable(_AptGetAnimationAtLevel(0), 0, &strName, pStr);
    APT_DEC(pStr);
}

void AptGetInternalVariable(const char *szName, char *szValue)
{
    AptUpdateAutoLock lock;

    AptNativeString strName(szName);
    AptValue *pVal = gAptActionInterpreter.getVariable(_AptGetAnimationAtLevel(0), 0, &strName);
    APT_INC(pVal);
    pVal->toString(szValue);
    APT_DEC(pVal);
}

void AptGetInternalVariable(const AptNativeString *szName, char *szValue)
{
    AptUpdateAutoLock lock;

    AptValue *pVal = gAptActionInterpreter.getVariable(_AptGetAnimationAtLevel(0), 0, szName);
    APT_INC(pVal);
    pVal->toString(szValue);
    APT_DEC(pVal);
}

// Factored this out to allow calling a member function on pThis
static void AptCallMemberFunctionInternal(const char *szName, char *szReturnValue, AptValue *pThis, int nNumParams, AptValue **apArguments)
{
    if (!pThis)
    {
        pThis = _AptGetAnimationAtLevel(0);
    }
    APT_ASSERT(pThis);

    // BTurkelson 03/2010 - Only do anything if the object whose member function we're calling is actually there.
    if (pThis->getIsDefined())
    {
        for (int i = nNumParams - 1; i >= 0; i--)
        {
            gAptActionInterpreter.stackPush(apArguments[i]);
        }

        AptNativeString strName(szName);
        AptValue *pFuncValue = gAptActionInterpreter.getVariable(pThis, 0, &strName, 1);

        // Save function state information before execution.
        void *pFuncState = AptScriptFunctionBase::PushStaticData();

#ifdef APT_DEBUGGER_ENABLE
        bool isScriptActionFlag = AptDebugger::GetInstance()->PushCallStack(pFuncValue, szName, reinterpret_cast<unsigned char *const>(AptDebugger::INVALID_OFFSET), pThis);
#endif

        // Fix for one of the "Super Bugs" from BTurkelson
        // it was pulling member var values from pThis's prototype
        // instead of pThis itself
        gAptActionInterpreter.withStack.push(pThis);

        // See comment about FindSuperImplementor in  AptActionInterpreter::_FunctionAptActionCallMethod.
        // This really should be refactored. AptCallMemberFunctionInternal should behave more like _FunctionAptActionCallMethod.
        AptValue *pOverrideSuper = NULL;
        if (!strName.Equal(*StringPool::GetString(SC_super)))
        {
            // search up the __proto__ chain until we find a prototype that implements strName.
            AptValue *pProto = AptActionInterpreter::FindSuperImplementor(pThis, &strName);
            if (pProto)
            {
                // pProto implements the given method.  Set pOverrideSuper
                // to the superclass of pProto.
                pOverrideSuper = AptActionInterpreter::GetNextProto(pProto);
            }
        }
        gAptActionInterpreter.callFunction(pThis, pFuncValue, nNumParams, NULL, pOverrideSuper);

        // Fix for one of the "Super Bugs" from BTurkelson
        // it was pulling member var values from pThis's prototype
        // instead of pThis itself
        gAptActionInterpreter.withStack.pop();

#ifdef APT_DEBUGGER_ENABLE
        if (isScriptActionFlag)
        {
            AptDebugger::GetInstance()->PopCallStack(pFuncValue);
        }
#endif

        // And aaawaaay we go.
        AptScriptFunctionBase::PopStaticData(pFuncState);


        if (szReturnValue)
        {
            AptValue *pVal = gAptActionInterpreter.stackAt(0);
            pVal->toString(szReturnValue);
        }
        gAptActionInterpreter.stackPop();
    }
}

static void AptCallFunctionInternal(const char *szName, char *szReturnValue, const char *szThisObject, int nNumParams, AptValue **apArguments)
{
    AptValue *pThis = 0;

    if (szThisObject != 0)
    {
        AptNativeString strObject(szThisObject);
        pThis = gAptActionInterpreter.getVariable(_AptGetAnimationAtLevel(0), 0, &strObject, 1);
    }

    AptCallMemberFunctionInternal(szName, szReturnValue, pThis, nNumParams, apArguments);
}

void AptCallFunctionV(const char *szName, char *szReturnValue, const char *szThisObject, int nNumParams, va_list &varargs)
{
    AptUpdateAutoLock lock;

    AptValue *apStrings[32];
    APT_ASSERT(nNumParams >= 0 && nNumParams < APT_ARRAYSIZE(apStrings));

    for (int i = 0; i < nNumParams; i++)
    {
        // lint --e(119) PcLint warning Error 119: Too many arguments (1) for prototype 'AptString::Create(void)' (on ps3)
        apStrings[i] = AptString::Create(va_arg(varargs, char *));
    }

    AptCallFunctionInternal(szName, szReturnValue, szThisObject, nNumParams, apStrings);
}

void AptCallFunction(const char *szName, char *szReturnValue, const char *szThisObject, int nNumParams, ...)
{
    AptUpdateAutoLock lock;

    va_list varargs;
    va_start(varargs, nNumParams);
    AptCallFunctionV(szName, szReturnValue, szThisObject, nNumParams, varargs);
    va_end(varargs);
}

void AptCallFunctionVOpti(const char *szName, char *szReturnValue, const char *szThisObject, int nNumParams, va_list &varargs)
{
    AptUpdateAutoLock lock;

    AptValue *apValues[32];
    APT_ASSERT(nNumParams >= 0 && nNumParams < APT_ARRAYSIZE(apValues));

    for (int i = 0; i < nNumParams; i++)
    {
        // lint --e(64) PcLint warning Error 64: Type mismatch (assignment) (AptValue * = int)
        apValues[i] = va_arg(varargs, AptValue *);
    }

    AptCallFunctionInternal(szName, szReturnValue, szThisObject, nNumParams, apValues);
}

void AptCallFunctionOpti(const char *szName, char *szReturnValue, const char *szThisObject, int nNumParams, ...)
{
    AptUpdateAutoLock lock;

    va_list varargs;
    va_start(varargs, nNumParams);
    AptCallFunctionVOpti(szName, szReturnValue, szThisObject, nNumParams, varargs);
    va_end(varargs);
}

void AptCallFunctionAOpti(const char *szName, char *szReturnValue, const char *szThisObject, int nNumParams, AptValue **apValues)
{
    AptUpdateAutoLock lock;
    AptCallFunctionInternal(szName, szReturnValue, szThisObject, nNumParams, apValues);
}

void AptCallMemberFunctionV(const char *szName, char *szReturnValue, AptValue *pThis, int nNumParams, va_list &varargs)
{
    AptUpdateAutoLock lock;

    AptValue *apValues[32];
    APT_ASSERT(nNumParams >= 0 && nNumParams < APT_ARRAYSIZE(apValues));

    for (int i = 0; i < nNumParams; i++)
    {
        // lint --e(64) PcLint warning Error 64: Type mismatch (assignment) (AptValue * = int)
        apValues[i] = va_arg(varargs, AptValue *);
    }

    AptCallMemberFunctionInternal(szName, szReturnValue, pThis, nNumParams, apValues);
}

void AptCallMemberFunction(const char *szName, char *szReturnValue, AptValue *pThis, int nNumParams, ...)
{
    AptAssertIsSimulationThread();

    va_list varargs;
    va_start(varargs, nNumParams);
    AptCallMemberFunctionV(szName, szReturnValue, pThis, nNumParams, varargs);
    va_end(varargs);
}

static AptValue *AptCallFunctionObjectInternal(AptValue *pFuncValue,
                                               int numParams,
                                               AptValue **values)
{
    for (int i = numParams - 1; i >= 0; i--)
    {
        gAptActionInterpreter.stackPush(values[i]);
    }


    void *pFuncState = AptScriptFunctionBase::PushStaticData();
    gAptActionInterpreter.callFunction(gpUndefinedValue, pFuncValue, numParams, NULL, NULL);
    AptScriptFunctionBase::PopStaticData(pFuncState);


    return gAptActionInterpreter.stackGetPop();
}

static AptValue *AptCallFunctionObjectV(AptValue *pFuncValue, int nNumParams, va_list &varargs)
{
    AptUpdateAutoLock lock;

    AptValue *apValues[32];
    APT_ASSERT(nNumParams >= 0 && nNumParams < APT_ARRAYSIZE(apValues));

    for (int i = 0; i < nNumParams; i++)
    {
        apValues[i] = va_arg(varargs, AptValue *);
    }

    return AptCallFunctionObjectInternal(pFuncValue, nNumParams, apValues);
}

AptValue *AptCallFunctionObject(AptValue *pFuncValue, int nNumParams, ...)
{
    AptUpdateAutoLock lock;

    va_list varargs;
    va_start(varargs, nNumParams);
    AptValue *retVal = AptCallFunctionObjectV(pFuncValue, nNumParams, varargs);
    va_end(varargs);
    return retVal;
}

/******************************************************************************/
/**
    @brief  Call a function object you already have a hold on

    @param  funcValue Function to call
    @param  numParams Number of parameters being passed
    @param  params    Array of those parameters

    @return           - Return value of the function
*/
/******************************************************************************/
AptValue *AptCallFunctionObjectA(AptValue *funcValue, int numParams, AptValue **params)
{
    AptUpdateAutoLock lock;
    return AptCallFunctionObjectInternal(funcValue, numParams, params);
}

bool AptSetInputRoot(const char *szRoot)
{
    AptUpdateAutoLock lock;

    // if szRoot == NULL, means want to disable the input masking
    if (szRoot == 0)
    {
        GetTargetSim()->GetAnimationTarget()->SetInputMask(0);
        return true;
    }

    // try to find the CIH which szRoot is specifying
    // if can't find just object, just report operation fail
    AptValue *pTarget;
    AptNativeString strRoot(szRoot);
    pTarget = gAptActionInterpreter.getVariable(_AptGetAnimationAtLevel(0), NULL, &strRoot);
    if (pTarget == 0 || !(pTarget->isCIH()))
    {
        return false;
    }

    AptCIH *pCIH = 0;
    pCIH         = pTarget->c_cih();

    // set the mask
    GetTargetSim()->GetAnimationTarget()->SetInputMask(pCIH);

    return true;
}

int AptDebugGetCurrentFrame(int nLevel)
{

    AptCIH *pCIH = _AptGetAnimationAtLevel(static_cast<int32_t>(nLevel));
    if (pCIH && pCIH->IsAnimationInst())
    {
        return pCIH->GetAnimationInst()->mnFrame;
        // if (pCIH->IsAnimationInst())
        //{
        //     return pCIH->GetAnimationInst()->mnFrame;
        // }
        // else
        //     return 0 ;
    }
    else
    {
        return -1;
    }
}

int AptDebugGetNumFrames(int nLevel)
{

    AptCIH *pCIH = _AptGetAnimationAtLevel(static_cast<int32_t>(nLevel));
    if (pCIH)
    {
        return pCIH->GetAnimationInst()->GetCharacterConst()->sprite.movie.nFrames;
    }
    else
    {
        return 0;
    }
}

int AptDebugIsPlaying(int nLevel)
{

    AptCIH *pCIH = _AptGetAnimationAtLevel(static_cast<int32_t>(nLevel));
    if (pCIH && pCIH->IsAnimationInst())
    {
        return pCIH->GetIsPlaying();
    }
    else
    {
        return -1;
    }
}

void AptDebugEnableSavedInputs(int bEnabled)
{

    AptGetLib()->mbSavedInputsEnabled = bEnabled;
    AptGetLib()->mnCurTick            = 0;
}

void AptDebugPlaySavedInputs(AptSavedInputRecord *aSavedInputs, int nSavedInputsFileSize)
{
    // commented because threadid is not yet set. It gets set from AptInitialize.
    //
    AptGetLib()->mSIPlayback.pSavedInputs   = (unsigned char *)aSavedInputs;
    AptGetLib()->mSIPlayback.pCurSavedInput = AptGetLib()->mSIPlayback.pSavedInputs;
    AptGetLib()->mSIPlayback.nInputFileSize = nSavedInputsFileSize;
    AptGetLib()->mSIPlayback.nCurTick       = 0;
}

void AptDebugStopSavedInputs(void)
{
    APT_DEBUGPRINT(APT_DEBUG_MSG_DEFAULT_LVL, "Playing of saved inputs stopped before completion\n");
    AptGetLib()->mSIPlayback.pSavedInputs = NULL;
    AptGetLib()->mbSavedInputsEnabled     = 0;
    AptGetUserFuncs().pfnPlaySavedInputsDone(true, "Playback Stopped.");
}

/*F*************************************************************************************************

    Apt_atoff

    Description:

    Inputs:
        const char *    _s              -

    Outputs:
        int                             -

    Notes:
        None.

    History:
        Ver.    Description
        1.0     06/17/00 Ported from LLAPI so that we have don't do double conversions on the ps2.

*************************************************************************************************F*/

#define __SPEED__
float Apt_atoff(const char *_s)
{
#if defined(APT_PLATFORM_PLAYSTATION2) || defined(APT_PLATFORM_PSP)
    float val, holder;
    int floatsign, expsign, exp, i;
    const char *s = _s;
#ifdef __SPEED__
    static const float pow10[] =
        {1.0e-1, 1.0e-2, 1.0e-3, 1.0e-4, 1.0e-5, 1.0e-6, 1.0e-7, 1.0e-8, 1.0e-9,
         1.0e-10, 1.0e-11, 1.0e-12, 1.0e-13, 1.0e-14, 1.0e-15, 1.0e-16, 1.0e-17};
    const float *dp;
#endif // __SPEED__

    // scan past blanks
    while (isspace(*s))
    {
        ++s;
    }

    floatsign = 1; // assume positive

    if ((i = *s) == '+')
    {
        ++s;
    }
    else if (i == '-')
    {
        ++s;
        floatsign = 0;
    }

    for (val = 0; isdigit(i = *s); ++s)
    {
        val = val * 10 + (i - '0');
    }

    if (*s == '.')
    {
        ++s;
#ifdef __SPEED__
        for (dp = pow10, holder = *dp; isdigit(i = *s) && dp < &pow10[sizeof pow10 / sizeof pow10[0]]; ++s, holder = *++dp)
#else
        for (holder = 0.1f; isdigit(i = *s); ++s, holder *= 0.1f)
#endif // __SPEED__
        {

            val = val + holder * (i - '0');
        }
    }

    while (isdigit(*s)) // scan past insignificant digits
    {
        ++s;
    }

    if (tolower(*s) == 'e')
    {
        expsign = 1;
        ++s;
        if (!isdigit(i = *s))
        {
            if (i == '-')
            {
                expsign = -1;
            }

            ++s;
        }

        for (exp = 0; isdigit(i = *s); ++s)
        {
            exp = 10 * exp + (i - '0');
        }

        if (expsign == -1)
        {
            exp = exp - (2 * exp);
        }

        while (exp < 0)
        {
            val = val / 10;
            ++exp;
        }

        while (exp > 0)
        {
            val = val * 10;
            --exp;
        }
    }

    return floatsign ? val : -val;
#else
    return (float)(atof(_s));
#endif
}

void AptDeallocAllAssetString()
{

    if (GetTargetSim()->GetAnimationTarget())
    {
        GetTargetSim()->GetAnimationTarget()->GetDisplayList()->DeallocAssetStringRecursive();
    }
}

void AptPrintAllDynamicText(AptAnimLevelE eAnimLevels)
{

    if (GetTargetSim()->GetAnimationTarget())
    {
        uint32_t (*pOldValue)(AptCIH *pCIH, void *pVoid) = AptCIH::sCIHProcessCb;
        bool bOldEarlyReturn                             = AptCIH::bEarlyReturn;
        AptCIH::bEarlyReturn                             = false;
        AptCIH::sCIHProcessCb                            = AptCIH::ProcessTextInstPrint;
        AptDisplayList::GeneralisedProcess(GetTargetSim()->GetAnimationTarget()->GetDisplayList(), NULL, eAnimLevels, true);
        AptCIH::sCIHProcessCb = pOldValue;
        AptCIH::bEarlyReturn  = bOldEarlyReturn;
    }
}

void AptPrintMovieclipTree(AptAnimLevelE eAnimLevels)
{

    if (GetTargetSim()->GetAnimationTarget())
    {
        APT_DEBUGPRINT(APT_DEBUG_MSG_DEFAULT_LVL, "<?xml version=\"1.0\" encoding=\"utf-8\" ?>\n");
#if defined(APT_GATHER_MOVIECLIP_METRICS)
        APT_DEBUGPRINT(APT_DEBUG_MSG_DEFAULT_LVL, "<Apt_MovieclipTree Animations=\"%d\" Movieclips=\"%d\" DynamicText=\"%d\" Shapes=\"%d\" CustomControls=\"%d\" StaticText=\"%d\" Buttons=\"%d\">\n",
                       gAptMovieclipInformation.nAnimations,
                       gAptMovieclipInformation.nMovieClips,
                       gAptMovieclipInformation.nDynamicText,
                       gAptMovieclipInformation.nShapes,
                       gAptMovieclipInformation.nCustomControls,
                       gAptMovieclipInformation.nStaticText,
                       gAptMovieclipInformation.nButtons);
#else
        APT_DEBUGPRINT(APT_DEBUG_MSG_DEFAULT_LVL, "<Apt_MovieclipTree warning=\"Please define APT_GATHER_MOVIECLIP_METRICS in AptDefine.h to get correct number of movieclips printed here, tree below is good though\">\n");
#endif
        APT_DEBUGPRINT(APT_DEBUG_MSG_DEFAULT_LVL, "<Message Note=\"Please look in AptCharacter.h file to see the enums for movieclip types\"/>\n");
        uint32_t (*pOldValue)(AptCIH *pCIH, void *pVoid) = AptCIH::sCIHProcessCb;
        bool bOldEarlyReturn                             = AptCIH::bEarlyReturn;
        AptCIH::bEarlyReturn                             = false;
        AptCIH::sCIHProcessCb                            = AptCIH::PrintMovieclipTree;
        AptDisplayList::GeneralisedProcess(GetTargetSim()->GetAnimationTarget()->GetDisplayList(), NULL, eAnimLevels, true);
        AptCIH::sCIHProcessCb = pOldValue;
        AptCIH::bEarlyReturn  = bOldEarlyReturn;
        APT_DEBUGPRINT(APT_DEBUG_MSG_DEFAULT_LVL, "</TreeDepth0>\n");
        APT_DEBUGPRINT(APT_DEBUG_MSG_DEFAULT_LVL, "</Apt_MovieclipTree>\n");
    }
}

void AptDumpDisplayList(const char *szFileName)
{
    // Grab the root display object and print out its tree to the specified file
    if (GetTargetSim()->GetAnimationTarget())
    {
        GetTargetSim()->GetAnimationTarget()->GetDisplayList()->dumpDisplayList(szFileName);
    }
}

// This is provided only in debug mode right now, just to keep static size small, but can be turned on in release mode.
void AptPrintLoadedSWFFiles(bool bPrintImports, bool bPrintExports)
{
#if defined(APT_DEBUG) || defined(APT_PRINT_LOADED_SWFS)

    if (GetTargetSim()->GetLinker())
    {
        APT_DEBUGPRINT(APT_DEBUG_MSG_DEFAULT_LVL, "<?xml version=\"1.0\" encoding=\"utf-8\" ?>\n");
        APT_DEBUGPRINT(APT_DEBUG_MSG_DEFAULT_LVL, "<Apt_SWF_Loaded>\n");
        APT_DEBUGPRINT(APT_DEBUG_MSG_DEFAULT_LVL, "<Message Note=\"FileState enums are 0:Invalid, 1:Queued, 2:WaitingForData, 3:WaitingForImports, 4:Resolved, 5:Zombie, 6:Unloaded\"/>\n");
        GetTargetSim()->GetLinker()->PrintLoadedFiles(bPrintImports, bPrintExports);
        APT_DEBUGPRINT(APT_DEBUG_MSG_DEFAULT_LVL, "</Apt_SWF_Loaded>\n");
    }
#endif
}

/**
    @brief this will change the bSkipTraceBytecodes variable from AptActionInterpreter
    Look in Apt.h for more help.
*/
void AptSetSkipTracesFlag(bool bSkipTraces)
{
    gAptActionInterpreter.mbSkipTraceBytecodes = bSkipTraces;
}


void AptSetXMLImplementor(IAptXmlImpl *pIAptXmlImpl)
{

    if (pIAptXmlImpl)
    {
        AptGetLib()->mpAptXmlImpl = pIAptXmlImpl;
    }
}

// given by maxis guys. use of this function can be at following places.
// -At the first line of AptAux..._LoadAnimation()
// -Each of our flash movies has an Init() function that is always the first function called when the movie is loaded.
// -At the end of this function there is a call to "extern._r".  This _r message tells us the movie is "ready".  When the
//  engine side recieves this ready message, this function is called.
// -We also added an "extern._f" message that flash could call when it wanted to flush the input queue.
// Basically we call this function when the load of a movie starts, and when we consider the load of a movie to be finished.

void AptFlushInputQueue(void)
{

    APT_ASSERT(AptGetLib()->mbUpdateInitialized);

    if (AptGetLib()->mSIPlayback.pSavedInputs)
        return;

    if (!GetTargetSim()->GetAnimationTarget())
        return;

    GetTargetSim()->GetAnimationTarget()->SetQueuedInputsSize(0);
#if defined(APT_ALTERNATE_INPUT)
    GetTargetSim()->GetAnimationTarget()->SetAltQueuedInputsSize(0);
#endif
}

// Mouse Over Button detection - not tested fully
bool AptIsMouseOverButton(void)
{

#if defined(APT_USE_MOUSE) && defined(APT_USE_BUTTONS)
    return GetTargetSim()->GetAnimationTarget()->GetFocusButton() != 0;
#else
    APT_ASSERT(false && "MOUSE NOT SUPPORTED, define APT_USE_MOUSE to use the mouse");
    return false;
#endif
}

void AptRegisterExtension(AptExtObject *pExtObject)
{
    AptUpdateAutoLock lock;

    APT_ASSERT(pExtObject != NULL);

    pExtObject->Initialize();

    AptExtObjectRegistry::Register(pExtObject);
}

void AptUnRegisterExtension(AptExtObject *pExtObject)
{
    AptUpdateAutoLock lock;

    APT_ASSERT(pExtObject != NULL);

    AptExtObjectRegistry::UnRegister(pExtObject->GetName());
}

void AptStackPush(AptValue *const pValue)
{
    gAptActionInterpreter.stackPush(pValue);
}

void AptStackPop(const int32_t nItems)
{
    gAptActionInterpreter.stackPop(nItems);
}

void AptUnRegisterExtension(const char *pExtObjectName)
{
    AptUpdateAutoLock lock;

    APT_ASSERT(pExtObjectName != NULL);

    AptExtObjectRegistry::UnRegister(pExtObjectName);
}

void AptBreakAfterAssert(int bBreakAfterAssert)
{
    if (bBreakAfterAssert != 0)
    {
        APT_DEBUGPRINT(APT_DEBUG_MSG_DEFAULT_LVL, "AptBreakAfterAssert function is deprecated break in Aux library Assert Fail call if desired.\n");
    }
}

void AptPartialGarbageCollection()
{

    AptGetLib()->mbGarbageCollectThisFrame = true;
}

//(rrv) AS2
static int32_t nSwfVersion = 0;
//
void AptSetSwfVersion(int32_t p_version)
{
    nSwfVersion = p_version;
}
//
int32_t AptGetSwfVersion()
{
    return nSwfVersion;
}

void AptDebugPrintZombieVector()
{
#if defined(APT_ENABLE_ZOMBIE_OUTPUT)
    if (AptGetLib()->mpZombieVector == NULL)
    {
        return;
    }
    AptCIH *pCIH;
    int nNumVals = AptGetLib()->mpZombieVector->GetNumValues() - 1;
    APT_DEBUGPRINT(APT_DEBUG_MSG_DEBUG_LVL, "ZOMBIE VECTOR HAS %d ITEMS IN IT\n", nNumVals + 1);
    for (int i = nNumVals; i >= 0; i--)
    {
        pCIH                                  = AptGetLib()->mpZombieVector->GetAt(i)->c_cih();
        AptCharacterAnimationInst *pAnimation = pCIH->GetAnimationInst();
        APT_DEBUGPRINT(APT_DEBUG_MSG_DEBUG_LVL, "%d :: ZOMBIE SPRITE[name %s  file = %s.swf] HAS %d EXTERNAL REFERENCES\n",
                       i + 1,
                       pCIH->GetInstanceName().c_str(),
                       pAnimation->mpFile.Get()->GetName().c_str(),
                       pCIH->GetZombieCount());
    }
#endif
}

void AptUpdateZombieVector(bool bClean)
{
    if (AptGetLib()->mpZombieVector == NULL)
    {
        return;
    }

    AptUpdateAutoLock lock;

    AptCIH *pCIH;
    int nNumVals = AptGetLib()->mpZombieVector->GetNumValues() - 1;
    for (int i = nNumVals; i >= 0; i--)
    {
        // Check if previous iteration freed
        // elements from zombie vector.
        if (AptGetLib()->mpZombieVector->GetNumValues() <= i) // Index out of bounds assert due to empty Zombie vector.
        {
            break;
        }
        pCIH = AptGetLib()->mpZombieVector->GetAt(i)->c_cih(true);
        if (pCIH->GetCIHState() == AptCIH::AptCIHState_Zombie && (bClean || pCIH->GetZombieCount() == 0))
        {
            AptGetLib()->mpZombieVector->RemoveAt(i);
            if (pCIH->GetCharacterInst() == NULL)
            {
                // that means that the CIH is cleared out already.
                continue;
            }
            AptCharacterAnimationInst *pAnimation = pCIH->GetAnimationInst();
            pCIH->SetCIHState(AptCIH::AptCIHState_Normal);
            pAnimation->mpFile.Get()->setState(AptFile::Resolved);
#if defined(APT_ENABLE_ZOMBIE_OUTPUT)
            if (AptGetLib()->mbPrintZombieReferences)
            {
                if (bClean == false && pAnimation->mpFile.Get()->mRefCount > 2)
                {
                    // Something else is holding onto the animation's file, so only kill the zombied sprite.  Cases for this would be that the zombied animation was reloaded.
#if defined(DO_COVERAGE)
                    APT_DEBUGPRINT(APT_DEBUG_MSG_WARNING_LVL, "ZOMBIE CLEARED [name = %s  file = %s.swf] NO LONGER HAS EXTERNAL FUNCTION REFERENCES\n",
#else
                    APT_DEBUGPRINT(APT_DEBUG_MSG_WARNING_LVL, "ZOMBIE CLEARED [%x  name = %s  file = %s.swf] NO LONGER HAS EXTERNAL FUNCTION REFERENCES\n",
                                   pCIH,
#endif
                                   pCIH->GetInstanceName().c_str(),
                                   pAnimation->mpFile.Get()->GetName().c_str());
                    if (AptGetUserFuncs().pfnHandleZombieState != NULL)
                        AptGetUserFuncs().pfnHandleZombieState(false, true, pCIH->GetInstanceName().c_str(), pAnimation->mpFile.Get()->GetName().c_str());
                    pAnimation->mpFile = AptFilePtr(0);
                }
                else
                {
                    // Here, the zombied animation and file can be release.
#if defined(DO_COVERAGE)
                    APT_DEBUGPRINT(APT_DEBUG_MSG_WARNING_LVL, "ZOMBIE CLEARED [name = %s  file = %s.swf] NO LONGER HAS EXTERNAL FUNCTION REFERENCES\n",
#else
                    APT_DEBUGPRINT(APT_DEBUG_MSG_WARNING_LVL, "ZOMBIE CLEARED [%x  name = %s  file = %s.swf] NO LONGER HAS EXTERNAL FUNCTION REFERENCES\n",
                                   pCIH,
#endif
                                   pCIH->GetInstanceName().c_str(),
                                   pAnimation->mpFile.Get()->GetName().c_str());

                    if (AptGetUserFuncs().pfnHandleZombieState != NULL)
                        AptGetUserFuncs().pfnHandleZombieState(false, true, pCIH->GetInstanceName().c_str(), pAnimation->mpFile.Get()->GetName().c_str());
                    bool bLastRef = false;

                    if (pAnimation->mpFile.Get()->mRefCount == 1)
                    {
                        bLastRef = true;
                    }
                    pAnimation->mpFile = AptFilePtr(0);
                    if (bLastRef) // If this is the last reference, we have to null out the mpCharacter pointer
                    {
                        pAnimation->GetRenderItemWritable()->SetCharacter(NULL);
                    }
                }
            }
#endif
            pCIH->setGCRoot(0);

            pCIH->DestroyGCPointers();

            AptPartialGarbageCollection(); // TO allow a clean removal of this zombie sprite, we need the garbage collecter to do it.
        }
    }
}

bool AptRemoveCIHFromZombieVector(AptCIH *pTarget)
{
    AptUpdateAutoLock lock;
    if (AptGetLib()->mpZombieVector != NULL)
    {
        for (int i = AptGetLib()->mpZombieVector->GetNumValues() - 1; i >= 0; i--)
        {
            if (pTarget == AptGetLib()->mpZombieVector->GetAt(i)->c_cih(true))
            {
                AptGetLib()->mpZombieVector->RemoveAt(i);
                return true;
            }
        }
    }
    return false;
}

void AptGetMouseOverSpriteName(char *szName)
{

#if defined(APT_USE_MOUSE)
#if defined APT_USE_BUTTONS
    if (GetTargetSim()->GetAnimationTarget()->GetFocusButton() != NULL) // If this is not NULL then we have a button inst under the mouse cursor
    {
        GetTargetSim()->GetAnimationTarget()->GetFocusButton()->toString(szName);
    }
    else
#endif
        if (GetTargetSim()->GetAnimationTarget()->GetTopMostSprite() && GetTargetSim()->GetAnimationTarget()->GetTopMostSprite()->isCIH() && !GetTargetSim()->GetAnimationTarget()->GetTopMostSprite()->c_cih()->GetInstanceName().IsEmpty()) // Else there might be a moveclip inst
    {
        GetTargetSim()->GetAnimationTarget()->GetTopMostSprite()->toString(szName);
    }
    else // Else we have nothing that handles mouse input
    {
        strcpy(szName, "");
    }
#else
    APT_ASSERT(false && "MOUSE NOT SUPPORTED, define APT_USE_MOUSE to use the mouse");
    strcpy(szName, "");
#endif
}

void AptRegisterGlobalReferences()
{

    AptTarget *pTmp = AptGetLib()->mpDefaultTarget;
    while (pTmp)
    {
        pTmp->GetAnimationTarget()->RegisterReferences();
        pTmp = pTmp->GetNext();
    }

    // AptGetLib()->mpObjRegistrationHash only exists if _gObjRegistrationFunc is called.
    if (AptGetLib()->mpObjRegistrationHash)
    {
#if defined(APT_XML_MEMORY_DUMP_SUPPORTED)
        AptGetLib()->mpObjRegistrationHash->RegisterReferences(NULL, "Object.registerClass::");
#else
        AptGetLib()->mpObjRegistrationHash->RegisterReferences(NULL);
#endif
    }
}

void AptPreloadFilterSWFFile()
{
#if APT_USE_FILTERS
    static const char szLevel[] = "_level24";

    // preload this filter swf file, immediately after apt initialization (AptTarget creation) and also
    // resolve this file so on next AptUpdate the actionscript in its first frame to define all the
    // filter classes gets executed, and we have these filter classes ready even before first SWF file
    // is loaded by aptaux
    AptNativeString sFileName(sFiltersPseudoFileName);
    // this will add filter definition file to linker and loader queues
    AptGetLib()->mpDefaultTarget->GetLinker()->Load(sFileName + ".swf", szLevel);
    AptFilePtr fp = AptGetLib()->mpDefaultTarget->GetLoader()->findFile(sFileName);
    fp->setState(AptFile::WaitingForData);
    // immediately call this to let Apt loader/linker logic know that the .Apt and
    // .const files for filters are ready for processing.
    // this will resolve/parse the .apt file and it will be ready for execution on the
    // next AptUpdate call for AptLinker->Update function.

    AptCompleteAnimationAsyncLoad(fp, g_arrFiltersAptFile, g_arrFiltersConstFile, (void *)APT_FILTERS_FILE_USERDATA);
#endif
}

void AptGetMovieclipInfo(AptMovieclipInformation *pMCInfo, bool bGenerateNew)
{
    AptUpdateAutoLock lock;

    APT_ASSERT(pMCInfo);

    if (bGenerateNew)
    {
        AptTarget *pTmp = AptGetLib()->mpDefaultTarget;
        while (pTmp)
        {
            pTmp->GetAnimationTarget()->GetDisplayList()->getState()->GetMovieclipInfo(pMCInfo);
            pTmp = pTmp->GetNext();
        }
    }
    else
    {
        // copy already calculated gAptMovieclipInformation into pMCInfo
#if defined(APT_GATHER_MOVIECLIP_METRICS)
        *pMCInfo = gAptMovieclipInformation;
#else
        APT_DEBUGPRINT(APT_DEBUG_MSG_WARNING_LVL, "Please define APT_GATHER_MOVIECLIP_METRICS in AptDEfine.h to get correct values, or pass true to this function\n");
#endif
    }
}

#if defined(APT_XML_MEMORY_DUMP_SUPPORTED)
const char *gVFTNameStrings[] =
    {
        "xxx",
        "StringValue",
        "Property",
        "None",
        "Register",
        "Boolean",
        "Float",
        "Integer",
        "Lookup",
        "NativeFunction",
        "FrameStack",
        "Extern",
        "CharacterInstHandle",
        "Sound",
        "Array",
        "Math",
        "Key",
        "Global",
        "ScriptColour",
        "Object",
        "Prototype",
        "Date",
        "MovieClip",
        "Mouse",
        "XmlNode",
        "Xml",
        "XmlAttributes",
        "LoadVars",
        "TextFormat",
        "Extension",
        "GlobalExtension",
        "Stage",
        "Error",
        "StringObject",
        "ScriptFunction1",
        "ScriptFunction2",
        "ScriptFunctionByteCodeBlock",
        "CIHNone",
        "MovieClipLoader"};
#endif

const char *AptGetTypeOfAptValue(const AptValue *pValue)
{
    AptUpdateAutoLock lock;

    const char *pVal = 0;

#if defined(APT_XML_MEMORY_DUMP_SUPPORTED)
    AptVirtualFunctionTable_Indices eType = pValue->getVtblIndex();

    if (eType < AptVFT_NumVFTs)
    {
        pVal = gVFTNameStrings[pValue->getVtblIndex()];
    }
    else
    {
        APT_ASSERT(eType < AptVFT_NumVFTs);
        pVal = "INVALID";
    }
#else
    static char tempString[8];
    sprintf(tempString, "0x%X", pValue->getVtblIndex());
    pVal = tempString;
#endif

    return pVal;
}

const uint8_t AptValueSizesByVType[] =
    {
        0,                                      //
        sizeof(AptNativeString),                // AptVFT_StringValue
        0,                                      // AptVFT_Property
        sizeof(AptNone),                        // AptVFT_None
        sizeof(AptRegister),                    // AptVFT_Register
        sizeof(AptBoolean),                     // AptVFT_Boolean
        sizeof(AptFloat),                       // AptVFT_Float
        sizeof(AptInteger),                     // AptVFT_Integer
        sizeof(AptLookup),                      // AptVFT_Lookup
        sizeof(AptNativeFunction),              // AptVFT_NativeFunction
        sizeof(AptFrameStack),                  // AptVFT_FrameStack
        sizeof(AptExtern),                      // AptVFT_Extern
        sizeof(AptCIH),                         // AptVFT_CharacterInstHandle
        sizeof(AptSound),                       // AptVFT_Sound
        sizeof(AptArray),                       // AptVFT_Array
        sizeof(AptMathObj),                     // AptVFT_Math
        sizeof(AptKey),                         // AptVFT_Key
        sizeof(AptGlobal),                      // AptVFT_Global
        sizeof(AptScriptColour),                // AptVFT_ScriptColour
        sizeof(AptObject),                      // AptVFT_Object
        sizeof(AptPrototype),                   // AptVFT_Prototype
        sizeof(AptDate),                        // AptVFT_Date
        sizeof(AptMovieClip),                   // AptVFT_MovieClip
        sizeof(AptMouse),                       // AptVFT_Mouse
        sizeof(AptXmlNode),                     // AptVFT_XmlNode
        sizeof(AptXml),                         // AptVFT_Xml
        sizeof(AptXmlAttributes),               // AptVFT_XmlAttributes
        sizeof(AptLoadVars),                    // AptVFT_LoadVars
        sizeof(AptTextFormat),                  // AptVFT_TextFormat
        sizeof(AptExtObject),                   // AptVFT_Extension
        sizeof(AptGlobalExtensionObject),       // AptVFT_GlobalExtension
        sizeof(AptStage),                       // AptVFT_Stage
        sizeof(AptError),                       // AptVFT_Error
        sizeof(AptStringObject),                // AptVFT_StringObject
        sizeof(AptScriptFunction1),             // AptVFT_ScriptFunction1
        sizeof(AptScriptFunction2),             // AptVFT_ScriptFunction2
        sizeof(AptScriptFunctionByteCodeBlock), // AptVFT_ScriptFunctionByteCodeBlock
        sizeof(AptCIHNone),                     // AptVFT_CIHNone
        sizeof(AptMovieClipLoader),             // AptVFT_MovieClipLoader
        sizeof(AptUtil),                        // AptVFT_AptUtil
        0,                                      // AptVFT_AptExternalFunction, size found elsewhere
#if defined(APT_ALTERNATE_INPUT)
        sizeof(AptAlternateInput), // AptVFT_AltInput
#endif
        0};

uint32_t AptGetSizeOfAptValue(const AptValue *pValue)
{
    AptVirtualFunctionTable_Indices eType = pValue->getVtblIndex();
    APT_ASSERT(eType < AptVFT_NumVFTs);
    APT_ASSERT(eType >= AptVFT_xxx);
    uint32_t size = 0;

    switch (eType)
    {
    case AptVFT_Extension:
        size = (static_cast<const AptExtObject *>(pValue))->GetSize();
        break;

    case AptVFT_ExternalFunction:
        size = (static_cast<const AptExternalFunction *>(pValue))->GetClassSize();
        break;

    default:
        size = AptValueSizesByVType[eType];
        break;
    }

    return size;
}

#if defined(APT_ALLOCATION_TRACKING)
void *AptNonGCAllocSaveSize(size_t size, const char *szName, int nLine)
#else
void *AptNonGCAllocSaveSize(size_t size)
#endif
{
    uint32_t *x = (uint32_t *)APT_MALLOC_BLOCK((size + 4));
    x[0]        = static_cast<uint32_t>(size);
    return &x[1];
}

void AptNonGCFreeSavedSize(void *p)
{
    uint32_t *x = (uint32_t *)p;
    APT_FREE_BLOCK((x - 1), (*(x - 1)) + 4);
}

void AptSetSimulationThread()
{
#ifndef APT_SINGLE_THREADED
    gAptSimThreadId = APT_THREAD_NAMESPACE::GetThreadId();
#endif
}

void AptAssertIsSimulationThread()
{
#ifndef APT_SINGLE_THREADED
    APT_ASSERT(AptIsSimulationThread());
#endif
}

bool AptIsSimulationThread()
{
#ifndef APT_SINGLE_THREADED
    return gAptSimThreadMutex.HasLock() || gAptSimThreadId == APT_THREAD_NAMESPACE::GetThreadId();
#else
    return true;
#endif
}

void AptAssertIsRenderThread()
{
}

const char *AptGetFunctionDebugName(AptValue *pFunctionValue, const char *defaultName)
{
#if defined(APT_USE_DEBUG_NAMES)
    // If it is a function, we hope to have stored its name at some point
    // in the past.  If so, we did it by throwing a name into the Function's HashTable
    if (pFunctionValue->isScriptFunction())
    {
        AptNativeHash *pFuncHash = pFunctionValue->GetNativeHashVirtual();
        AptValue *currentName    = pFuncHash->Lookup(StringPool::GetString(SC__debugName));
        if (NULL != currentName && currentName->isString())
        {
            return currentName->c_string()->GetInternalString()->c_str();
        }
    }
#endif
    return defaultName;
}

