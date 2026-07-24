# Target setup
add_library(c_gameenginedevice INTERFACE)

target_sources(
  c_gameenginedevice
  INTERFACE Include/W3DDevice/Common/W3DConvert.h
            Source/W3DDevice/GameLogic/W3DGameLogic.cpp
            Source/W3DDevice/GameClient/W3DPoly.cpp
            Source/W3DDevice/GameClient/W3DGranny.cpp
            Source/W3DDevice/GameClient/W3DAssetManagerExposed.cpp
            Source/W3DDevice/GameClient/Shadow/W3DBufferManager.cpp
            Source/W3DDevice/GameClient/GUI/W3DGameFont.cpp
            Include/W3DDevice/GameClient/W3DWater.h
            Include/W3DDevice/GameClient/W3DTreeBuffer.h
            Include/W3DDevice/GameClient/W3DScene.h
            Include/W3DDevice/GameClient/W3DPropBuffer.h
            Include/W3DDevice/GameLogic/W3DTerrainLogic.h
            Include/W3DDevice/GameLogic/W3DGhostObject.h
            Include/W3DDevice/GameLogic/W3DGameLogic.h
            Include/W3DDevice/GameClient/W3DWaterTracks.h
            Include/W3DDevice/GameClient/W3DVolumetricShadow.h
            Include/W3DDevice/GameClient/W3DTerrainTracks.h
            Include/W3DDevice/GameClient/W3DStatusCircle.h
            Include/W3DDevice/GameClient/W3DShroud.h
            Include/W3DDevice/GameClient/W3DShadow.h
            Include/W3DDevice/GameClient/W3DPoly.h
            Include/W3DDevice/GameClient/W3DParticleSys.h
            Include/W3DDevice/GameClient/W3DMouse.h
            Include/W3DDevice/GameClient/W3DMirror.h
            Include/W3DDevice/GameClient/W3DInGameUI.h
            Include/W3DDevice/GameClient/W3DGranny.h
            Include/W3DDevice/GameClient/W3DGameWindowManager.h
            Include/W3DDevice/GameClient/W3DGameWindow.h
            Include/W3DDevice/GameClient/W3DGameFont.h
            Include/W3DDevice/GameClient/W3DGadget.h
            Include/W3DDevice/GameClient/W3DGUICallbacks.h
            Include/W3DDevice/GameClient/W3DDisplayString.h
            Include/W3DDevice/GameClient/W3DDisplay.h
            Include/W3DDevice/GameClient/W3DDebugIcons.h
            Include/W3DDevice/GameClient/W3DDebugDisplay.h
            Include/W3DDevice/GameClient/W3DCustomScene.h
            Include/W3DDevice/GameClient/W3DCustomEdging.h
            Include/W3DDevice/GameClient/W3DBufferManager.h
            Include/W3DDevice/GameClient/W3DBibBuffer.h
            Include/W3DDevice/GameClient/W3DAssetManagerExposed.h
            Include/W3DDevice/GameClient/TileData.h
            Include/W3DDevice/GameClient/WorldHeightMap.h
            Include/W3DDevice/GameClient/HeightMap.h
            Include/W3DDevice/GameClient/BaseHeightMap.h
            Include/W3DDevice/GameClient/FlatHeightMap.h
            Include/W3DDevice/GameClient/Module/W3DTruckDraw.h
            Include/W3DDevice/GameClient/Module/W3DTracerDraw.h
            Include/W3DDevice/GameClient/Module/W3DTankTruckDraw.h
            Include/W3DDevice/GameClient/Module/W3DTankDraw.h
            Include/W3DDevice/GameClient/Module/W3DSupplyDraw.h
            Include/W3DDevice/GameClient/Module/W3DScienceModelDraw.h
            Include/W3DDevice/GameClient/Module/W3DRopeDraw.h
            Include/W3DDevice/GameClient/Module/W3DProjectileStreamDraw.h
            Include/W3DDevice/GameClient/Module/W3DPoliceCarDraw.h
            Include/W3DDevice/GameClient/Module/W3DOverlordTankDraw.h
            Include/W3DDevice/GameClient/Module/W3DDependencyModelDraw.h
            Include/W3DDevice/GameClient/Module/W3DDefaultDraw.h
            Include/W3DDevice/GameClient/Module/W3DDebrisDraw.h
            Include/W3DDevice/Common/W3DThingFactory.h
            Include/W3DDevice/Common/W3DModuleFactory.h
            Include/W3DDevice/Common/W3DFunctionLexicon.h
          Include/W3DDevice/GameClient/Module/W3DPropDraw.h
          Include/W3DDevice/GameClient/Module/W3DTreeDraw.h
          Include/W3DDevice/GameClient/W3DAssetManager.h
          Include/W3DDevice/GameClient/W3DDynamicLight.h
          Include/W3DDevice/GameClient/W3DSmudge.h
          Include/W3DDevice/GameClient/W3DSnow.h
          Include/W3DDevice/GameClient/W3DTerrainBackground.h
          Include/W3DDevice/GameClient/W3DVideobuffer.h
          Include/W3DDevice/GameClient/W3DWebBrowser.h
          Source/W3DDevice/Common/W3DConvert.cpp
          Source/W3DDevice/GameClient/W3DDynamicLight.cpp
          Source/W3DDevice/GameClient/W3DVideoBuffer.cpp
          Source/W3DDevice/GameClient/TileData.cpp
          Source/W3DDevice/GameClient/WorldHeightMap.cpp
          Source/W3DDevice/GameClient/HeightMap.cpp
          Source/W3DDevice/GameClient/BaseHeightMap.cpp
          Source/W3DDevice/GameClient/FlatHeightMap.cpp)

if(SAGE_USE_SDL3)
  target_sources(
    c_gameenginedevice
    INTERFACE Include/SDL3Device/Common/SDL3BIGFile.h
            Source/SDL3Device/Common/SDL3OSDisplay.cpp
            Source/SDL3Device/Common/SDL3CDManager.cpp
            Include/SDL3Device/Common/SDL3BIGFileSystem.h
            Include/SDL3Device/Common/SDL3CDManager.h
            Include/SDL3Device/Common/SDL3GameEngine.h
            Include/SDL3Device/Common/SDL3LocalFile.h
            Include/SDL3Device/Common/SDL3LocalFileSystem.h
            Include/SDL3Device/GameClient/SDL3Keyboard.h
            Include/SDL3Device/GameClient/SDL3Mouse.h
            Source/SDL3Device/Common/SDL3BIGFile.cpp
            Source/SDL3Device/Common/SDL3BIGFileSystem.cpp
            Source/SDL3Device/Common/SDL3LocalFile.cpp
            Source/SDL3Device/Common/SDL3LocalFileSystem.cpp)

  find_package(SDL3 CONFIG REQUIRED)
  find_package(SDL3_image CONFIG REQUIRED)
  target_link_libraries(c_gameenginedevice INTERFACE SDL3::SDL3
                                                  SDL3_image::SDL3_image)
  target_compile_definitions(c_gameenginedevice INTERFACE SAGE_USE_SDL3)
endif()

if(SAGE_USE_FFMPEG)
  target_sources(
    c_gameenginedevice
    INTERFACE Include/VideoDevice/FFmpeg/FFmpegFile.h
            Source/VideoDevice/FFmpeg/FFmpegVideoPlayer.cpp
            Include/VideoDevice/FFmpeg/FFmpegVideoPlayer.h
            Source/VideoDevice/FFmpeg/FFmpegFile.cpp)

  find_package(FFMPEG REQUIRED)
  target_include_directories(c_gameenginedevice INTERFACE ${FFMPEG_INCLUDE_DIRS})
  target_link_libraries(c_gameenginedevice INTERFACE ${FFMPEG_LIBRARIES})
  target_compile_definitions(c_gameenginedevice INTERFACE SAGE_USE_FFMPEG)
endif()

if(SAGE_USE_MINIAUDIO)
  target_sources(
    c_gameenginedevice
    INTERFACE Include/MiniAudioDevice/MiniAudioManager.h)
endif()

if(WIN32)
  target_sources(
    c_gameenginedevice
    INTERFACE Include/Win32Device/Common/Win32BIGFile.h
            Include/Win32Device/GameClient/Win32Mouse.h
            Include/Win32Device/GameClient/Win32DIMouse.h
            Include/Win32Device/GameClient/Win32DIKeyboard.h
            Include/Win32Device/Common/Win32LocalFileSystem.h
            Include/Win32Device/Common/Win32LocalFile.h
            Include/Win32Device/Common/Win32GameEngine.h
            Include/Win32Device/Common/Win32CDManager.h
            Include/Win32Device/Common/Win32BIGFileSystem.h
            Source/Win32Device/Common/Win32BIGFile.cpp
            Source/Win32Device/Common/Win32BIGFileSystem.cpp
            Source/Win32Device/Common/Win32LocalFile.cpp
            Source/Win32Device/Common/Win32LocalFileSystem.cpp)
endif()

if(SAGE_USE_BINK)
  target_sources(
    c_gameenginedevice
    INTERFACE Include/VideoDevice/Bink/BinkVideoPlayer.h
            Source/VideoDevice/Bink/BinkVideoPlayer.cpp)
endif()

set_target_properties(c_gameenginedevice PROPERTIES OUTPUT_NAME
                                                    "GameEngineDevice")

target_include_directories(c_gameenginedevice
                           INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/Include)

target_link_libraries(c_gameenginedevice INTERFACE c_gameengine)

# An INTERFACE source list propagates through the link graph, so every
# transitive consumer (device libs, executables, tests) would recompile all
# of these files. Park the list in a custom property instead; only the
# per-game engine libraries and the Common test stub compile it.
get_target_property(_sage_engine_sources c_gameenginedevice INTERFACE_SOURCES)
set_property(TARGET c_gameenginedevice PROPERTY SAGE_GAME_SOURCES ${_sage_engine_sources})
set_property(TARGET c_gameenginedevice PROPERTY INTERFACE_SOURCES)
