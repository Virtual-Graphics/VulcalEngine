add_executable(build_pipeline_test build_pipeline.cpp ../../apps/vulcal_build/build_pipeline.cpp)
target_include_directories(build_pipeline_test PRIVATE ../../apps)
target_link_libraries(build_pipeline_test PRIVATE vulcal_engine GTest::gtest_main vulcal::miniz)

include(GoogleTest)
gtest_discover_tests(build_pipeline_test)
