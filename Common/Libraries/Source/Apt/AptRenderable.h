#pragma once

struct AptRenderInfo;
struct AptFile;
class AptRenderableGeometry;

extern int nRenderableGeometries,
    nRenderableCustomControls,
    nRenderableStrings;

class IAptRenderable
{
  public:
    // not needed for this class as it is a ABC
    // APT_RI_NEW_DELETE_OPERATORS

    virtual void RenderPrepare()                                                     = 0;
    virtual void Render(const AptRenderInfo &info, AptRenderableGeometry *pGeometry) = 0;
    virtual const AptFile *GetFile() const                                           = 0;
    virtual void AddRef() const                                                      = 0;
    virtual void Release() const                                                     = 0;

  protected:
    virtual ~IAptRenderable() {}
};
