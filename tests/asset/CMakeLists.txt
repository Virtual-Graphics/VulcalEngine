add_executable(asset_manager_test asset_manager.cpp)
target_link_libraries(asset_manager_test PRIVATE vulcal_engine GTest::gtest_main)

add_executable(mesh_asset_test mesh_asset.cpp)
target_link_libraries(mesh_asset_test PRIVATE vulcal_engine GTest::gtest_main)

add_executable(material_asset_test material_asset.cpp)
target_link_libraries(material_asset_test PRIVATE vulcal_engine GTest::gtest_main)

add_executable(texture_asset_test texture_asset.cpp)
target_link_libraries(texture_asset_test PRIVATE vulcal_engine GTest::gtest_main)

include(GoogleTest)
gtest_discover_tests(asset_manager_test)
gtest_discover_tests(mesh_asset_test)
gtest_discover_tests(material_asset_test)
gtest_discover_tests(texture_asset_test)

option(BUILD_GRAPHICS_TESTS "Build tests requiring an OpenGL 4.6 context" OFF)
if (BUILD_GRAPHICS_TESTS)
    add_executable(render_asset_cache_test render_asset_cache.cpp)
    target_link_libraries(render_asset_cache_test PRIVATE vulcal_engine GTest::gtest_main)
    gtest_discover_tests(render_asset_cache_test PROPERTIES LABELS graphics)
endif()
