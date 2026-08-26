/******************************************************************************/
/**
    Copyright 2012 Electronic Arts Inc.

    @file       AptRenderableImage.cpp

    @brief      Renderable wrapper for direct-loaded images
*/
/******************************************************************************/

/*** Includes *****************************************************************/

#include "_Apt.h"
#include "AptRenderableImage.h"
#include "AptDefine.h"

/*** Macros *******************************************************************/

/*** Namespaces ***************************************************************/

/*** Constants ****************************************************************/

/*** Declarations *************************************************************/

/*** Variables ****************************************************************/

/*** Implementation ***********************************************************/

/******************************************************************************/
/**
    @brief  Constructor

    @param  texture Texture we will be rendering
    @param  file    Pointer to the file.  Useful because we want to keep a
                    reference around while we're still rendering it; we don't
                    want to unload the file until we're finished rendering it.
*/
/******************************************************************************/
AptRenderableImage::AptRenderableImage(AptAssetTexture texture, AptFilePtr file)
    : mTexture(texture), mFile(file)
{
    APT_ASSERT(file != AptFilePtr(0));
}

/******************************************************************************/
/**
    @brief  Render this renderable texture

    @param  info     Info, like position matrices and mask stuff
    @param  geometry Geometry (probably NULL)
*/
/******************************************************************************/
void AptRenderableImage::RenderPrepare()
{
}

void AptRenderableImage::Render(const AptRenderInfo &info, AptRenderableGeometry *geometry)
{
    if (mTexture)
    {
        APT_ASSERT(AptGetUserFuncs().pfnDrawImage);
        if (AptGetUserFuncs().pfnDrawImage)
        {
            AptGetUserFuncs().pfnDrawImage(mTexture, &info);
        }
    }
}

/******************************************************************************/
/**
    @brief  AptRenderableImage destructor.

    Kills the AptFilePtr, freeing up the file to be unloaded
*/
/******************************************************************************/
AptRenderableImage::~AptRenderableImage()
{
    mTexture = NULL;
    mFile    = AptFilePtr(NULL);
}

/******************************************************************************/
