#include "ww3d.h"
#include "assetmgr.h"
#include "rawfile.h"

#include <cstdio>
#include <SDL3/SDL.h>
#include <gtest/gtest.h>

WW3DAssetManager The3DAssetManager;

class W3DLoadTest :  public ::testing::TestWithParam<const char*>
{
    protected:
    void SetUp() override
    {
    #ifndef _WIN32
        setenv("DXVK_WSI_DRIVER", "SDL3", 1);
    #endif
        SDL_Window *win = SDL_CreateWindow("MeshTest", 800, 600, SDL_WINDOW_VULKAN);
        ASSERT_NE(win, nullptr);
        WW3D::Init(win);
        WW3D::Set_Any_Render_Device();
    }

    void TearDown() override
    {
        WW3D::Shutdown();
    }
};

TEST_P(W3DLoadTest, LoadFromMemory)
{
    const char* filepath = GetParam();
    RawFileClass* file = new RawFileClass(filepath);
    EXPECT_TRUE(WW3DAssetManager::Get_Instance()->Load_3D_Assets(*file));
}

const char* w3d_files[] = {
    MESH_DATA_PATH "bodybox.w3d",
    MESH_DATA_PATH "carmarker.w3d",
    MESH_DATA_PATH "coverspot.w3d",
    MESH_DATA_PATH "dummy.w3d",
    MESH_DATA_PATH "grid.w3d",
    MESH_DATA_PATH "light.w3d",
};

INSTANTIATE_TEST_SUITE_P(LightW3D, W3DLoadTest, ::testing::ValuesIn(w3d_files));