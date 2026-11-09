add_executable(editor_state_test
    main.cpp
    play.cpp
    assets.cpp
    ../../apps/editor/editor_context.cpp
    ../../apps/editor/editor_camera.cpp
    ../../apps/editor/project/project_manager.cpp
    ../../apps/editor/project/scene/default_scene.cpp
    ../../apps/editor/project/scene/scene_serializer.cpp
    snake.cpp
    arena.cpp
)
target_include_directories(editor_state_test PRIVATE ../../apps)
target_link_libraries(editor_state_test PRIVATE vulcal_engine yaml-cpp::yaml-cpp GTest::gtest_main Lua::Lua sol2::sol2)
file(TO_CMAKE_PATH "${CMAKE_SOURCE_DIR}" VULCAL_TEST_SOURCE_DIR)
target_compile_definitions(editor_state_test PRIVATE VULCAL_TEST_SOURCE_DIR="${VULCAL_TEST_SOURCE_DIR}")

if (BUILD_APPS)
    target_sources(editor_state_test PRIVATE
        importer.cpp
    )
    target_link_libraries(editor_state_test PRIVATE vulcal_asset_pipeline)
endif()

include(GoogleTest)
gtest_discover_tests(editor_state_test)

if (BUILD_APPS)
    # Headless ImGui interaction tests; no window or OpenGL context is required.
    add_executable(editor_panel_test panels.cpp)
    target_link_libraries(editor_panel_test PRIVATE vulcal_editor GTest::gtest_main)
    gtest_discover_tests(editor_panel_test)

    if (BUILD_GRAPHICS_TESTS)
        add_executable(arena_graphics_test arena_graphics.cpp)
        target_link_libraries(arena_graphics_test PRIVATE vulcal_editor GTest::gtest_main)
        target_compile_definitions(arena_graphics_test PRIVATE VULCAL_TEST_SOURCE_DIR="${VULCAL_TEST_SOURCE_DIR}")
        gtest_discover_tests(arena_graphics_test PROPERTIES LABELS graphics RUN_SERIAL TRUE)

        add_executable(editor_actions_test actions.cpp)
        target_link_libraries(editor_actions_test PRIVATE vulcal_editor GTest::gtest_main)
        gtest_discover_tests(editor_actions_test PROPERTIES LABELS graphics)

        add_executable(editor_viewport_test viewport.cpp)
        target_link_libraries(editor_viewport_test PRIVATE vulcal_editor GTest::gtest_main)
        gtest_discover_tests(editor_viewport_test PROPERTIES LABELS graphics RUN_SERIAL TRUE)

        add_executable(editor_layer_test layer.cpp)
        target_link_libraries(editor_layer_test PRIVATE vulcal_editor GTest::gtest_main)
        gtest_discover_tests(editor_layer_test PROPERTIES LABELS graphics)
    endif()
endif()
