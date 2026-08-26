#include "_Apt.h"
#include "AptDefine.h"
#include "AptRenderableCustomControl.h"
#include "AptRenderableGeometry.h"
#include "AptSafeQueueFixed.h"

#include <string.h>

static char *copy_string(const char *s)
{
    size_t sz = strlen(s);
    char *ns  = (char *)APT_MALLOC_BLOCK(sz + 1);
    memcpy(ns, s, sz + 1);
    return ns;
}

// We use this queue because we don't want to free the custom control zID in the sim thread.
// We just queue up the zID in our destructor. Then, later, it gets freed by the render thread.
AptSafeQueueFixed *AptRenderableCustomControl::spFreeQueue = NULL;


void AptRenderableCustomControl::Set(const char *szType, const char *szTarget, const char *szProperties)
{
    if (!mszType)
        mszType = copy_string(szType);
    else if (strcmp(szType, mszType) != 0)
    {
        APT_FREE_BLOCK(mszType, strlen(mszType) + 1);
        mszType = copy_string(szType);
    }

    if (!mszTarget)
        mszTarget = copy_string(szTarget);
    else if (strcmp(szTarget, mszTarget) != 0)
    {
        APT_FREE_BLOCK(mszTarget, strlen(mszTarget) + 1);
        mszTarget = copy_string(szTarget);
    }

    if (!mszProperties)
        mszProperties = copy_string(szProperties);
    else if (strcmp(szProperties, mszProperties) != 0)
    {
        APT_FREE_BLOCK(mszProperties, strlen(mszProperties) + 1);
        mszProperties = copy_string(szProperties);
    }
}

void AptRenderableCustomControl::RenderPrepare()
{
}

void AptRenderableCustomControl::Render(const AptRenderInfo &info, AptRenderableGeometry *pGeometry)
{
    if (mZid && AptGetUserFuncs().pfnCustomControlRenderWithZid)
    {
        // Masking issue with custom render controls (see Apt.h for details)
        if (AptGetUserFuncs().pfnRenderDispatch)
        {
            AptGetUserFuncs().pfnRenderDispatch(&info, true /* bCustomRender */);
        };
        AptGetUserFuncs().pfnCustomControlRenderWithZid(mZid, pGeometry->mRenderUnit, &info);
    }
    else if (mszType)
    {
        // Masking issue with custom render controls (see Apt.h for details)
        if (AptGetUserFuncs().pfnRenderDispatch)
        {
            AptGetUserFuncs().pfnRenderDispatch(&info, true /* bCustomRender */);
        };
        AptGetUserFuncs().pfnCustomControlRender(const_cast<char *>(mszType),
                                         const_cast<char *>(mszTarget), pGeometry->mRenderUnit,
                                         const_cast<char *>(mszProperties), &info);
    }
}

AptRenderableCustomControl::~AptRenderableCustomControl()
{
    AptAssetCustomControlZId zid = mZid;
    mZid                         = 0;
    if (zid && AptGetUserFuncs().pfnDestroyCustomControlZid)
    {
        APT_ASSERT(spFreeQueue);
        if (spFreeQueue)
        {
            // if Enqueue causes an assert, then you must increase the value of
            // kMaxFreeCustCtrlArrSize in Apt.cpp -- the default maximum is 512.
            //  Or, perhaps you need to use fewer custom controls.
            spFreeQueue->Enqueue(zid);
        }
    }
    nRenderableCustomControls--;
    if (mszType)
        APT_FREE_BLOCK(mszType, strlen(mszType) + 1);
    if (mszTarget)
        APT_FREE_BLOCK(mszTarget, strlen(mszTarget) + 1);
    if (mszProperties)
        APT_FREE_BLOCK(mszProperties, strlen(mszProperties) + 1);
    mszType = mszTarget = mszProperties = 0;
}
