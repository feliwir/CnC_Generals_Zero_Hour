#include "SDLGPURenderer.h"

#include <glslang/Public/ShaderLang.h>
#include <glslang/Public/ResourceLimits.h>
#include <glslang/SPIRV/GlslangToSpv.h>

#include <stdio.h>
#include <string.h>
#include <vector>

/*
 * SDL3 GPU resource-binding convention for SPIR-V shaders:
 *  - vertex uniform buffers:   set 1
 *  - fragment sampled textures: set 2
 *  - fragment uniform buffers: set 3
 */
static const char *kVertexShaderSource = R"(
#version 450
layout(location = 0) in vec2 inPos;
layout(location = 0) out vec2 outUV;
layout(set = 1, binding = 0) uniform VertexUniforms
{
    mat4 uMVP;
    mat4 uUVMatrix;
};
void main()
{
    gl_Position = uMVP * vec4(inPos, 0.0, 1.0);
    outUV = (uUVMatrix * vec4(inPos, 0.0, 1.0)).xy;
}
)";

static const char *kSolidFragmentShaderSource = R"(
#version 450
layout(location = 0) in vec2 inUV;
layout(location = 0) out vec4 outColor;
layout(set = 3, binding = 0) uniform FragmentUniforms
{
    vec4 uColorMul;
    vec4 uColorAdd;
};
void main()
{
    outColor = clamp(uColorMul, 0.0, 1.0);
}
)";

static const char *kTexturedFragmentShaderSource = R"(
#version 450
layout(location = 0) in vec2 inUV;
layout(location = 0) out vec4 outColor;
layout(set = 2, binding = 0) uniform sampler2D uTexture;
layout(set = 3, binding = 0) uniform FragmentUniforms
{
    vec4 uColorMul;
    vec4 uColorAdd;
};
void main()
{
    vec4 c = texture(uTexture, inUV) * uColorMul + uColorAdd;
    // mirrors glAlphaFunc(GL_GEQUAL, colorAdd.a) of the OpenGL implementation
    if (c.a < uColorAdd.a)
    {
        discard;
    }
    outColor = clamp(c, 0.0, 1.0);
}
)";

// APTVIEWER_TEXDEBUG: visualizes the interpolated uv instead of sampling
static const char *kUVDebugFragmentShaderSource = R"(
#version 450
layout(location = 0) in vec2 inUV;
layout(location = 0) out vec4 outColor;
layout(set = 2, binding = 0) uniform sampler2D uTexture;
layout(set = 3, binding = 0) uniform FragmentUniforms
{
    vec4 uColorMul;
    vec4 uColorAdd;
};
void main()
{
    outColor = vec4(fract(inUV.x), fract(inUV.y), (inUV.x < 0.0 || inUV.x > 1.0 || inUV.y < 0.0 || inUV.y > 1.0) ? 1.0 : 0.0, 1.0);
}
)";

struct VertexUniforms
{
    float afMVP[16];
    float afUVMatrix[16];
};

struct FragmentUniforms
{
    float afColorMul[4];
    float afColorAdd[4];
};

static void MatIdentity(float m[16])
{
    memset(m, 0, sizeof(float) * 16);
    m[0] = m[5] = m[10] = m[15] = 1.f;
}

// column-major, matches the OpenGL conventions of the original aux library
static void MatMultiply(float out[16], const float a[16], const float b[16])
{
    float r[16];
    for (int col = 0; col < 4; col++)
    {
        for (int row = 0; row < 4; row++)
        {
            r[col * 4 + row] = a[0 * 4 + row] * b[col * 4 + 0] +
                               a[1 * 4 + row] * b[col * 4 + 1] +
                               a[2 * 4 + row] * b[col * 4 + 2] +
                               a[3 * 4 + row] * b[col * 4 + 3];
        }
    }
    memcpy(out, r, sizeof(r));
}

static void MatOrtho(float m[16], float fLeft, float fRight, float fBottom, float fTop)
{
    memset(m, 0, sizeof(float) * 16);
    m[0] = 2.f / (fRight - fLeft);
    m[5] = 2.f / (fTop - fBottom);
    m[10] = 1.f;
    m[12] = -(fRight + fLeft) / (fRight - fLeft);
    m[13] = -(fTop + fBottom) / (fTop - fBottom);
    m[15] = 1.f;
}

static bool CompileGLSL(EShLanguage eStage, const char *szSource, std::vector<Uint32> &spirv)
{
    glslang::TShader shader(eStage);
    shader.setStrings(&szSource, 1);
    shader.setEnvInput(glslang::EShSourceGlsl, eStage, glslang::EShClientVulkan, 100);
    shader.setEnvClient(glslang::EShClientVulkan, glslang::EShTargetVulkan_1_0);
    shader.setEnvTarget(glslang::EShTargetSpv, glslang::EShTargetSpv_1_0);

    if (!shader.parse(GetDefaultResources(), 100, false, EShMsgDefault))
    {
        fprintf(stderr, "AptViewer: shader compile failed:\n%s\n", shader.getInfoLog());
        return false;
    }

    glslang::TProgram program;
    program.addShader(&shader);
    if (!program.link(EShMsgDefault))
    {
        fprintf(stderr, "AptViewer: shader link failed:\n%s\n", program.getInfoLog());
        return false;
    }

    std::vector<unsigned int> words;
    glslang::GlslangToSpv(*program.getIntermediate(eStage), words);
    spirv.assign(words.begin(), words.end());
    return true;
}

bool AptGpuRenderer::Init(SDL_Window *pWindow)
{
    mpWindow = pWindow;

    mpDevice = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV, SDL_getenv("APTVIEWER_DEBUG") != NULL, NULL);
    if (!mpDevice)
    {
        fprintf(stderr, "AptViewer: SDL_CreateGPUDevice failed: %s\n", SDL_GetError());
        return false;
    }
    if (!SDL_ClaimWindowForGPUDevice(mpDevice, mpWindow))
    {
        fprintf(stderr, "AptViewer: SDL_ClaimWindowForGPUDevice failed: %s\n", SDL_GetError());
        return false;
    }

    if (SDL_GPUTextureSupportsFormat(mpDevice, SDL_GPU_TEXTUREFORMAT_D24_UNORM_S8_UINT,
                                     SDL_GPU_TEXTURETYPE_2D, SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET))
    {
        meDepthStencilFormat = SDL_GPU_TEXTUREFORMAT_D24_UNORM_S8_UINT;
    }
    else
    {
        meDepthStencilFormat = SDL_GPU_TEXTUREFORMAT_D32_FLOAT_S8_UINT;
    }

    if (!CreateShaders() || !CreatePipelines())
    {
        return false;
    }

    SDL_GPUSamplerCreateInfo samplerInfo = {};
    samplerInfo.min_filter = SDL_GPU_FILTER_LINEAR;
    samplerInfo.mag_filter = SDL_GPU_FILTER_LINEAR;
    samplerInfo.mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_NEAREST;
    samplerInfo.address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_REPEAT;
    samplerInfo.address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_REPEAT;
    samplerInfo.address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_REPEAT;
    mpSamplerRepeat = SDL_CreateGPUSampler(mpDevice, &samplerInfo);
    samplerInfo.address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
    samplerInfo.address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
    samplerInfo.address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
    mpSamplerClamp = SDL_CreateGPUSampler(mpDevice, &samplerInfo);
    if (!mpSamplerRepeat || !mpSamplerClamp)
    {
        fprintf(stderr, "AptViewer: SDL_CreateGPUSampler failed: %s\n", SDL_GetError());
        return false;
    }

    static const float afUnitQuadTris[] = {
        0, 0, 1, 0, 1, 1,
        0, 0, 1, 1, 0, 1,
    };
    static const float afUnitQuadOutline[] = {
        0, 0, 1, 0,
        1, 0, 1, 1,
        1, 1, 0, 1,
        0, 1, 0, 0,
    };
    mpUnitQuadTris = CreateVertexBuffer(afUnitQuadTris, sizeof(afUnitQuadTris) / sizeof(float));
    mpUnitQuadOutline = CreateVertexBuffer(afUnitQuadOutline, sizeof(afUnitQuadOutline) / sizeof(float));

    MatIdentity(mafProjection);
    return mpUnitQuadTris && mpUnitQuadOutline;
}

void AptGpuRenderer::Shutdown()
{
    if (!mpDevice)
    {
        return;
    }
    SDL_WaitForGPUIdle(mpDevice);
    DestroyVertexBuffer(mpUnitQuadTris);
    DestroyVertexBuffer(mpUnitQuadOutline);
    for (int i = 0; i < Pipeline_Count; i++)
    {
        if (mapPipelines[i])
        {
            SDL_ReleaseGPUGraphicsPipeline(mpDevice, mapPipelines[i]);
        }
    }
    if (mpVertexShader) SDL_ReleaseGPUShader(mpDevice, mpVertexShader);
    if (mpSolidFragmentShader) SDL_ReleaseGPUShader(mpDevice, mpSolidFragmentShader);
    if (mpTexturedFragmentShader) SDL_ReleaseGPUShader(mpDevice, mpTexturedFragmentShader);
    if (mpSamplerRepeat) SDL_ReleaseGPUSampler(mpDevice, mpSamplerRepeat);
    if (mpSamplerClamp) SDL_ReleaseGPUSampler(mpDevice, mpSamplerClamp);
    if (mpDepthStencil) SDL_ReleaseGPUTexture(mpDevice, mpDepthStencil);
    SDL_ReleaseWindowFromGPUDevice(mpDevice, mpWindow);
    SDL_DestroyGPUDevice(mpDevice);
    mpDevice = nullptr;
    glslang::FinalizeProcess();
}

bool AptGpuRenderer::CreateShaders()
{
    glslang::InitializeProcess();

    struct
    {
        EShLanguage eStage;
        const char *szSource;
        SDL_GPUShaderStage eSDLStage;
        Uint32 nSamplers;
        SDL_GPUShader **ppShader;
    } aShaders[] = {
        { EShLangVertex, kVertexShaderSource, SDL_GPU_SHADERSTAGE_VERTEX, 0, &mpVertexShader },
        { EShLangFragment, kSolidFragmentShaderSource, SDL_GPU_SHADERSTAGE_FRAGMENT, 0, &mpSolidFragmentShader },
        { EShLangFragment,
          SDL_getenv("APTVIEWER_TEXDEBUG") ? kUVDebugFragmentShaderSource : kTexturedFragmentShaderSource,
          SDL_GPU_SHADERSTAGE_FRAGMENT, 1, &mpTexturedFragmentShader },
    };

    for (auto &entry : aShaders)
    {
        std::vector<Uint32> spirv;
        if (!CompileGLSL(entry.eStage, entry.szSource, spirv))
        {
            return false;
        }
        SDL_GPUShaderCreateInfo info = {};
        info.code = (const Uint8 *)spirv.data();
        info.code_size = spirv.size() * sizeof(Uint32);
        info.entrypoint = "main";
        info.format = SDL_GPU_SHADERFORMAT_SPIRV;
        info.stage = entry.eSDLStage;
        info.num_samplers = entry.nSamplers;
        info.num_uniform_buffers = 1;
        *entry.ppShader = SDL_CreateGPUShader(mpDevice, &info);
        if (!*entry.ppShader)
        {
            fprintf(stderr, "AptViewer: SDL_CreateGPUShader failed: %s\n", SDL_GetError());
            return false;
        }
    }
    return true;
}

bool AptGpuRenderer::CreatePipelines()
{
    SDL_GPUVertexBufferDescription vertexBuffer = {};
    vertexBuffer.slot = 0;
    vertexBuffer.pitch = 2 * sizeof(float);
    vertexBuffer.input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX;

    SDL_GPUVertexAttribute vertexAttribute = {};
    vertexAttribute.location = 0;
    vertexAttribute.buffer_slot = 0;
    vertexAttribute.format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2;
    vertexAttribute.offset = 0;

    SDL_GPUColorTargetDescription colorTarget = {};
    colorTarget.format = SDL_GetGPUSwapchainTextureFormat(mpDevice, mpWindow);
    colorTarget.blend_state.enable_blend = true;
    colorTarget.blend_state.src_color_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA;
    colorTarget.blend_state.dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
    colorTarget.blend_state.color_blend_op = SDL_GPU_BLENDOP_ADD;
    colorTarget.blend_state.src_alpha_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA;
    colorTarget.blend_state.dst_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
    colorTarget.blend_state.alpha_blend_op = SDL_GPU_BLENDOP_ADD;

    for (int i = 0; i < Pipeline_Count; i++)
    {
        const bool bLines = (i == Pipeline_SolidLine || i == Pipeline_SolidLineMasked ||
                             i == Pipeline_MaskAddLine || i == Pipeline_MaskSubLine);
        const bool bTextured = (i == Pipeline_TexturedTri || i == Pipeline_TexturedTriMasked);
        const bool bMasked = (i == Pipeline_SolidTriMasked || i == Pipeline_SolidLineMasked ||
                              i == Pipeline_TexturedTriMasked);
        const bool bMaskWrite = (i == Pipeline_MaskAddTri || i == Pipeline_MaskSubTri ||
                                 i == Pipeline_MaskAddLine || i == Pipeline_MaskSubLine);
        const bool bMaskAdd = (i == Pipeline_MaskAddTri || i == Pipeline_MaskAddLine);

        SDL_GPUColorTargetDescription target = colorTarget;
        if (bMaskWrite)
        {
            // stencil-only: no color output
            target.blend_state.enable_color_write_mask = true;
            target.blend_state.color_write_mask = 0;
        }

        SDL_GPUGraphicsPipelineCreateInfo info = {};
        info.vertex_shader = mpVertexShader;
        info.fragment_shader = bTextured ? mpTexturedFragmentShader : mpSolidFragmentShader;
        info.vertex_input_state.vertex_buffer_descriptions = &vertexBuffer;
        info.vertex_input_state.num_vertex_buffers = 1;
        info.vertex_input_state.vertex_attributes = &vertexAttribute;
        info.vertex_input_state.num_vertex_attributes = 1;
        info.primitive_type = bLines ? SDL_GPU_PRIMITIVETYPE_LINELIST : SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
        info.rasterizer_state.fill_mode = SDL_GPU_FILLMODE_FILL;
        info.rasterizer_state.cull_mode = SDL_GPU_CULLMODE_NONE;
        info.target_info.color_target_descriptions = &target;
        info.target_info.num_color_targets = 1;
        info.target_info.depth_stencil_format = meDepthStencilFormat;
        info.target_info.has_depth_stencil_target = true;

        SDL_GPUDepthStencilState &ds = info.depth_stencil_state;
        ds.enable_depth_test = false;
        ds.enable_depth_write = false;
        if (bMaskWrite)
        {
            // Always fail the stencil test and use the fail op to inc/dec the buffer,
            // mirroring glStencilFunc(GL_NEVER)/glStencilOp(GL_INCR or GL_DECR).
            ds.enable_stencil_test = true;
            SDL_GPUStencilOpState op = {};
            op.compare_op = SDL_GPU_COMPAREOP_NEVER;
            op.fail_op = bMaskAdd ? SDL_GPU_STENCILOP_INCREMENT_AND_CLAMP
                                  : SDL_GPU_STENCILOP_DECREMENT_AND_CLAMP;
            op.pass_op = op.fail_op;
            op.depth_fail_op = op.fail_op;
            ds.front_stencil_state = op;
            ds.back_stencil_state = op;
            ds.compare_mask = 0xFF;
            ds.write_mask = 0xFF;
        }
        else if (bMasked)
        {
            // glStencilFunc(GL_LESS, 0, 0xFF): pass where reference (0) < stencil
            ds.enable_stencil_test = true;
            SDL_GPUStencilOpState op = {};
            op.compare_op = SDL_GPU_COMPAREOP_LESS;
            op.fail_op = SDL_GPU_STENCILOP_KEEP;
            op.pass_op = SDL_GPU_STENCILOP_KEEP;
            op.depth_fail_op = SDL_GPU_STENCILOP_KEEP;
            ds.front_stencil_state = op;
            ds.back_stencil_state = op;
            ds.compare_mask = 0xFF;
            ds.write_mask = 0;
        }

        mapPipelines[i] = SDL_CreateGPUGraphicsPipeline(mpDevice, &info);
        if (!mapPipelines[i])
        {
            fprintf(stderr, "AptViewer: SDL_CreateGPUGraphicsPipeline failed: %s\n", SDL_GetError());
            return false;
        }
    }
    return true;
}

SDL_GPUBuffer *AptGpuRenderer::CreateVertexBuffer(const float *pData, int nFloats)
{
    const Uint32 nBytes = (Uint32)(nFloats * sizeof(float));

    SDL_GPUBufferCreateInfo bufferInfo = {};
    bufferInfo.usage = SDL_GPU_BUFFERUSAGE_VERTEX;
    bufferInfo.size = nBytes;
    SDL_GPUBuffer *pBuffer = SDL_CreateGPUBuffer(mpDevice, &bufferInfo);
    if (!pBuffer)
    {
        return nullptr;
    }

    SDL_GPUTransferBufferCreateInfo transferInfo = {};
    transferInfo.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    transferInfo.size = nBytes;
    SDL_GPUTransferBuffer *pTransfer = SDL_CreateGPUTransferBuffer(mpDevice, &transferInfo);
    void *pMapped = SDL_MapGPUTransferBuffer(mpDevice, pTransfer, false);
    memcpy(pMapped, pData, nBytes);
    SDL_UnmapGPUTransferBuffer(mpDevice, pTransfer);

    // uploads run on their own command buffer so they can happen at any point of the frame
    SDL_GPUCommandBuffer *pCmd = SDL_AcquireGPUCommandBuffer(mpDevice);
    SDL_GPUCopyPass *pCopy = SDL_BeginGPUCopyPass(pCmd);
    SDL_GPUTransferBufferLocation src = { pTransfer, 0 };
    SDL_GPUBufferRegion dst = { pBuffer, 0, nBytes };
    SDL_UploadToGPUBuffer(pCopy, &src, &dst, false);
    SDL_EndGPUCopyPass(pCopy);
    SDL_SubmitGPUCommandBuffer(pCmd);
    SDL_ReleaseGPUTransferBuffer(mpDevice, pTransfer);
    return pBuffer;
}

void AptGpuRenderer::DestroyVertexBuffer(SDL_GPUBuffer *pBuffer)
{
    if (pBuffer)
    {
        SDL_ReleaseGPUBuffer(mpDevice, pBuffer);
    }
}

SDL_GPUTexture *AptGpuRenderer::CreateTextureRGBA(const unsigned char *pPixels, int nWidth, int nHeight)
{
    const Uint32 nBytes = (Uint32)(nWidth * nHeight * 4);

    SDL_GPUTextureCreateInfo textureInfo = {};
    textureInfo.type = SDL_GPU_TEXTURETYPE_2D;
    textureInfo.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
    textureInfo.usage = SDL_GPU_TEXTUREUSAGE_SAMPLER;
    textureInfo.width = (Uint32)nWidth;
    textureInfo.height = (Uint32)nHeight;
    textureInfo.layer_count_or_depth = 1;
    textureInfo.num_levels = 1;
    SDL_GPUTexture *pTexture = SDL_CreateGPUTexture(mpDevice, &textureInfo);
    if (!pTexture)
    {
        return nullptr;
    }

    SDL_GPUTransferBufferCreateInfo transferInfo = {};
    transferInfo.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    transferInfo.size = nBytes;
    SDL_GPUTransferBuffer *pTransfer = SDL_CreateGPUTransferBuffer(mpDevice, &transferInfo);
    void *pMapped = SDL_MapGPUTransferBuffer(mpDevice, pTransfer, false);
    memcpy(pMapped, pPixels, nBytes);
    SDL_UnmapGPUTransferBuffer(mpDevice, pTransfer);

    SDL_GPUCommandBuffer *pCmd = SDL_AcquireGPUCommandBuffer(mpDevice);
    SDL_GPUCopyPass *pCopy = SDL_BeginGPUCopyPass(pCmd);
    SDL_GPUTextureTransferInfo src = {};
    src.transfer_buffer = pTransfer;
    SDL_GPUTextureRegion dst = {};
    dst.texture = pTexture;
    dst.w = (Uint32)nWidth;
    dst.h = (Uint32)nHeight;
    dst.d = 1;
    SDL_UploadToGPUTexture(pCopy, &src, &dst, false);
    SDL_EndGPUCopyPass(pCopy);
    SDL_SubmitGPUCommandBuffer(pCmd);
    SDL_ReleaseGPUTransferBuffer(mpDevice, pTransfer);
    return pTexture;
}

void AptGpuRenderer::DestroyTexture(SDL_GPUTexture *pTexture)
{
    if (pTexture)
    {
        SDL_ReleaseGPUTexture(mpDevice, pTexture);
    }
}

bool AptGpuRenderer::EnsureDepthStencilTexture()
{
    if (mpDepthStencil &&
        mnDepthStencilWidth == (int)mnSwapchainWidth &&
        mnDepthStencilHeight == (int)mnSwapchainHeight)
    {
        return true;
    }
    if (mpDepthStencil)
    {
        SDL_ReleaseGPUTexture(mpDevice, mpDepthStencil);
        mpDepthStencil = nullptr;
    }
    SDL_GPUTextureCreateInfo info = {};
    info.type = SDL_GPU_TEXTURETYPE_2D;
    info.format = meDepthStencilFormat;
    info.usage = SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET;
    info.width = mnSwapchainWidth;
    info.height = mnSwapchainHeight;
    info.layer_count_or_depth = 1;
    info.num_levels = 1;
    mpDepthStencil = SDL_CreateGPUTexture(mpDevice, &info);
    mnDepthStencilWidth = (int)mnSwapchainWidth;
    mnDepthStencilHeight = (int)mnSwapchainHeight;
    return mpDepthStencil != nullptr;
}

bool AptGpuRenderer::BeginFrame(float fR, float fG, float fB, float fA)
{
    mpCommandBuffer = SDL_AcquireGPUCommandBuffer(mpDevice);
    if (!mpCommandBuffer)
    {
        return false;
    }
    if (!SDL_WaitAndAcquireGPUSwapchainTexture(mpCommandBuffer, mpWindow,
                                               &mpSwapchainTexture, &mnSwapchainWidth, &mnSwapchainHeight) ||
        !mpSwapchainTexture)
    {
        SDL_CancelGPUCommandBuffer(mpCommandBuffer);
        mpCommandBuffer = nullptr;
        return false;
    }
    mafClearColor[0] = fR;
    mafClearColor[1] = fG;
    mafClearColor[2] = fB;
    mafClearColor[3] = fA;
    mbFrameCleared = false;
    return EnsureDepthStencilTexture();
}

bool AptGpuRenderer::EnsureRenderPass()
{
    if (mpRenderPass)
    {
        return true;
    }
    if (!mpCommandBuffer || !mpSwapchainTexture)
    {
        return false;
    }

    SDL_GPUColorTargetInfo colorInfo = {};
    colorInfo.texture = mpSwapchainTexture;
    colorInfo.load_op = mbFrameCleared ? SDL_GPU_LOADOP_LOAD : SDL_GPU_LOADOP_CLEAR;
    colorInfo.store_op = SDL_GPU_STOREOP_STORE;
    colorInfo.clear_color.r = mafClearColor[0];
    colorInfo.clear_color.g = mafClearColor[1];
    colorInfo.clear_color.b = mafClearColor[2];
    colorInfo.clear_color.a = mafClearColor[3];

    SDL_GPUDepthStencilTargetInfo dsInfo = {};
    dsInfo.texture = mpDepthStencil;
    dsInfo.load_op = mbFrameCleared ? SDL_GPU_LOADOP_LOAD : SDL_GPU_LOADOP_CLEAR;
    dsInfo.store_op = SDL_GPU_STOREOP_STORE;
    dsInfo.stencil_load_op = mbFrameCleared ? SDL_GPU_LOADOP_LOAD : SDL_GPU_LOADOP_CLEAR;
    dsInfo.stencil_store_op = SDL_GPU_STOREOP_STORE;
    dsInfo.clear_depth = 0.f;
    dsInfo.clear_stencil = 0;

    mpRenderPass = SDL_BeginGPURenderPass(mpCommandBuffer, &colorInfo, 1, &dsInfo);
    if (!mpRenderPass)
    {
        return false;
    }
    mbFrameCleared = true;

    SDL_GPUViewport viewport = {};
    viewport.x = (float)mnStageX;
    viewport.y = (float)mnStageY;
    viewport.w = (float)(mnStageWidth > 0 ? mnStageWidth : (int)mnSwapchainWidth);
    viewport.h = (float)(mnStageHeight > 0 ? mnStageHeight : (int)mnSwapchainHeight);
    viewport.min_depth = 0.f;
    viewport.max_depth = 1.f;
    SDL_SetGPUViewport(mpRenderPass, &viewport);
    SDL_SetGPUStencilReference(mpRenderPass, 0);
    return true;
}

void AptGpuRenderer::EndRenderPass()
{
    if (mpRenderPass)
    {
        SDL_EndGPURenderPass(mpRenderPass);
        mpRenderPass = nullptr;
    }
}

void AptGpuRenderer::EndFrame()
{
    if (!mpCommandBuffer)
    {
        return;
    }
    // make sure the clear happens even if nothing was drawn
    EnsureRenderPass();
    EndRenderPass();
    SDL_SubmitGPUCommandBuffer(mpCommandBuffer);
    mpCommandBuffer = nullptr;
    mpSwapchainTexture = nullptr;
}

void AptGpuRenderer::SetStage(int nX, int nY, int nWidth, int nHeight)
{
    if (mnStageX != nX || mnStageY != nY || mnStageWidth != nWidth || mnStageHeight != nHeight)
    {
        // viewport is set at render-pass begin; force a new pass on change
        EndRenderPass();
        mnStageX = nX;
        mnStageY = nY;
        mnStageWidth = nWidth;
        mnStageHeight = nHeight;
    }
    MatOrtho(mafProjection, 0.f, (float)nWidth, (float)nHeight, 0.f);
}

void AptGpuRenderer::SetMaskingEnabled(bool bEnabled)
{
    mbMaskingEnabled = bEnabled;
}

void AptGpuRenderer::PushUniforms(const float afModel[16], const float afUVMatrix[16],
                                  const float afColorMul[4], const float afColorAdd[4])
{
    VertexUniforms vu;
    MatMultiply(vu.afMVP, mafProjection, afModel);
    if (afUVMatrix)
    {
        memcpy(vu.afUVMatrix, afUVMatrix, sizeof(vu.afUVMatrix));
    }
    else
    {
        MatIdentity(vu.afUVMatrix);
    }
    SDL_PushGPUVertexUniformData(mpCommandBuffer, 0, &vu, sizeof(vu));

    FragmentUniforms fu = {};
    memcpy(fu.afColorMul, afColorMul, sizeof(fu.afColorMul));
    if (afColorAdd)
    {
        memcpy(fu.afColorAdd, afColorAdd, sizeof(fu.afColorAdd));
    }
    SDL_PushGPUFragmentUniformData(mpCommandBuffer, 0, &fu, sizeof(fu));
}

void AptGpuRenderer::DrawSolid(SDL_GPUBuffer *pBuffer, int nVertices, bool bLines,
                               const float afModel[16], const float afColor[4])
{
    if (!pBuffer || nVertices <= 0 || !EnsureRenderPass())
    {
        return;
    }
    Pipeline ePipeline;
    if (bLines)
    {
        ePipeline = mbMaskingEnabled ? Pipeline_SolidLineMasked : Pipeline_SolidLine;
    }
    else
    {
        ePipeline = mbMaskingEnabled ? Pipeline_SolidTriMasked : Pipeline_SolidTri;
    }
    SDL_BindGPUGraphicsPipeline(mpRenderPass, mapPipelines[ePipeline]);
    SDL_GPUBufferBinding binding = { pBuffer, 0 };
    SDL_BindGPUVertexBuffers(mpRenderPass, 0, &binding, 1);
    PushUniforms(afModel, nullptr, afColor, nullptr);
    SDL_DrawGPUPrimitives(mpRenderPass, (Uint32)nVertices, 1, 0, 0);
}

void AptGpuRenderer::DrawTextured(SDL_GPUBuffer *pBuffer, int nVertices,
                                  const float afModel[16], const float afUVMatrix[16],
                                  const float afColorMul[4], const float afColorAdd[4],
                                  SDL_GPUTexture *pTexture, bool bRepeat)
{
    if (!pBuffer || !pTexture || nVertices <= 0 || !EnsureRenderPass())
    {
        return;
    }
    SDL_BindGPUGraphicsPipeline(mpRenderPass,
                                mapPipelines[mbMaskingEnabled ? Pipeline_TexturedTriMasked : Pipeline_TexturedTri]);
    SDL_GPUBufferBinding binding = { pBuffer, 0 };
    SDL_BindGPUVertexBuffers(mpRenderPass, 0, &binding, 1);
    SDL_GPUTextureSamplerBinding textureBinding = {};
    textureBinding.texture = pTexture;
    textureBinding.sampler = bRepeat ? mpSamplerRepeat : mpSamplerClamp;
    SDL_BindGPUFragmentSamplers(mpRenderPass, 0, &textureBinding, 1);
    PushUniforms(afModel, afUVMatrix, afColorMul, afColorAdd);
    SDL_DrawGPUPrimitives(mpRenderPass, (Uint32)nVertices, 1, 0, 0);
}

void AptGpuRenderer::DrawMask(SDL_GPUBuffer *pBuffer, int nVertices, bool bLines,
                              const float afModel[16], bool bAdd)
{
    if (!pBuffer || nVertices <= 0 || !EnsureRenderPass())
    {
        return;
    }
    Pipeline ePipeline;
    if (bLines)
    {
        ePipeline = bAdd ? Pipeline_MaskAddLine : Pipeline_MaskSubLine;
    }
    else
    {
        ePipeline = bAdd ? Pipeline_MaskAddTri : Pipeline_MaskSubTri;
    }
    SDL_BindGPUGraphicsPipeline(mpRenderPass, mapPipelines[ePipeline]);
    SDL_GPUBufferBinding binding = { pBuffer, 0 };
    SDL_BindGPUVertexBuffers(mpRenderPass, 0, &binding, 1);
    static const float afWhite[4] = { 1, 1, 1, 1 };
    PushUniforms(afModel, nullptr, afWhite, nullptr);
    SDL_DrawGPUPrimitives(mpRenderPass, (Uint32)nVertices, 1, 0, 0);
}
