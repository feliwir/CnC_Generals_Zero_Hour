add_custom_command(OUTPUT buildVersion.h COMMAND c_buildversionupdate ARGS
                                                 buildVersion.h)

add_custom_command(OUTPUT generatedVersion.h COMMAND c_versionupdate ARGS
                                                     generatedVersion.h)

add_executable(z_rts_sdl)
target_sources(
  z_rts_sdl
  PRIVATE ${CMAKE_SOURCE_DIR}/Common/Main/SDL3Main.cpp WinMain.h ${CMAKE_CURRENT_BINARY_DIR}/buildVersion.h
          ${CMAKE_CURRENT_BINARY_DIR}/generatedVersion.h)

set_target_properties(z_rts_sdl PROPERTIES OUTPUT_NAME "RTS")
target_link_libraries(z_rts_sdl PRIVATE z_gameengine z_gameenginedevice)
target_include_directories(z_rts_sdl PRIVATE ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR})

# Install/packaging rules for this target live in cmake/Packaging.cmake.
