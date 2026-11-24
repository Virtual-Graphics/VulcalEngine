FetchContent_GetProperties(imgui SOURCE_DIR IMGUI_SOURCE_DIR)

add_library(vulcal_editor_lib STATIC
    editor_layer.cpp
    editor_actions.cpp
    editor_context.cpp
    editor_camera.cpp
    dialogs/dialog.cpp
    dialogs/new_project_dialog.cpp
    dialogs/build_settings_dialog.cpp
    dialogs/error_dialog.cpp
    project/scene/default_scene.cpp
    panels/viewport_panel.cpp
    panels/hierarchy_panel.cpp
    panels/inspector_panel.cpp
    project/project_manager.cpp
    "${IMGUI_SOURCE_DIR}/misc/cpp/imgui_stdlib.cpp"
)
target_compile_definitions(vulcal_editor_lib PRIVATE ASSETS_DIR="${PROJECT_SOURCE_DIR}/assets")
target_link_libraries(vulcal_editor_lib PUBLIC vulcal_app_imgui PRIVATE vulcal_asset_pipeline yaml-cpp::yaml-cpp)

add_executable(vulcal_editor main.cpp)
target_link_libraries(vulcal_editor PRIVATE vulcal_editor_lib)
