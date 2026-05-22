# Resurcive glob all *.map files inside the mod support directory and decompress them with the -memdecompress flag to test if the decompression works correctly for all maps. This is important because some maps are compressed with a different compression method than the one we use for compression, so we need to make sure that we can still decompress them correctly.
file(GLOB_RECURSE CNC_GEN_MAPS "${cnc_mod_support_SOURCE_DIR}/Generals/Maps/*.map")
foreach (map_path IN LISTS CNC_GEN_MAPS)
  # Get the mapname
  get_filename_component(map_name ${map_path} NAME_WE)
  # Run the compress tool and probe each map file for its compression
  add_test(
    NAME "Decompress_${map_name}"
    COMMAND c_compress -memdecompress -in "${map_path}"
  )
endforeach()