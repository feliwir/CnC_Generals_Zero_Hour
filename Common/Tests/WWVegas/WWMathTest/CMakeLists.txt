find_package(GTest REQUIRED)

add_executable(c_wwmathtest aaboxtest.cpp mempooltest.cpp obboxtest.cpp raytest.cpp)
target_link_libraries(c_wwmathtest c_wwmath c_wwstub GTest::gtest GTest::gtest_main)
set_target_properties(c_wwmathtest PROPERTIES OUTPUT_NAME "WWMathTest")

include(GoogleTest)
gtest_discover_tests(c_wwmathtest)