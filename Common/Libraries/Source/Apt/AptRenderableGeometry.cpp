#include "_Apt.h"
#include "AptDefine.h"
#include "AptRenderableGeometry.h"


void AptRenderableGeometry::RenderPrepare()
{
}

void AptRenderableGeometry::Render(const AptRenderInfo &info, AptRenderableGeometry *pGeometry)
{
    APT_ASSERT(mRenderUnit);
    {
        AptGetUserFuncs().pfnDrawRenderingUnit(mRenderUnit, &info);
    }
}

AptRenderableGeometry::~AptRenderableGeometry()
{
    AptAssetRenderingUnit ru = mRenderUnit;
    mRenderUnit              = 0;
    AptGetUserFuncs().pfnFreeRenderingUnit(ru);
    nRenderableGeometries--;
    mpFile = NULL;
}
