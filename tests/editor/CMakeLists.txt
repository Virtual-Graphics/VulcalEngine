add_executable(editor_state_test
    main.cpp
    ../../apps/editor/editor_context.cpp
    ../../apps/editor/editor_camera.cpp
    ../../apps/editor/project/project_manager.cpp
    ../../apps/editor/project/scene/default_scene.cpp
    ../../apps/editor/project/scene/scene_serializer.cpp
)
target_include_directories(editor_state_test PRIVATE ../../apps)
target_link_libraries(editor_state_test PRIVATE vulcal_engine yaml-cpp::yaml-cpp GTest::gtest_main)

include(GoogleTest)
gtest_discover_tests(editor_state_test)

if (BUILD_APPS)
    # Headless ImGui interaction tests; no window or OpenGL context is required.
    add_executable(editor_panel_test panels.cpp)
    target_link_libraries(editor_panel_test PRIVATE vulcal_editor GTest::gtest_main)
    gtest_discover_tests(editor_panel_test)
endif()
