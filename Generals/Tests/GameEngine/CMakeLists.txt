find_package(GTest REQUIRED)
find_package(SDL3 REQUIRED)

add_executable(g_gameenginetest
    ${CMAKE_SOURCE_DIR}/Common/Tests/GameEngine/Common/AsciiStringTest.cpp
    ${CMAKE_SOURCE_DIR}/Common/Tests/GameEngine/Common/CRCTest.cpp
    ${CMAKE_SOURCE_DIR}/Common/Tests/GameEngine/Common/DictTest.cpp
    ${CMAKE_SOURCE_DIR}/Common/Tests/GameEngine/Common/EncryptTest.cpp
    ${CMAKE_SOURCE_DIR}/Common/Tests/GameEngine/Common/INITest.cpp
    ${CMAKE_SOURCE_DIR}/Common/Tests/GameEngine/Common/NameKeyGeneratorTest.cpp
    ${CMAKE_SOURCE_DIR}/Common/Tests/GameEngine/Common/UnicodeStringTest.cpp
    ${CMAKE_SOURCE_DIR}/Common/Tests/MainStub.cpp
)

# g_gameengine and g_gameenginedevice reference each other's symbols (e.g. the
# engine's Team/Player code uses TheKey_* keys instantiated in the device's
# WorldHeightMap.cpp). As static archives on a single link line the linker
# scans each only once, so back-references from the earlier archive to the
# later one go unresolved. Wrap them in a RESCAN link group (ld --start-group)
# so the linker re-scans until the cycle is satisfied.
target_link_libraries(g_gameenginetest
    "$<LINK_GROUP:RESCAN,g_gameenginedevice,g_gameengine>"
    GTest::gtest GTest::gtest_main SDL3::SDL3 sage_test_support)
target_compile_definitions(g_gameenginetest PRIVATE 
    SAGE_MOD_SUPPORT_DIR="${SAGE_MOD_SUPPORT_DIR}"
    SAGE_TEST_DATA_DIR="${SAGE_TEST_DATA_DIR}"
)

set_target_properties(g_gameenginetest PROPERTIES OUTPUT_NAME "GameEngineTest")

include(GoogleTest)
gtest_discover_tests(g_gameenginetest)