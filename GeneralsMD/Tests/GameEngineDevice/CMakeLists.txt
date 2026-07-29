find_package(GTest REQUIRED)
find_package(SDL3 REQUIRED)

add_executable(z_gameenginedevicetest
    ${CMAKE_SOURCE_DIR}/Common/Tests/GameEngineDevice/SDL3Device/SDL3LocalFileSystemTest.cpp
    SDL3Device/SDL3BigFileSystemTest.cpp
    VideoDevice/FFmpeg/FFmpegVideoPlayerTest.cpp
    ${CMAKE_SOURCE_DIR}/Common/Tests/MainStub.cpp
)

# See GeneralsMD/Tests/GameEngine/CMakeLists.txt: the engine/device static
# archives reference each other, so wrap them in a RESCAN link group.
target_link_libraries(z_gameenginedevicetest
    "$<LINK_GROUP:RESCAN,z_gameenginedevice,z_gameengine>"
    GTest::gtest GTest::gtest_main SDL3::SDL3 sage_test_support)
target_compile_definitions(z_gameenginedevicetest PRIVATE 
    SAGE_MOD_SUPPORT_DIR="${SAGE_MOD_SUPPORT_DIR}"
    SAGE_TEST_DATA_DIR="${SAGE_TEST_DATA_DIR}"
)

set_target_properties(z_gameenginedevicetest PROPERTIES OUTPUT_NAME "GameEngineDeviceTest")

include(GoogleTest)
gtest_discover_tests(z_gameenginedevicetest)