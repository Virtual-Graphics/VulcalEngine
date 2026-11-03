add_executable(asset_manager_test asset_manager.cpp)
target_link_libraries(asset_manager_test PRIVATE vulcal_engine GTest::gtest_main)

include(GoogleTest)
gtest_discover_tests(asset_manager_test)

option(BUILD_GRAPHICS_TESTS "Build tests requiring an OpenGL 4.6 context" OFF)
if (BUILD_GRAPHICS_TESTS)
    add_executable(render_asset_cache_test render_asset_cache.cpp)
    target_link_libraries(render_asset_cache_test PRIVATE vulcal_engine GTest::gtest_main)
    gtest_discover_tests(render_asset_cache_test PROPERTIES LABELS graphics)
endif()
