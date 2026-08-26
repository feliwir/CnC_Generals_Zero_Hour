/******************************************************************************/
/**
    Copyright 2012 Electronic Arts Inc.

    @file       AptRenderableImage.h

    @brief      Renderable wrapper for direct-loaded images
*/
/******************************************************************************/

/*** Include guard ************************************************************/

#pragma once

/*** Includes *****************************************************************/

#include "Apt.h"
#include "AptRenderable.h"
#include "AptRefCounted.h"

/*** Forward Declarations *****************************************************/

struct AptRenderInfo;

/*** Macros *******************************************************************/

/*** Namespaces ***************************************************************/

/*** Interfaces ***************************************************************/

/*** Implementation ***********************************************************/

class AptRenderableImage : public AptRefCounted<IAptRenderable>
{
  public:
    APT_RI_NEW_DELETE_OPERATORS

    AptRenderableImage(AptAssetTexture texture, AptFilePtr file);

    void RenderPrepare();
    void Render(const AptRenderInfo &info, AptRenderableGeometry *geometry);
    const AptFile *GetFile() const { return mFile.Get(); }

  protected:
    virtual ~AptRenderableImage();

  private:
    AptAssetTexture mTexture;
    AptFilePtr mFile;
};

/******************************************************************************/

