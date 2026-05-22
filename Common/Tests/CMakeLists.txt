include(FetchContent)
FetchContent_Declare(
  cnc_mod_support
  GIT_REPOSITORY https://github.com/electronicarts/CnC_Modding_Support.git
  GIT_TAG main
)
FetchContent_MakeAvailable(cnc_mod_support)

message(STATUS "CNC Mod Support directory: ${cnc_mod_support_SOURCE_DIR}")
add_subdirectory(Compression)
add_subdirectory(WWVegas)