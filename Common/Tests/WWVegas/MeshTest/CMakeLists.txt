find_package(GTest REQUIRED)
find_package(SDL3 REQUIRED)

add_executable(c_meshtest meshtest.cpp)
target_link_libraries(c_meshtest c_ww3d2 c_wwstub GTest::gtest GTest::gtest_main SDL3::SDL3)
set_target_properties(c_meshtest PROPERTIES OUTPUT_NAME "MeshTest")
target_compile_definitions(c_meshtest PRIVATE MESH_DATA_PATH="${CMAKE_CURRENT_SOURCE_DIR}/data/")

include(GoogleTest)
gtest_discover_tests(c_meshtest)