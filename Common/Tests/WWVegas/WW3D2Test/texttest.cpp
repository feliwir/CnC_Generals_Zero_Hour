#include "ww3d.h"
#include "assetmgr.h"
#include "render2dsentence.h"

#include <cstdio>
#include <SDL3/SDL.h>
#include <gtest/gtest.h>
#include <unicode/ustring.h>

class W3DTextRenderTest :  public ::testing::TestWithParam<const char*>
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

TEST_P(W3DTextRenderTest, DrawText)
{
    FontCharsClass *fontChars = WW3DAssetManager::Get_Instance()->Get_FontChars("Arial Unicode MS", 12, false);
    const char *sentence = GetParam();
    UChar wSentence[256] = {0};
    UErrorCode err = U_ZERO_ERROR;
    u_strFromUTF8(wSentence, 256, nullptr, sentence, -1, &err);
    ASSERT_EQ(err, U_ZERO_ERROR) << "Failed to convert sentence to UTF-16";

    Render2DSentenceClass textRenderer;
    textRenderer.Set_Font(fontChars);

    // Measure a sentence and check that the extents are reasonable
    Vector2 textExtents = textRenderer.Get_Text_Extents(wSentence);
    EXPECT_GT(textExtents.X, 0.0f);
    EXPECT_GT(textExtents.Y, 0.0f);
    Vector2 formattedTextExtents = textRenderer.Get_Formatted_Text_Extents(wSentence);
    EXPECT_GT(formattedTextExtents.X, 0.0f);
    EXPECT_GT(formattedTextExtents.Y, 0.0f);
    EXPECT_EQ(textExtents, formattedTextExtents);

    // Draw a sentence and check that the draw extents are updated correctly
    textRenderer.Build_Sentence(wSentence, nullptr, nullptr);
    RectClass drawExtents = textRenderer.Get_Draw_Extents();
    EXPECT_EQ(drawExtents.Width(), 0.0f);
    EXPECT_EQ(drawExtents.Height(), 0.0f);
    textRenderer.Draw_Sentence(0xFFFFFFFF);
    drawExtents = textRenderer.Get_Draw_Extents();
    EXPECT_GT(drawExtents.Width(), 0.0f);
    EXPECT_GT(drawExtents.Height(), 0.0f);
	textRenderer.Render();

    WW3D::Make_Screen_Shot("texttest");
}

const char* sentences[] = {
    u8"Launch game",
    u8"Lorem Ipsum",
    u8"ÄÖÜ äöü ß",
    u8"こんにちは世界",
    u8"😀😃😄😁😆😅😂🤣☺️😊",
};

INSTANTIATE_TEST_SUITE_P(WW3D2, W3DTextRenderTest, ::testing::ValuesIn(sentences));