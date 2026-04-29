add_library(g_library_include INTERFACE)
target_include_directories(g_library_include
                           INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/Include)

add_subdirectory(Source/Compression)
