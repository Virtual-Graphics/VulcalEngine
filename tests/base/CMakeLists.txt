add_executable(binary_io_test binary_io.cpp)
target_link_libraries(binary_io_test PRIVATE vulcal_engine GTest::gtest_main)

include(GoogleTest)
gtest_discover_tests(binary_io_test)
