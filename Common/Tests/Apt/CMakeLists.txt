# GoogleTest covering the Apt library: round-trip every Apt type.
find_package(GTest REQUIRED)

add_executable(c_apttest AptTest.cpp)
target_link_libraries(c_apttest PRIVATE c_apt c_library_include GTest::gtest GTest::gtest_main)
set_target_properties(c_apttest PROPERTIES OUTPUT_NAME "AptTest")
target_compile_definitions(c_apttest PRIVATE 
    SAGE_MOD_SUPPORT_DIR="${SAGE_MOD_SUPPORT_DIR}"
    SAGE_TEST_DATA_DIR="${SAGE_TEST_DATA_DIR}"
)

include(GoogleTest)
gtest_discover_tests(c_apttest)
