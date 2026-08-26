#ifndef _Apt_AptRenderableGeometry_h
#define _Apt_AptRenderableGeometry_h

#include "AptRenderable.h"
#include "AptRefCounted.h"

struct AptRenderInfo;

class AptRenderableGeometry : public AptRefCounted<IAptRenderable>
{
  public:
    APT_RI_NEW_DELETE_OPERATORS

    AptAssetRenderingUnit mRenderUnit;

    AptRenderableGeometry(AptAssetRenderingUnit ru, const AptFile *pFile) : mRenderUnit(ru), mpFile(pFile) { nRenderableGeometries++; }

    void RenderPrepare();
    void Render(const AptRenderInfo &info, AptRenderableGeometry *pGeometry);
    const AptFile *GetFile() const { return mpFile; }

  protected:
    ~AptRenderableGeometry();

  private:
    const AptFile *mpFile;
};

inline AptAssetRenderingUnit GetZID(const AptRenderableGeometry *pGeo)
{
    return pGeo ? pGeo->mRenderUnit : 0;
}

#endif // _Apt_AptRenderableGeometry_h
