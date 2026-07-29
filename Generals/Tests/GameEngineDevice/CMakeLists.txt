find_package(GTest REQUIRED)
find_package(SDL3 REQUIRED)

add_executable(g_gameenginedevicetest
    ${CMAKE_SOURCE_DIR}/Common/Tests/GameEngineDevice/SDL3Device/SDL3LocalFileSystemTest.cpp
    SDL3Device/SDL3BigFileSystemTest.cpp
    ${CMAKE_SOURCE_DIR}/Common/Tests/MainStub.cpp
)

# See Generals/Tests/GameEngine/CMakeLists.txt: the engine/device static
# archives reference each other, so wrap them in a RESCAN link group.
target_link_libraries(g_gameenginedevicetest
    "$<LINK_GROUP:RESCAN,g_gameenginedevice,g_gameengine>"
    GTest::gtest GTest::gtest_main SDL3::SDL3 sage_test_support)
target_compile_definitions(g_gameenginedevicetest PRIVATE 
    SAGE_MOD_SUPPORT_DIR="${SAGE_MOD_SUPPORT_DIR}"
    SAGE_TEST_DATA_DIR="${SAGE_TEST_DATA_DIR}"
)

set_target_properties(g_gameenginedevicetest PROPERTIES OUTPUT_NAME "GameEngineDeviceTest")

include(GoogleTest)
gtest_discover_tests(g_gameenginedevicetest)