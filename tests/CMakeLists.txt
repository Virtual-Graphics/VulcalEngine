set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/tests")

add_subdirectory(base)
add_subdirectory(resource)
if (BUILD_APPS)
    add_subdirectory(build_pipeline)
endif()
add_subdirectory(asset)
add_subdirectory(ecs)
add_subdirectory(editor)
add_subdirectory(input)
add_subdirectory(render)
include(GoogleTest)

add_executable(common_process_test
    common/application_paths.cpp
    ../apps/common/application_paths.cpp
)
target_include_directories(common_process_test PRIVATE ../apps)
target_link_libraries(common_process_test PRIVATE vulcal_engine GTest::gtest_main)
gtest_discover_tests(common_process_test)

add_executable(player_runtime_test
    player/player_runtime.cpp
    ../apps/vulcal_player/player_runtime.cpp
)
target_include_directories(player_runtime_test PRIVATE ../apps)
target_link_libraries(player_runtime_test PRIVATE vulcal_engine GTest::gtest_main)
gtest_discover_tests(player_runtime_test)
