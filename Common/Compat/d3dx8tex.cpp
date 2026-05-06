#include "d3dx8tex.h"
#include <algorithm>
#include <cstring>
#include <limits.h>
#define D3DX_DEFAULT UINT_MAX

namespace
{
    inline unsigned int clampi(int v, int lo, int hi)
    {
        if (v < lo)
        {
            return static_cast<unsigned int>(lo);
        }
        if (v > hi)
        {
            return static_cast<unsigned int>(hi);
        }
        return static_cast<unsigned int>(v);
    }

    inline unsigned int lerp8(unsigned int a, unsigned int b, float t)
    {
        return static_cast<unsigned int>(a + (b - a) * t + 0.5f);
    }

    inline unsigned int sampleA1R5G5B5Nearest(const unsigned short *src, int srcWidth, int srcHeight, int x, int y)
    {
        const unsigned int sx = clampi(x, 0, srcWidth - 1);
        const unsigned int sy = clampi(y, 0, srcHeight - 1);
        return src[sy * srcWidth + sx];
    }

    inline unsigned int sampleA8R8G8B8Nearest(const unsigned int *src, int srcWidth, int srcHeight, int x, int y)
    {
        const unsigned int sx = clampi(x, 0, srcWidth - 1);
        const unsigned int sy = clampi(y, 0, srcHeight - 1);
        return src[sy * srcWidth + sx];
    }

    inline unsigned int sampleA8R8G8B8Bilinear(const unsigned int *src, int srcWidth, int srcHeight, float fx, float fy)
    {
        const int x0 = static_cast<int>(fx);
        const int y0 = static_cast<int>(fy);
        const int x1 = x0 + 1;
        const int y1 = y0 + 1;

        const float tx = fx - static_cast<float>(x0);
        const float ty = fy - static_cast<float>(y0);

        const unsigned int c00 = sampleA8R8G8B8Nearest(src, srcWidth, srcHeight, x0, y0);
        const unsigned int c10 = sampleA8R8G8B8Nearest(src, srcWidth, srcHeight, x1, y0);
        const unsigned int c01 = sampleA8R8G8B8Nearest(src, srcWidth, srcHeight, x0, y1);
        const unsigned int c11 = sampleA8R8G8B8Nearest(src, srcWidth, srcHeight, x1, y1);

        const unsigned int a00 = (c00 >> 24) & 0xFF;
        const unsigned int r00 = (c00 >> 16) & 0xFF;
        const unsigned int g00 = (c00 >> 8) & 0xFF;
        const unsigned int b00 = c00 & 0xFF;

        const unsigned int a10 = (c10 >> 24) & 0xFF;
        const unsigned int r10 = (c10 >> 16) & 0xFF;
        const unsigned int g10 = (c10 >> 8) & 0xFF;
        const unsigned int b10 = c10 & 0xFF;

        const unsigned int a01 = (c01 >> 24) & 0xFF;
        const unsigned int r01 = (c01 >> 16) & 0xFF;
        const unsigned int g01 = (c01 >> 8) & 0xFF;
        const unsigned int b01 = c01 & 0xFF;

        const unsigned int a11 = (c11 >> 24) & 0xFF;
        const unsigned int r11 = (c11 >> 16) & 0xFF;
        const unsigned int g11 = (c11 >> 8) & 0xFF;
        const unsigned int b11 = c11 & 0xFF;

        const unsigned int a0 = lerp8(a00, a10, tx);
        const unsigned int r0 = lerp8(r00, r10, tx);
        const unsigned int g0 = lerp8(g00, g10, tx);
        const unsigned int b0 = lerp8(b00, b10, tx);

        const unsigned int a1 = lerp8(a01, a11, tx);
        const unsigned int r1 = lerp8(r01, r11, tx);
        const unsigned int g1 = lerp8(g01, g11, tx);
        const unsigned int b1 = lerp8(b01, b11, tx);

        const unsigned int a = lerp8(a0, a1, ty);
        const unsigned int r = lerp8(r0, r1, ty);
        const unsigned int g = lerp8(g0, g1, ty);
        const unsigned int b = lerp8(b0, b1, ty);

        return (a << 24) | (r << 16) | (g << 8) | b;
    }
}

HRESULT WINAPI
D3DXFilterTexture(
    LPDIRECT3DBASETEXTURE8 pBaseTexture,
    CONST PALETTEENTRY *pPalette,
    UINT SrcLevel,
    DWORD Filter)
{
    HRESULT hr = D3DERR_INVALIDCALL;
    if (SrcLevel == D3DX_DEFAULT)
    {
        SrcLevel = 0;
    }
    else if (SrcLevel >= pBaseTexture->GetLevelCount())
    {
        return D3DERR_INVALIDCALL;
    }

    D3DRESOURCETYPE type;
    switch (type = pBaseTexture->GetType())
    {
    case D3DRTYPE_TEXTURE:
    {
        IDirect3DTexture8 *tex = (IDirect3DTexture8 *)pBaseTexture;
        IDirect3DSurface8 *topsurf, *mipsurf;
        D3DSURFACE_DESC desc;
        int i;

        tex->GetLevelDesc(SrcLevel, &desc);
        if (Filter == D3DX_DEFAULT)
        {
            Filter = D3DX_FILTER_BOX;
        }

        int Level = SrcLevel + 1;
        hr = tex->GetSurfaceLevel(SrcLevel, &topsurf);
        if (FAILED(hr))
        {
            return hr;
        }

        while (tex->GetSurfaceLevel(Level, &mipsurf) == D3D_OK)
        {
            // Copy the data
            D3DXLoadSurfaceFromSurface(mipsurf, NULL, NULL, topsurf, NULL, NULL, Filter, 0);

            // Release the surface
            mipsurf->Release();
            topsurf = mipsurf;

            Level++;
        }

        topsurf->Release();
        if (FAILED(hr))
        {
            return hr;
        }
    }
        return D3D_OK;
    }

    return D3D_OK;
}

HRESULT WINAPI
D3DXLoadSurfaceFromSurface(
    LPDIRECT3DSURFACE8 pDestSurface,
    CONST PALETTEENTRY *pDestPalette,
    CONST RECT *pDestRect,
    LPDIRECT3DSURFACE8 pSrcSurface,
    CONST PALETTEENTRY *pSrcPalette,
    CONST RECT *pSrcRect,
    DWORD Filter,
    D3DCOLOR ColorKey)
{
    (void)pDestPalette;
    (void)pSrcPalette;
    (void)ColorKey;

    if (pDestSurface == NULL || pSrcSurface == NULL)
    {
        return D3DERR_INVALIDCALL;
    }

    D3DSURFACE_DESC descSrc;
    D3DSURFACE_DESC descDest;

    // DEBUG_ASSERTCRASH(pDestPalette == NULL);

    pSrcSurface->GetDesc(&descSrc);
    pDestSurface->GetDesc(&descDest);

    if (descSrc.Format != descDest.Format)
    {
        // Currently we only support scaling between formats of the same type
        return D3DERR_INVALIDCALL;
    }

    if (descSrc.Format != D3DFMT_A8R8G8B8 && descSrc.Format != D3DFMT_X8R8G8B8 && descSrc.Format != D3DFMT_A1R5G5B5)
    {
        return D3DERR_INVALIDCALL;
    }

    RECT defaultSrcRect = {0, 0, static_cast<LONG>(descSrc.Width), static_cast<LONG>(descSrc.Height)};
    RECT defaultDstRect = {0, 0, static_cast<LONG>(descDest.Width), static_cast<LONG>(descDest.Height)};
    const RECT &srcRectIn = pSrcRect ? *pSrcRect : defaultSrcRect;
    const RECT &dstRectIn = pDestRect ? *pDestRect : defaultDstRect;

    const int srcW = static_cast<int>(srcRectIn.right - srcRectIn.left);
    const int srcH = static_cast<int>(srcRectIn.bottom - srcRectIn.top);
    const int dstW = static_cast<int>(dstRectIn.right - dstRectIn.left);
    const int dstH = static_cast<int>(dstRectIn.bottom - dstRectIn.top);

    if (srcW <= 0 || srcH <= 0 || dstW <= 0 || dstH <= 0)
    {
        return D3DERR_INVALIDCALL;
    }

    D3DLOCKED_RECT srcRect;
    if (FAILED(pSrcSurface->LockRect(&srcRect, NULL, 0)))
    {
        return D3DERR_INVALIDCALL;
    }

    D3DLOCKED_RECT destRect;
    if (FAILED(pDestSurface->LockRect(&destRect, NULL, 0)))
    {
        pSrcSurface->UnlockRect();
        return D3DERR_INVALIDCALL;
    }

    // Fast path: No scaling needs to be done if the dimensions are the same
    if (dstW == srcW && dstH == srcH)
    {
        const int bpp = (descSrc.Format == D3DFMT_A1R5G5B5) ? 2 : 4;
        for (int y = 0; y < dstH; ++y)
        {
            const unsigned char *srcLine = static_cast<const unsigned char *>(srcRect.pBits) +
                                           (srcRectIn.top + y) * srcRect.Pitch + srcRectIn.left * bpp;
            unsigned char *dstLine = static_cast<unsigned char *>(destRect.pBits) +
                                     (dstRectIn.top + y) * destRect.Pitch + dstRectIn.left * bpp;
            memcpy(dstLine, srcLine, static_cast<size_t>(dstW * bpp));
        }
        pDestSurface->UnlockRect();
        pSrcSurface->UnlockRect();
        return D3D_OK;
    }

    const bool bilinear = (Filter == D3DX_FILTER_BOX || Filter == D3DX_FILTER_TRIANGLE || Filter == D3DX_DEFAULT);
    const float sxScale = static_cast<float>(srcW) / static_cast<float>(dstW);
    const float syScale = static_cast<float>(srcH) / static_cast<float>(dstH);

    if (descSrc.Format == D3DFMT_A1R5G5B5)
    {
        for (int y = 0; y < dstH; ++y)
        {
            unsigned short *dstLine = reinterpret_cast<unsigned short *>(
                                          static_cast<unsigned char *>(destRect.pBits) + (dstRectIn.top + y) * destRect.Pitch) +
                                      dstRectIn.left;

            const int srcY = srcRectIn.top + static_cast<int>((static_cast<long long>(y) * srcH) / dstH);
            const unsigned short *srcBase = reinterpret_cast<const unsigned short *>(
                static_cast<const unsigned char *>(srcRect.pBits) + srcY * srcRect.Pitch);

            for (int x = 0; x < dstW; ++x)
            {
                const int srcX = srcRectIn.left + static_cast<int>((static_cast<long long>(x) * srcW) / dstW);
                dstLine[x] = sampleA1R5G5B5Nearest(srcBase, static_cast<int>(descSrc.Width), static_cast<int>(descSrc.Height), srcX, srcY);
            }
        }
    }
    else
    {
        for (int y = 0; y < dstH; ++y)
        {
            unsigned int *dstLine = reinterpret_cast<unsigned int *>(
                                        static_cast<unsigned char *>(destRect.pBits) + (dstRectIn.top + y) * destRect.Pitch) +
                                    dstRectIn.left;

            for (int x = 0; x < dstW; ++x)
            {
                const float srcFx = srcRectIn.left + (static_cast<float>(x) + 0.5f) * sxScale - 0.5f;
                const float srcFy = srcRectIn.top + (static_cast<float>(y) + 0.5f) * syScale - 0.5f;

                const unsigned int *srcBase = reinterpret_cast<const unsigned int *>(srcRect.pBits);
                if (bilinear)
                {
                    dstLine[x] = sampleA8R8G8B8Bilinear(srcBase, static_cast<int>(descSrc.Width), static_cast<int>(descSrc.Height), srcFx, srcFy);
                }
                else
                {
                    dstLine[x] = sampleA8R8G8B8Nearest(srcBase, static_cast<int>(descSrc.Width), static_cast<int>(descSrc.Height), static_cast<int>(srcFx + 0.5f), static_cast<int>(srcFy + 0.5f));
                }
            }
        }
    }

    pDestSurface->UnlockRect();
    pSrcSurface->UnlockRect();

    return D3D_OK;
}

HRESULT WINAPI
D3DXCreateTextureFromFileExA(
    LPDIRECT3DDEVICE8 pDevice,
    LPCSTR pSrcFile,
    UINT Width,
    UINT Height,
    UINT MipLevels,
    DWORD Usage,
    D3DFORMAT Format,
    D3DPOOL Pool,

    DWORD Filter,
    DWORD MipFilter,
    D3DCOLOR ColorKey,
    D3DXIMAGE_INFO *pSrcInfo,
    PALETTEENTRY *pPalette,

    LPDIRECT3DTEXTURE8 *ppTexture)
{
    HRESULT hr;

    return D3DERR_INVALIDCALL;
}

HRESULT WINAPI
D3DXCreateTexture(LPDIRECT3DDEVICE8 pDevice,
	UINT Width,
	UINT Height,
	UINT MipLevels,
	DWORD Usage,
	D3DFORMAT Format,
	D3DPOOL Pool,
	LPDIRECT3DTEXTURE8 *ppTexture)
{
	return pDevice->CreateTexture(Width, Height, MipLevels, Usage, Format, Pool, ppTexture);
}
