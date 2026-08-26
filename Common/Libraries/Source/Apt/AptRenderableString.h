#pragma once

#include "AptRenderable.h"
#include "AptRefCounted.h"

struct AptRenderInfo;

class AptRenderableString : public AptRefCounted<IAptRenderable>
{
  public:
    APT_RI_NEW_DELETE_OPERATORS

    AptAssetString mString;

    AptRenderableString(const AptAssetString &str) : mString(str) { nRenderableStrings++; }

    void RenderPrepare();
    void Render(const AptRenderInfo &info, AptRenderableGeometry *pGeometry);
    const AptFile *GetFile() const { return 0; }

  protected:
    ~AptRenderableString();
};

inline AptAssetString GetZID(const AptRenderableString *pText)
{
    return pText ? pText->mString : 0;
}

