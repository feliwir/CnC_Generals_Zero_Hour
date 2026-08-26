# AptViewer needs the FreeType text bridge (c_aptft)
if(NOT SAGE_USE_FREETYPE)
    return()
endif()

add_executable(c_aptviewer)

target_sources(c_aptviewer PRIVATE
    AptViewer.cpp
    AptAuxSDLGPU.cpp
    AptAuxSDLGPU.h
    SDLGPURenderer.cpp
    SDLGPURenderer.h
)

find_package(SDL3 REQUIRED)
# the viewer compiles its GLSL shaders to SPIR-V at runtime
find_package(glslang CONFIG REQUIRED)

target_link_libraries(c_aptviewer PRIVATE
    c_apt
    c_aptft
    SDL3::SDL3
    glslang::glslang
    glslang::glslang-default-resource-limits
    glslang::SPIRV
)

set_target_properties(c_aptviewer PROPERTIES OUTPUT_NAME "AptViewer")
