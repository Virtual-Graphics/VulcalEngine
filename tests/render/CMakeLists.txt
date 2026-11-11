add_executable(embedded_shaders_test embedded_shaders.cpp)
target_include_directories(embedded_shaders_test PRIVATE "${PROJECT_BINARY_DIR}/vulcal_engine/generated")
target_link_libraries(embedded_shaders_test PRIVATE GTest::gtest_main)
file(TO_CMAKE_PATH "${PROJECT_SOURCE_DIR}/vulcal_engine/render/shaders" VULCAL_TEST_SHADER_DIR)
target_compile_definitions(embedded_shaders_test PRIVATE VULCAL_TEST_SHADER_DIR="${VULCAL_TEST_SHADER_DIR}")

include(GoogleTest)
gtest_discover_tests(embedded_shaders_test)
