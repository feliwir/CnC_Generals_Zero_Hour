# add_subdirectory(DXLib)
include(FetchContent)
FetchContent_Declare(
    DXVK
    GIT_REPOSITORY https://github.com/feliwir/DXVK-CMake.git
    GIT_TAG origin/main
)

FetchContent_MakeAvailable(DXVK)

add_subdirectory(Compat)
