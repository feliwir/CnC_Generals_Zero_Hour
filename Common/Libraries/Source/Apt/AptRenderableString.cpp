#include "_Apt.h"
#include "AptDefine.h"
#include "AptRenderableString.h"
#include "AptCIH.h"


void AptRenderableString::RenderPrepare()
{
    APT_ASSERT(mString);
    {
        AptGetUserFuncs().pfnDrawString(mString, NULL);
    }
}

void AptRenderableString::Render(const AptRenderInfo &info, AptRenderableGeometry *pGeometry)
{
    APT_ASSERT(mString);
    {
        AptGetUserFuncs().pfnDrawString(mString, &info);
    }
}

AptRenderableString::~AptRenderableString()
{
    AptAssetString str = mString;
    mString            = 0;
    if (str && str != &AptCIH::sEmptyAssetString)
    {
        AptGetUserFuncs().pfnDeallocateString(str, APT_TEXTFIELD_FUPDATE);
    }
    nRenderableStrings--;
}
