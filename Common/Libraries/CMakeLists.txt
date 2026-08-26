add_library(c_library_include INTERFACE)
target_include_directories(c_library_include
                           INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/Include)

find_package(ICU REQUIRED data i18n io uc)                
target_link_libraries(c_library_include INTERFACE c_compat ICU::data ICU::i18n ICU::io ICU::uc)

add_subdirectory(Source/Compression)
add_subdirectory(Source/WWVegas)
add_subdirectory(Source/Apt)
if(SAGE_USE_FREETYPE)
    add_subdirectory(Source/AptFT)
endif()