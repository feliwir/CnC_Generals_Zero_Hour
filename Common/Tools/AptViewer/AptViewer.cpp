/**
 * AptViewer - plays back .apt Flash-UI animations in an SDL3 window using the
 * SDL3 GPU API. Port of EA's pc_opengl_viewer without .big archive support:
 * all assets (<base>.apt/.const/.dat, <base>_geometry/, art/Textures/, *.ttf
 * fonts) are loose files in the animation's directory.
 *
 * Usage: AptViewer [--gremlins] <path/to/animation-base-name>
 */

#include "Apt.h"
#include "AptAuxSDLGPU.h"
#include "SDLGPURenderer.h"

#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include <filesystem>
#include <stdio.h>
#include <string.h>

static AptGpuRenderer gRenderer;
static AptAuxFontList gFonts;

struct ScriptCall
{
    const char *szName;
    long nFrame;
};
static ScriptCall gaScriptCalls[16];
static int gnScriptCalls = 0;
static SDL_Window *gpWindow;
static int gnWindowWidth = 640;
static int gnWindowHeight = 480;
static int gnStageX = 0;
static int gnStageY = 0;
static bool gbGremlins = false;

static void freeFontTable()
{
    for (int i = 0; i < gFonts.nFonts; i++)
    {
        free(gFonts.pData[i]);
        AptGetUserFuncs().pfnMemFree(gFonts.aszFontNamesEnglish[i]);
        AptGetUserFuncs().pfnMemFree(gFonts.aszFontNamesJapanese[i]);
    }
    gFonts.nFonts = 0;
}

static void loadFonts()
{
    gFonts.nFonts = 0;
    std::error_code ec;
    for (auto &entry : std::filesystem::directory_iterator(".", ec))
    {
        if (gFonts.nFonts >= (int)(sizeof(gFonts.pData) / sizeof(gFonts.pData[0])))
        {
            break;
        }
        if (!std::filesystem::is_regular_file(entry) || entry.path().extension() != ".ttf")
        {
            continue;
        }
        FILE *pIn = fopen(entry.path().c_str(), "rb");
        if (!pIn)
        {
            continue;
        }
        fseek(pIn, 0, SEEK_END);
        gFonts.nDataSize[gFonts.nFonts] = (int)ftell(pIn);
        gFonts.pData[gFonts.nFonts] = malloc(gFonts.nDataSize[gFonts.nFonts]);
        rewind(pIn);
        fread(gFonts.pData[gFonts.nFonts], gFonts.nDataSize[gFonts.nFonts], 1, pIn);
        fclose(pIn);
        gFonts.nFonts++;
    }
}

/// Recomputes the letterboxed stage rectangle inside the window.
static void updateStage()
{
    int nAnimWidth = 0, nAnimHeight = 0;
    AptGetAnimationSize(&nAnimWidth, &nAnimHeight);
    if (nAnimWidth <= 0 || nAnimHeight <= 0)
    {
        gnStageX = 0;
        gnStageY = 0;
        gRenderer.SetStage(0, 0, gnWindowWidth, gnWindowHeight);
        return;
    }
    gnStageX = (gnWindowWidth - nAnimWidth) / 2;
    gnStageY = (gnWindowHeight - nAnimHeight) / 2;
    gRenderer.SetStage(gnStageX, gnStageY, nAnimWidth, nAnimHeight);
}

/// Sizes the window to the animation once its dimensions are known.
static void resizeWindowToFitAnim()
{
    static bool sbResized = false;
    if (sbResized)
    {
        return;
    }
    int nAnimWidth = 0, nAnimHeight = 0;
    AptGetAnimationSize(&nAnimWidth, &nAnimHeight);
    if (nAnimWidth <= 0)
    {
        return;
    }
    sbResized = true;
    SDL_SetWindowSize(gpWindow, nAnimWidth, nAnimHeight);
    gnWindowWidth = nAnimWidth;
    gnWindowHeight = nAnimHeight;
}

static void doGremlins()
{
    extern unsigned int AptRand();
    int nNumInputs = AptRand() % 3;
    for (int i = 0; i < nNumInputs; i++)
    {
        if (AptRand() % 2)
        {
            int nController = AptRand() % AptInputController_NumControllers;
            AptAddToInputQueue((AptInputType)(AptRand() % AptInputType_NumInputs),
                               (AptInputState)(AptRand() % AptInputState_NumStates),
                               (AptInputController)nController);
        }
        else
        {
            int nW, nH;
            AptGetAnimationSize(&nW, &nH);
            if (nW && nH)
            {
                AptSetMousePosition(AptRand() % nW, AptRand() % nH);
            }
        }
    }
}

static void handleKey(const SDL_KeyboardEvent &key, bool bPressed)
{
    const AptInputState eState = bPressed ? AptInputState_Pressed : AptInputState_Released;

    switch (key.key)
    {
        case SDLK_LEFT: AptAddToInputQueue(AptInputType_Left, eState, AptInputController_Keyboard); return;
        case SDLK_RIGHT: AptAddToInputQueue(AptInputType_Right, eState, AptInputController_Keyboard); return;
        case SDLK_UP: AptAddToInputQueue(AptInputType_Up, eState, AptInputController_Keyboard); return;
        case SDLK_DOWN: AptAddToInputQueue(AptInputType_Down, eState, AptInputController_Keyboard); return;
        case SDLK_F1: AptAddToInputQueue(AptInputType_PadSelect, eState, AptInputController_Gamepad0); return;
        case SDLK_F2: AptAddToInputQueue(AptInputType_PadStart, eState, AptInputController_Gamepad0); return;
        case SDLK_F3: AptAddToInputQueue(AptInputType_PadCross, eState, AptInputController_Gamepad0); return;
        case SDLK_F4: AptAddToInputQueue(AptInputType_PadCircle, eState, AptInputController_Gamepad0); return;
        case SDLK_F5: AptAddToInputQueue(AptInputType_PadTriangle, eState, AptInputController_Gamepad0); return;
        case SDLK_F6: AptAddToInputQueue(AptInputType_PadSquare, eState, AptInputController_Gamepad0); return;
        case SDLK_F7: AptAddToInputQueue(AptInputType_PadL1, eState, AptInputController_Gamepad0); return;
        case SDLK_F8: AptAddToInputQueue(AptInputType_PadR1, eState, AptInputController_Gamepad0); return;
        case SDLK_F9: AptAddToInputQueue(AptInputType_PadL2, eState, AptInputController_Gamepad0); return;
        case SDLK_F10: AptAddToInputQueue(AptInputType_PadR2, eState, AptInputController_Gamepad0); return;
        default: break;
    }

    if (key.key == SDLK_SPACE || key.key == SDLK_RETURN)
    {
        AptAddToInputQueue(AptInputType_MouseButton0, eState, AptInputController_Keyboard);
    }
    if (key.key >= 32 && key.key <= 126)
    {
        AptAddToInputQueue((AptInputType)key.key, eState, AptInputController_Keyboard);
    }
}

int main(int argc, char **argv)
{
    const char *szFile = nullptr;
    long nMaxFrames = -1;
    for (int i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "--gremlins") == 0)
        {
            gbGremlins = true;
        }
        else if (strcmp(argv[i], "--frames") == 0 && i + 1 < argc)
        {
            nMaxFrames = atol(argv[++i]);
        }
        else if (strcmp(argv[i], "--extern") == 0 && i + 1 < argc)
        {
            char *szEquals = strchr(argv[++i], '=');
            if (szEquals)
            {
                *szEquals = 0;
                AptAuxSDLGPU_SetExternVariable(argv[i], szEquals + 1);
            }
        }
        else if (strcmp(argv[i], "--call") == 0 && i + 1 < argc && gnScriptCalls < (int)(sizeof(gaScriptCalls) / sizeof(gaScriptCalls[0])))
        {
            // --call <function>[@frame]: invokes an ActionScript function (path relative
            // to _level0) at the given frame, emulating a game->script callback
            char *szCall = argv[++i];
            char *szAt = strchr(szCall, '@');
            gaScriptCalls[gnScriptCalls].nFrame = 60;
            if (szAt)
            {
                *szAt = 0;
                gaScriptCalls[gnScriptCalls].nFrame = atol(szAt + 1);
            }
            gaScriptCalls[gnScriptCalls].szName = szCall;
            gnScriptCalls++;
        }
        else
        {
            szFile = argv[i];
        }
    }
    if (!szFile)
    {
        fprintf(stderr, "usage: %s [--gremlins] [--frames <n>] <path/to/animation-base-name>\n", argv[0]);
        fprintf(stderr, "example: %s Data/apt/32_coupled/AptLevel0\n", argv[0]);
        return 1;
    }

    // all assets (including imports) are resolved relative to the animation's directory
    std::filesystem::path filePath(szFile);
    if (filePath.has_parent_path())
    {
        std::error_code ec;
        std::filesystem::current_path(filePath.parent_path(), ec);
        if (ec)
        {
            fprintf(stderr, "cannot change into directory '%s'\n", filePath.parent_path().c_str());
            return 1;
        }
    }
    std::string baseName = filePath.stem().string();

    if (!SDL_Init(SDL_INIT_VIDEO))
    {
        fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
        return 1;
    }
    gpWindow = SDL_CreateWindow("AptViewer", gnWindowWidth, gnWindowHeight, SDL_WINDOW_RESIZABLE);
    if (!gpWindow)
    {
        fprintf(stderr, "SDL_CreateWindow failed: %s\n", SDL_GetError());
        return 1;
    }
    if (!gRenderer.Init(gpWindow))
    {
        return 1;
    }

    loadFonts();

    AptLibraryInitParams libParams;
    AptAuxSDLGPU_Initialize(libParams.Funcs, &gRenderer, &gFonts);
    if (gFonts.nFonts == 0)
    {
        printf("WARNING: no fonts (*.ttf) found in animation directory: dynamic text rendering disabled.\n");
    }

    // needed for buffered rendering, like the reference viewers
    libParams.nOptFlags = APT_OPT_SPRTREE;

    AptLibraryHandle hLib = AptLibraryInitialize(&libParams);
    if (hLib == NULL)
    {
        printf("ERROR: AptLibraryInitialize failed.\n");
        return 1;
    }

    AptInitParams initParams;
    initParams.iZombieVectorSize = 64;
    initParams.iCallStackDepth = 64;
    initParams.iLookupArraySize = 0x400;
    AptInitialize(hLib, &initParams);

    AptLoadAnimation(baseName.c_str(), "_level0");
    AptSetValidFocusButton();

    Uint64 nLastTicks = SDL_GetTicks();
    bool bRunning = true;
    while (bRunning)
    {
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            switch (event.type)
            {
                case SDL_EVENT_QUIT:
                    bRunning = false;
                    break;
                case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
                    gnWindowWidth = event.window.data1;
                    gnWindowHeight = event.window.data2;
                    break;
                case SDL_EVENT_MOUSE_MOTION:
                    AptSetMousePosition((int)event.motion.x - gnStageX, (int)event.motion.y - gnStageY);
                    break;
                case SDL_EVENT_MOUSE_BUTTON_DOWN:
                case SDL_EVENT_MOUSE_BUTTON_UP:
                    if (event.button.button == SDL_BUTTON_LEFT)
                    {
                        AptAddToInputQueue(AptInputType_MouseButton0,
                                           event.type == SDL_EVENT_MOUSE_BUTTON_DOWN ? AptInputState_Pressed
                                                                                     : AptInputState_Released,
                                           AptInputController_Mouse);
                    }
                    break;
                case SDL_EVENT_KEY_DOWN:
                    if (event.key.key == SDLK_ESCAPE)
                    {
                        bRunning = false;
                        break;
                    }
                    handleKey(event.key, true);
                    break;
                case SDL_EVENT_KEY_UP:
                    handleKey(event.key, false);
                    break;
                default:
                    break;
            }
        }

        if (gbGremlins)
        {
            doGremlins();
        }

        static Uint64 nFrameCounter = 0;
        nFrameCounter++;

        for (int i = 0; i < gnScriptCalls; i++)
        {
            if (gaScriptCalls[i].szName && (long)nFrameCounter == gaScriptCalls[i].nFrame)
            {
                char szTarget[512];
                snprintf(szTarget, sizeof(szTarget), "_level0/%s", gaScriptCalls[i].szName);
                printf("calling script function '%s'\n", szTarget);
                AptCallFunctionOpti(szTarget);
            }
        }

        // APTVIEWER_AUTOCLICK="x,y,frame": moves the mouse to the stage position and
        // clicks at the given frame - synthetic input for unattended testing
        static const char *szAutoClick = SDL_getenv("APTVIEWER_AUTOCLICK");
        if (szAutoClick)
        {
            int nX, nY;
            unsigned long nFrame;
            if (sscanf(szAutoClick, "%d,%d,%lu", &nX, &nY, &nFrame) == 3)
            {
                if (nFrameCounter >= nFrame - 30)
                {
                    AptSetMousePosition(nX, nY);
                }
                if (nFrameCounter == nFrame)
                {
                    printf("autoclick: press at %d,%d\n", nX, nY);
                    AptAddToInputQueue(AptInputType_MouseButton0, AptInputState_Pressed, AptInputController_Mouse);
                }
                if (nFrameCounter == nFrame + 5)
                {
                    printf("autoclick: release at %d,%d\n", nX, nY);
                    AptAddToInputQueue(AptInputType_MouseButton0, AptInputState_Released, AptInputController_Mouse);
                }
            }
        }

        AptAuxSDLGPU_FinishAsyncLoads();

        Uint64 nNow = SDL_GetTicks();
        unsigned int nDelta = (unsigned int)(nNow - nLastTicks);
        nLastTicks = nNow;
        if (nDelta > 250)
        {
            nDelta = 16;
        }
        AptUpdate(nDelta);

        resizeWindowToFitAnim();
        updateStage();

        unsigned int nBackground = AptAuxSDLGPU_GetBackgroundColour();
        float fR = ((nBackground >> 16) & 0xff) / 255.f;
        float fG = ((nBackground >> 8) & 0xff) / 255.f;
        float fB = (nBackground & 0xff) / 255.f;
        if (gRenderer.BeginFrame(fR, fG, fB, 1.f))
        {
            gRenderer.SetMaskingEnabled(false);
            AptRender(16);
            gRenderer.EndFrame();
        }
        else
        {
            SDL_Delay(16);
        }

        if (nMaxFrames > 0 && --nMaxFrames == 0)
        {
            bRunning = false;
        }
    }

    AptShutdown(hLib);
    AptAuxSDLGPU_Shutdown();
    freeFontTable();
    AptLibraryShutdown(hLib);

    gRenderer.Shutdown();
    SDL_DestroyWindow(gpWindow);
    SDL_Quit();
    return 0;
}
