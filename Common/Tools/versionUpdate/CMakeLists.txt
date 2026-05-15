add_executable(c_versionupdate)

target_sources(c_versionupdate PRIVATE versionUpdate.cpp)
set_target_properties(c_versionupdate PROPERTIES OUTPUT_NAME "versionUpdate")