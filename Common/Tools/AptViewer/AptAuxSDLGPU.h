#pragma once

/**
 * Apt auxiliary library for the SDL3 GPU viewer.
 *
 * Port of EA's AptAuxPCOpenGL with two deliberate changes:
 *  - assets are loaded as loose files relative to the animation directory
 *    (<base>.apt, <base>.const, <base>.dat, <base>_geometry/<id>.ru,
 *    art/Textures/apt_<base>_<id>.tga) - no .big archive support
 *  - all rendering goes through AptGpuRenderer (SDL3 GPU) instead of
 *    fixed-function OpenGL
 */

#include "SDLGPURenderer.h"

struct AptAuxFontList
{
    int nFonts;
    unsigned short *aszFontNamesEnglish[16];
    unsigned short *aszFontNamesJapanese[16];
    void *pData[16];
    int nDataSize[16];
};

/// The renderer must outlive Apt; it is used by the draw callbacks.
// Fills in the Apt user callbacks this aux layer implements. The callbacks are
// now supplied to AptLibraryInitialize() rather than poked into a global, so
// this takes the struct to populate.
void AptAuxSDLGPU_Initialize(AptUserFunctions &funcs, AptGpuRenderer *pRenderer, AptAuxFontList *pFonts);
void AptAuxSDLGPU_Shutdown();

/// Completes loads queued by the pfnLoadAnimation callback; call once per frame before AptUpdate.
void AptAuxSDLGPU_FinishAsyncLoads();

/// Sets the value returned for a game ("extern") variable queried by the movie;
/// unknown variables default to an empty string.
void AptAuxSDLGPU_SetExternVariable(const char *szName, const char *szValue);

unsigned int AptAuxSDLGPU_GetBackgroundColour();
