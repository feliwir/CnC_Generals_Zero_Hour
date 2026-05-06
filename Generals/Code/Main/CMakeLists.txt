add_custom_command(OUTPUT buildVersion.h COMMAND g_buildversionupdate ARGS
                                                 buildVersion.h)

add_custom_command(OUTPUT generatedVersion.h COMMAND g_versionupdate ARGS
                                                     generatedVersion.h)

add_executable(g_rts_sdl)
target_sources(
  g_rts_sdl
  PRIVATE SDL3Main.cpp WinMain.h ${CMAKE_CURRENT_BINARY_DIR}/buildVersion.h
          ${CMAKE_CURRENT_BINARY_DIR}/generatedVersion.h)

set_target_properties(g_rts_sdl PROPERTIES OUTPUT_NAME "RTS")
target_link_libraries(g_rts_sdl PRIVATE g_gameengine g_gameenginedevice)
target_include_directories(g_rts_sdl PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
