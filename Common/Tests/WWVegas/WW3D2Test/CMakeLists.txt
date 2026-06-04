find_package(GTest REQUIRED)
find_package(SDL3 REQUIRED)

add_executable(c_ww3d2test meshtest.cpp texttest.cpp)
target_link_libraries(c_ww3d2test c_ww3d2 c_wwstub GTest::gtest GTest::gtest_main SDL3::SDL3)
set_target_properties(c_ww3d2test PROPERTIES OUTPUT_NAME "WW3D2Test")
target_compile_definitions(c_ww3d2test PRIVATE MESH_DATA_PATH="${CMAKE_CURRENT_SOURCE_DIR}/data/")

include(GoogleTest)
gtest_discover_tests(c_ww3d2test)