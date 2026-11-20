add_executable(resource_provider_test resource_provider.cpp)
target_link_libraries(resource_provider_test PRIVATE vulcal_engine vulcal::miniz GTest::gtest_main)

include(GoogleTest)
gtest_discover_tests(resource_provider_test)
