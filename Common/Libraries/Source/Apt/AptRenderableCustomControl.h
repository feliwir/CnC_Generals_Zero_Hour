#pragma once

#include "AptRenderable.h"
#include "AptRefCounted.h"

struct AptRenderInfo;
class AptSafeQueueFixed;

class AptRenderableCustomControl : public AptRefCounted<IAptRenderable>
{
  public:
    APT_RI_NEW_DELETE_OPERATORS

    AptAssetCustomControlZId mZid;

    AptRenderableCustomControl(AptAssetCustomControlZId zid = 0) : mZid(zid) { nRenderableCustomControls++; }

    void Set(const char *szType, const char *szTarget, const char *szProperties);

    void RenderPrepare();
    void Render(const AptRenderInfo &info, AptRenderableGeometry *pGeometry);
    const AptFile *GetFile() const { return 0; }

    static void SetFreeQueue(AptSafeQueueFixed *pQueue) { spFreeQueue = pQueue; }

    static const int32_t MAX_FREE_CUSTCTRLS = 512;

  protected:
    ~AptRenderableCustomControl();

  private:
    static AptSafeQueueFixed *spFreeQueue;

    char *mszType{0},
        *mszTarget{0},
        *mszProperties{0};
};

inline AptAssetCustomControlZId GetZID(const AptRenderableCustomControl *pCC)
{
    return pCC ? pCC->mZid : 0;
}

