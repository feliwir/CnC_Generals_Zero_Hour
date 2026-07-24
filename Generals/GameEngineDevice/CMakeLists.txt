# Target setup
add_library(g_gameenginedevice)

target_sources(
  g_gameenginedevice
  PRIVATE
    Source/W3DDevice/GameClient/W3DDebugDisplay.cpp
          Include/W3DDevice/Common/W3DRadar.h
          Include/W3DDevice/GameClient/Module/W3DLaserDraw.h
          Include/W3DDevice/GameClient/Module/W3DModelDraw.h
          Include/W3DDevice/GameClient/TerrainTex.h
          Include/W3DDevice/GameClient/W3DBridgeBuffer.h
          Include/W3DDevice/GameClient/W3DDisplayStringManager.h
          Include/W3DDevice/GameClient/W3DFileSystem.h
          Include/W3DDevice/GameClient/W3DGameClient.h
          Include/W3DDevice/GameClient/W3DProjectedShadow.h
          Include/W3DDevice/GameClient/W3DRoadBuffer.h
          Include/W3DDevice/GameClient/W3DShaderManager.h
          Include/W3DDevice/GameClient/W3DTerrainVisual.h
          Include/W3DDevice/GameClient/W3DView.h
          Include/W3DDevice/GameClient/W3DWaypointBuffer.h
          Source/W3DDevice/Common/System/W3DFunctionLexicon.cpp
          Source/W3DDevice/Common/System/W3DRadar.cpp
          Source/W3DDevice/Common/Thing/W3DModuleFactory.cpp
          Source/W3DDevice/Common/Thing/W3DThingFactory.cpp
          Source/W3DDevice/GameClient/Drawable/Draw/W3DDebrisDraw.cpp
          Source/W3DDevice/GameClient/Drawable/Draw/W3DDefaultDraw.cpp
          Source/W3DDevice/GameClient/Drawable/Draw/W3DDependencyModelDraw.cpp
          Source/W3DDevice/GameClient/Drawable/Draw/W3DLaserDraw.cpp
          Source/W3DDevice/GameClient/Drawable/Draw/W3DModelDraw.cpp
          Source/W3DDevice/GameClient/Drawable/Draw/W3DOverlordTankDraw.cpp
          Source/W3DDevice/GameClient/Drawable/Draw/W3DPoliceCarDraw.cpp
          Source/W3DDevice/GameClient/Drawable/Draw/W3DProjectileStreamDraw.cpp
          Source/W3DDevice/GameClient/Drawable/Draw/W3DRopeDraw.cpp
          Source/W3DDevice/GameClient/Drawable/Draw/W3DScienceModelDraw.cpp
          Source/W3DDevice/GameClient/Drawable/Draw/W3DSupplyDraw.cpp
          Source/W3DDevice/GameClient/Drawable/Draw/W3DTankDraw.cpp
          Source/W3DDevice/GameClient/Drawable/Draw/W3DTankTruckDraw.cpp
          Source/W3DDevice/GameClient/Drawable/Draw/W3DTracerDraw.cpp
          Source/W3DDevice/GameClient/Drawable/Draw/W3DTreeDraw.cpp
          Source/W3DDevice/GameClient/Drawable/Draw/W3DTruckDraw.cpp
          Source/W3DDevice/GameClient/GUI/Gadget/W3DCheckBox.cpp
          Source/W3DDevice/GameClient/GUI/Gadget/W3DComboBox.cpp
          Source/W3DDevice/GameClient/GUI/Gadget/W3DHorizontalSlider.cpp
          Source/W3DDevice/GameClient/GUI/Gadget/W3DListBox.cpp
          Source/W3DDevice/GameClient/GUI/Gadget/W3DProgressBar.cpp
          Source/W3DDevice/GameClient/GUI/Gadget/W3DPushButton.cpp
          Source/W3DDevice/GameClient/GUI/Gadget/W3DRadioButton.cpp
          Source/W3DDevice/GameClient/GUI/Gadget/W3DStaticText.cpp
          Source/W3DDevice/GameClient/GUI/Gadget/W3DTabControl.cpp
          Source/W3DDevice/GameClient/GUI/Gadget/W3DTextEntry.cpp
          Source/W3DDevice/GameClient/GUI/Gadget/W3DVerticalSlider.cpp
          Source/W3DDevice/GameClient/GUI/GUICallbacks/W3DControlBar.cpp
          Source/W3DDevice/GameClient/GUI/GUICallbacks/W3DMainMenu.cpp
          Source/W3DDevice/GameClient/GUI/GUICallbacks/W3DMOTD.cpp
          Source/W3DDevice/GameClient/GUI/W3DGameWindow.cpp
          Source/W3DDevice/GameClient/GUI/W3DGameWindowManager.cpp
          Source/W3DDevice/GameClient/Shadow/W3DProjectedShadow.cpp
          Source/W3DDevice/GameClient/Shadow/W3DShadow.cpp
          Source/W3DDevice/GameClient/Shadow/W3DVolumetricShadow.cpp
          Source/W3DDevice/GameClient/TerrainTex.cpp
          Source/W3DDevice/GameClient/W3DAssetManager.cpp
          Source/W3DDevice/GameClient/W3DBibBuffer.cpp
          Source/W3DDevice/GameClient/W3DBridgeBuffer.cpp
          Source/W3DDevice/GameClient/W3DCustomEdging.cpp
          Source/W3DDevice/GameClient/W3DDebugIcons.cpp
          Source/W3DDevice/GameClient/W3DDisplay.cpp
          Source/W3DDevice/GameClient/W3DDisplayString.cpp
          Source/W3DDevice/GameClient/W3DDisplayStringManager.cpp
          Source/W3DDevice/GameClient/W3DFileSystem.cpp
          Source/W3DDevice/GameClient/W3DGameClient.cpp
          Source/W3DDevice/GameClient/W3DInGameUI.cpp
          Source/W3DDevice/GameClient/W3DMouse.cpp
          Source/W3DDevice/GameClient/W3DParticleSys.cpp
          Source/W3DDevice/GameClient/W3DPropBuffer.cpp
          Source/W3DDevice/GameClient/W3DRoadBuffer.cpp
          Source/W3DDevice/GameClient/W3DScene.cpp
          Source/W3DDevice/GameClient/W3DShaderManager.cpp
          Source/W3DDevice/GameClient/W3DShroud.cpp
          Source/W3DDevice/GameClient/W3DSmudge.cpp
          Source/W3DDevice/GameClient/W3DSnow.cpp
          Source/W3DDevice/GameClient/W3DStatusCircle.cpp
          Source/W3DDevice/GameClient/W3DTerrainBackground.cpp
          Source/W3DDevice/GameClient/W3DTerrainTracks.cpp
          Source/W3DDevice/GameClient/W3DTerrainVisual.cpp
          Source/W3DDevice/GameClient/W3DTreeBuffer.cpp
          Source/W3DDevice/GameClient/W3DView.cpp
          Source/W3DDevice/GameClient/W3dWaypointBuffer.cpp
          Source/W3DDevice/GameClient/W3DWebBrowser.cpp
          Source/W3DDevice/GameClient/Water/W3DWater.cpp
          Source/W3DDevice/GameClient/Water/W3DWaterTracks.cpp
          Source/W3DDevice/GameLogic/W3DGhostObject.cpp
          Source/W3DDevice/GameLogic/W3DTerrainLogic.cpp)

if(SAGE_USE_MILES)
  target_sources(
    g_gameenginedevice PRIVATE Include/MilesAudioDevice/MilesAudioManager.h
                               Source/MilesAudioDevice/MilesAudioManager.cpp)
  target_compile_definitions(g_gameenginedevice PUBLIC SAGE_USE_MILES)
endif()

if(SAGE_USE_BINK)
  target_compile_definitions(g_gameenginedevice PUBLIC SAGE_USE_BINK)
endif()

if(SAGE_USE_SDL3)
  target_sources(
    g_gameenginedevice
    PRIVATE Source/SDL3Device/Common/SDL3GameEngine.cpp
            Source/SDL3Device/Common/SDL3IMEManager.cpp
            Source/SDL3Device/GameClient/SDL3Keyboard.cpp
            Source/SDL3Device/GameClient/SDL3Mouse.cpp)

  find_package(SDL3 CONFIG REQUIRED)
  find_package(SDL3_image CONFIG REQUIRED)
  target_link_libraries(g_gameenginedevice PUBLIC SDL3::SDL3
                                                  SDL3_image::SDL3_image)
  target_compile_definitions(g_gameenginedevice PUBLIC SAGE_USE_SDL3)
endif()

find_package(glm CONFIG REQUIRED)
target_link_libraries(g_gameenginedevice PRIVATE glm::glm)

if(SAGE_USE_FFMPEG)
  find_package(FFMPEG REQUIRED)
  target_include_directories(g_gameenginedevice PRIVATE ${FFMPEG_INCLUDE_DIRS})
  target_link_libraries(g_gameenginedevice PRIVATE ${FFMPEG_LIBRARIES})
  target_compile_definitions(g_gameenginedevice PUBLIC SAGE_USE_FFMPEG)
endif()

if(SAGE_USE_MINIAUDIO)
  target_sources(
    g_gameenginedevice PRIVATE Source/MiniAudioDevice/MiniAudioManager.cpp)

  target_compile_definitions(g_gameenginedevice PUBLIC SAGE_USE_MINIAUDIO)
endif()

if(WIN32)
  target_sources(
    g_gameenginedevice
    PRIVATE Source/Win32Device/Common/Win32BIGFile.cpp
            Source/Win32Device/Common/Win32CDManager.cpp
            Source/Win32Device/Common/Win32GameEngine.cpp
            Source/Win32Device/Common/Win32OSDisplay.cpp
            Source/Win32Device/GameClient/Win32DIKeyboard.cpp
            Source/Win32Device/GameClient/Win32DIMouse.cpp
            Source/Win32Device/GameClient/Win32Mouse.cpp
            Source/Win32Device/Common/Win32BIGFileSystem.cpp
            Source/Win32Device/Common/Win32LocalFileSystem.cpp)
endif()

set_target_properties(g_gameenginedevice PROPERTIES OUTPUT_NAME
                                                    "GameEngineDevice")

target_include_directories(g_gameenginedevice
                           PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/Include)

target_link_libraries(g_gameenginedevice PUBLIC c_gameenginedevice g_gameengine)

# Compile the shared Common engine sources into this per-game library.
target_sources(g_gameenginedevice PRIVATE $<TARGET_PROPERTY:c_gameenginedevice,SAGE_GAME_SOURCES>)
