find_package(GTest REQUIRED)

add_executable(c_mathtest aaboxtest.cpp mempooltest.cpp obboxtest.cpp raytest.cpp)
target_link_libraries(c_mathtest c_wwmath c_wwstub GTest::gtest GTest::gtest_main)
set_target_properties(c_mathtest PROPERTIES OUTPUT_NAME "MathTest")

include(GoogleTest)
gtest_discover_tests(c_mathtest)