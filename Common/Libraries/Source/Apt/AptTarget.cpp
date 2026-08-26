/*** Include files ********************************************************************************/
#include "_Apt.h"
#include "AptTarget.h"
#include "AptAnimationTarget.h"
#include "AptBCRenderTree.h"
#include "MainInline.h"
#include "AptRenderList.h"

/*** Defines **************************************************************************************/

/*** Macros ***************************************************************************************/

/*** Type Definitions *****************************************************************************/

/*** Function Prototypes **************************************************************************/

/*** Variables ************************************************************************************/

// Private variables
// Public variables
// The target registry now lives on the library (AptLibrary::mpDefaultTarget,
// mpCurrentTargetSim, mpCurrentTargetRender, mnTargetsCreated).

/*** Private Functions ****************************************************************************/

/*** Public Functions *****************************************************************************/
//////////////////////////////////////////////////////////////////////////
// AptTarget Implementation
AptTarget::AptTarget(const AptInitParams *aptInitParms)
{
    AptUpdateAutoLock lock;

    mpLibrary = AptGetLib();
    mInitParms.CopyAptInitParms(aptInitParms);
    mpPool   = new AptAnimationTarget(&mInitParms);
    mpLoader = new AptLoader();
    mpLinker = new AptLinker();
    mpRTMgr  = new AptBCRenderTreeManager();

    mpNext = NULL;
    mpPrev = NULL;
}

AptTarget::AptTarget(const AptTargetInitParams *aptInitParms)
{
    mpLibrary = AptGetLib();
    memcpy(&mInitParms, aptInitParms, sizeof(AptTargetInitParams));
    mpPool   = new AptAnimationTarget(&mInitParms);
    mpLoader = new AptLoader();
    mpLinker = new AptLinker();
    mpRTMgr  = new AptBCRenderTreeManager();

    mpNext = NULL;
    mpPrev = NULL;
}

AptTarget::~AptTarget()
{
    Shutdown();
    mpPool   = NULL;
    mpLoader = NULL;
    mpLinker = NULL;
    mpNext   = NULL;
    mpPrev   = NULL;
    mpRTMgr  = NULL;
}

void AptTarget::Shutdown()
{
    AptUpdateAutoLock lock;

    AptRenderList::Stop();

    APT_SETUP_SIM_TARGET_STATES(this);
    APT_SETUP_RENDER_TARGET_STATES(this);

    bool bPrevState                 = gAptActionInterpreter.bShutDown;
    gAptActionInterpreter.bShutDown = true;

    if (mpPool != NULL)
    {
        mpPool->PreDestroy(); // Do this before deleting the mpLoader and mpLinker, then delete the mpPool after
        mpPool->CleanRemList();
    }

    // XXX this is actually not good, as there can be zombies across targets. But putting it in to fix
    // shutdown crashes in rw4.5 ps2 viewers which first delete target and then later call AptShutdown.
    // so here we make sure we force delete all zombies. we probably have to have separate zombie vectors
    // for every target.
    AptUpdateZombieVector(true);

    if (mpRTMgr != NULL)
    {
        // We are in the correct thread or we are shutting down, go ahead and clean the render manager.
        mpRTMgr->Render_Shutdown();
        delete mpRTMgr;
        mpRTMgr = NULL;
    }

    if (mpLoader != NULL)
    {
        delete mpLoader;
        mpLoader = NULL;
    }
    if (mpLinker != NULL)
    {
        delete mpLinker;
        mpLinker = NULL;
    }
    if (mpPool != NULL)
    {
        delete mpPool;
        mpPool = NULL;
    }

    gAptActionInterpreter.bShutDown = bPrevState;

    AptRenderList::Stop();
    APT_TEARDOWN_RENDER_TARGET_STATES;
    APT_TEARDOWN_SIM_TARGET_STATES;
}
void AptTarget::Reset()
{
    Shutdown();
    AptRenderList::Restart();

    mpRTMgr = new AptBCRenderTreeManager();

    mpPool   = new AptAnimationTarget(&mInitParms);
    mpLoader = new AptLoader();
    mpLinker = new AptLinker();
}
//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
// AptTarget API functions
AptTarget *GetTarget()
{
    return AptGetLib()->mpCurrentTargetSim;
}

AptTargetInstance AptCreateTargetInstance(const AptTargetInitParams *aptInitParms)
{
    AptUpdateAutoLock lock;

    AptGetLib()->mnTargetsCreated++;
    AptTarget *pNew = new AptTarget(aptInitParms);
    AptTarget *pTmp = AptGetLib()->mpDefaultTarget;

    if (pTmp != NULL)
    {
        while (pTmp->GetNext() != NULL)
            pTmp = pTmp->GetNext();
        pTmp->SetNext(pNew);
        pNew->SetPrevious(pTmp);
    }
    else
    {
        AptGetLib()->mpDefaultTarget = pNew; // Set as default for now
        // we have to set these in order to pre-load filters definition swf file
        AptGetLib()->mpCurrentTargetSim    = pNew;
        AptGetLib()->mpCurrentTargetRender = pNew;

        AptPreloadFilterSWFFile();
    }
    return (AptTargetInstance)(pNew);
}

void AptChangeTargetInstance(AptTargetInstance Target)
{
    APT_ASSERT((AptTarget *)Target != NULL);
    AptGetLib()->mpCurrentTargetSim    = (AptTarget *)Target;
    AptGetLib()->mpCurrentTargetRender = (AptTarget *)Target;
}

void AptChangeSimTargetInstance(AptTargetInstance Target)
{
    APT_ASSERT((AptTarget *)Target != NULL);
    AptGetLib()->mpCurrentTargetSim = (AptTarget *)Target;
}

void AptChangeRenderTargetInstance(AptTargetInstance Target)
{
    APT_ASSERT((AptTarget *)Target != NULL);
    AptGetLib()->mpCurrentTargetRender = (AptTarget *)Target;
}

void AptDestroyTargetInstance(AptTargetInstance Target)
{
    AptGetLib()->mnTargetsCreated--;
    AptTarget *pTmp  = (AptTarget *)Target;
    AptTarget *pNext = pTmp->GetNext();
    AptTarget *pPrev = pTmp->GetPrevious();

    if (pTmp == AptGetLib()->mpCurrentTargetSim)
    {
        if (!pNext)
        {
            AptRenderList::Stop();
        }
        AptGetLib()->mpCurrentTargetSim = pNext;
    }
    if (pTmp == AptGetLib()->mpCurrentTargetRender)
    {
        AptGetLib()->mpCurrentTargetRender = pNext;
    }
    if (pTmp == AptGetLib()->mpDefaultTarget)
    {
        AptGetLib()->mpDefaultTarget = pNext;
    }
    if (pPrev != NULL)
    {
        pPrev->SetNext(pNext);
    }
    if (pNext != NULL)
    {
        pNext->SetPrevious(pPrev);
    }

    {
        // protect Dogma pools in case we're being called from non-sim thread.
        AptUpdateAutoLock lock;
        delete pTmp;
    }
}

void AptResetTargetInstance(AptTargetInstance Target)
{
    APT_SETUP_SIM_TARGET_STATES(Target);
    ((AptTarget *)Target)->Reset();
    APT_TEARDOWN_SIM_TARGET_STATES;
}

void AptSetDefaultTargetInstance()
{
    AptGetLib()->mpCurrentTargetSim    = AptGetLib()->mpDefaultTarget;
    AptGetLib()->mpCurrentTargetRender = AptGetLib()->mpDefaultTarget;
}
//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
// Core Apt AptTarget API functions
void AptTargetCompleteAnimationAsyncLoad(AptTargetInstance Target, AptFilePtr file, void *pData, void *pConstTable, void *pUserData)
{
    APT_SETUP_SIM_TARGET_STATES(Target);
    AptCompleteAnimationAsyncLoad(file, pData, pConstTable, pUserData);
    APT_TEARDOWN_SIM_TARGET_STATES;
}

void AptTargetCompleteImageAsyncLoad(AptTargetInstance Target, AptFilePtr file, AptAssetTexture texture, int width, int height, void *userData)
{
    APT_SETUP_SIM_TARGET_STATES(Target);
    AptCompleteImageAsyncLoad(file, texture, width, height, userData);
    APT_TEARDOWN_SIM_TARGET_STATES;
}

void AptTargetLoadAnimation(AptTargetInstance Target, const char *szBaseName, const char *szTarget)
{
    APT_SETUP_SIM_TARGET_STATES(Target);
    AptLoadAnimation(szBaseName, szTarget);
    APT_TEARDOWN_SIM_TARGET_STATES;
}

AptFilePtr AptTargetPreloadAnimation(AptTargetInstance Target, const char *szBaseName)
{
    APT_SETUP_SIM_TARGET_STATES(Target);
    AptFilePtr pTmp = AptPreloadAnimation(szBaseName);
    APT_TEARDOWN_SIM_TARGET_STATES;
    return pTmp;
}

void AptTargetCancelPreloadedAnimation(AptTargetInstance Target, const char *szBaseName)
{
    APT_SETUP_SIM_TARGET_STATES(Target);
    AptCancelPreloadedAnimation(szBaseName);
    APT_TEARDOWN_SIM_TARGET_STATES;
}

bool AptTargetIsFileLoaded(AptTargetInstance Target, AptFilePtr file)
{
    APT_SETUP_SIM_TARGET_STATES(Target);
    bool bRet = AptIsFileLoaded(file);
    APT_TEARDOWN_SIM_TARGET_STATES;
    return bRet;
}

void AptTargetSetMousePosition(AptTargetInstance Target, int nX, int nY)
{
    APT_SETUP_SIM_TARGET_STATES(Target);
    AptSetMousePosition(nX, nY);
    APT_TEARDOWN_SIM_TARGET_STATES;
}

void AptTargetAddToInputQueue(AptTargetInstance Target, AptInputType eInput, AptInputState eState, AptInputController eController)
{
    APT_SETUP_SIM_TARGET_STATES(Target);
    AptAddToInputQueue(eInput, eState, eController);
    APT_TEARDOWN_SIM_TARGET_STATES;
}

void AptTargetAddToInputAnalogQueue(AptTargetInstance Target, AptAnalogStickInfo pAnalogInput)
{
    APT_SETUP_SIM_TARGET_STATES(Target);
    AptAddToInputAnalogQueue(pAnalogInput);
    APT_TEARDOWN_SIM_TARGET_STATES;
}

void AptTargetAddToInputGestureQueue(AptTargetInstance Target, AptGestureInfo pGestureInput)
{
    APT_SETUP_SIM_TARGET_STATES(Target);
    AptAddToInputGestureQueue(pGestureInput);
    APT_TEARDOWN_SIM_TARGET_STATES;
}

#if defined(APT_ALTERNATE_INPUT)
void AptTargetAddAlternateInput(AptTargetInstance Target, const char *szEvent, AptValue *pValue)
{
    APT_SETUP_SIM_TARGET_STATES(Target);
    AptAddAlternateInput(szEvent, pValue);
    APT_TEARDOWN_SIM_TARGET_STATES;
}
#endif
