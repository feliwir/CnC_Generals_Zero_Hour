include(FetchContent)
FetchContent_Declare(
  cnc_mod_support
  GIT_REPOSITORY https://github.com/electronicarts/CnC_Modding_Support.git
  GIT_TAG main
)
FetchContent_MakeAvailable(cnc_mod_support)

message(STATUS "CNC Mod Support directory: ${cnc_mod_support_SOURCE_DIR}")

# Resurcive glob all *.map files inside the mod support directory
file(GLOB_RECURSE CNC_GEN_MAPS "${cnc_mod_support_SOURCE_DIR}/Generals/Maps/*.map")
foreach (map_path IN LISTS CNC_GEN_MAPS)
  # Get the mapname
  get_filename_component(map_name ${map_path} NAME_WE)
  # Run the compress tool and probe each map file for its compression
  add_test(
    NAME "Compress_${map_name}"
    COMMAND c_compress -in "${map_path}"
  )
endforeach()