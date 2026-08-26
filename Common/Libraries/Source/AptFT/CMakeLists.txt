add_library(c_aptft STATIC)

target_sources(c_aptft PRIVATE
    ../../Include/AptFT/ftapt.h
    ftapt.cpp
)

target_include_directories(c_aptft PUBLIC
    ../../Include/AptFT
)

# ftapt.h exposes FT_Library in its interface, so FreeType propagates publicly.
find_package(Freetype CONFIG REQUIRED)
target_link_libraries(c_aptft PUBLIC c_apt Freetype::Freetype)

target_compile_definitions(c_aptft PUBLIC APT_USE_FREETYPE)
