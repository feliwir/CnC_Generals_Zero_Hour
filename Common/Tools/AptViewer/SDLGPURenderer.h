#pragma once

/**
 * Minimal SDL3 GPU renderer for the Apt viewer.
 *
 * Emulates the fixed-function state the original AptAuxPCOpenGL backend relied on:
 * an orthographic 2D stage, alpha blending, a color matrix transform
 * (color * mul + add), textured and solid triangles/lines, and stencil-based
 * masking (AptMaskRenderOperation_Add/Subtract increment/decrement the stencil
 * buffer; normal draws then only pass where stencil > 0).
 *
 * Geometry is uploaded once into static vertex buffers (Apt rendering units are
 * immutable), so no vertex data needs to be streamed during the render pass.
 * Buffer/texture uploads use their own command buffers and may happen at any
 * time, including between draws of the current frame.
 */

#include <SDL3/SDL.h>

class AptGpuRenderer
{
public:
    bool Init(SDL_Window *pWindow);
    void Shutdown();

    /// Starts a frame; returns false if no swapchain texture is available (e.g. minimized).
    bool BeginFrame(float fR, float fG, float fB, float fA);
    void EndFrame();

    /// Sets the stage rectangle inside the window (letterbox) and the ortho projection over it.
    void SetStage(int nX, int nY, int nWidth, int nHeight);

    SDL_GPUBuffer *CreateVertexBuffer(const float *pData, int nFloats);
    void DestroyVertexBuffer(SDL_GPUBuffer *pBuffer);

    /// pPixels is w*h RGBA8.
    SDL_GPUTexture *CreateTextureRGBA(const unsigned char *pPixels, int nWidth, int nHeight);
    void DestroyTexture(SDL_GPUTexture *pTexture);

    /// Solid-colored geometry; afColor is RGBA, already color-transformed.
    void DrawSolid(SDL_GPUBuffer *pBuffer, int nVertices, bool bLines,
                   const float afModel[16], const float afColor[4]);

    /// Textured triangles: frag = tex(uv) * colorMul + colorAdd, uv = (uvMatrix * pos).xy.
    void DrawTextured(SDL_GPUBuffer *pBuffer, int nVertices,
                      const float afModel[16], const float afUVMatrix[16],
                      const float afColorMul[4], const float afColorAdd[4],
                      SDL_GPUTexture *pTexture, bool bRepeat);

    /// Stencil-write pass for masks: increments (bAdd) or decrements the stencil buffer,
    /// no color output. Track the active mask count yourself and call SetMaskingEnabled.
    void DrawMask(SDL_GPUBuffer *pBuffer, int nVertices, bool bLines,
                  const float afModel[16], bool bAdd);

    /// When enabled, DrawSolid/DrawTextured only pass where the stencil buffer is > 0.
    void SetMaskingEnabled(bool bEnabled);

    /// Unit geometry for text fields: quad triangles over [0,1]x[0,1] and its outline as lines.
    SDL_GPUBuffer *GetUnitQuadTriangles() const { return mpUnitQuadTris; }
    SDL_GPUBuffer *GetUnitQuadOutline() const { return mpUnitQuadOutline; }

    int GetStageWidth() const { return mnStageWidth; }
    int GetStageHeight() const { return mnStageHeight; }

private:
    enum Pipeline
    {
        Pipeline_SolidTri = 0,
        Pipeline_SolidTriMasked,
        Pipeline_SolidLine,
        Pipeline_SolidLineMasked,
        Pipeline_TexturedTri,
        Pipeline_TexturedTriMasked,
        Pipeline_MaskAddTri,
        Pipeline_MaskSubTri,
        Pipeline_MaskAddLine,
        Pipeline_MaskSubLine,
        Pipeline_Count
    };

    bool CreateShaders();
    bool CreatePipelines();
    bool EnsureDepthStencilTexture();
    bool EnsureRenderPass();
    void EndRenderPass();
    void PushUniforms(const float afModel[16], const float afUVMatrix[16],
                      const float afColorMul[4], const float afColorAdd[4]);

    SDL_Window *mpWindow = nullptr;
    SDL_GPUDevice *mpDevice = nullptr;

    SDL_GPUShader *mpVertexShader = nullptr;
    SDL_GPUShader *mpSolidFragmentShader = nullptr;
    SDL_GPUShader *mpTexturedFragmentShader = nullptr;
    SDL_GPUGraphicsPipeline *mapPipelines[Pipeline_Count] = {};
    SDL_GPUSampler *mpSamplerRepeat = nullptr;
    SDL_GPUSampler *mpSamplerClamp = nullptr;
    SDL_GPUTexture *mpDepthStencil = nullptr;
    SDL_GPUTextureFormat meDepthStencilFormat = SDL_GPU_TEXTUREFORMAT_INVALID;
    int mnDepthStencilWidth = 0;
    int mnDepthStencilHeight = 0;

    SDL_GPUBuffer *mpUnitQuadTris = nullptr;
    SDL_GPUBuffer *mpUnitQuadOutline = nullptr;

    // per-frame state
    SDL_GPUCommandBuffer *mpCommandBuffer = nullptr;
    SDL_GPURenderPass *mpRenderPass = nullptr;
    SDL_GPUTexture *mpSwapchainTexture = nullptr;
    unsigned int mnSwapchainWidth = 0;
    unsigned int mnSwapchainHeight = 0;
    bool mbFrameCleared = false;
    float mafClearColor[4] = {0, 0, 0, 1};

    bool mbMaskingEnabled = false;
    int mnStageX = 0;
    int mnStageY = 0;
    int mnStageWidth = 0;
    int mnStageHeight = 0;
    float mafProjection[16] = {};
};
