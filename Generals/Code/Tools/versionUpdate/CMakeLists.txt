add_executable(g_versionupdate)

target_sources(g_versionupdate PRIVATE versionUpdate.cpp)
set_target_properties(g_versionupdate PROPERTIES OUTPUT_NAME "versionUpdate")