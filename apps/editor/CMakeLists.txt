FetchContent_GetProperties(imgui SOURCE_DIR IMGUI_SOURCE_DIR)

add_library(vulcal_editor STATIC
    editor_layer.cpp
    editor_context.cpp
    editor_camera.cpp
    project/scene/default_scene.cpp
    panels/viewport_panel.cpp
    panels/hierarchy_panel.cpp
    panels/inspector_panel.cpp
    project/project_manager.cpp
    project/scene/scene_serializer.cpp
    "${IMGUI_SOURCE_DIR}/misc/cpp/imgui_stdlib.cpp"
)
target_compile_definitions(vulcal_editor PRIVATE ASSETS_DIR="${PROJECT_SOURCE_DIR}/assets")
target_link_libraries(vulcal_editor PUBLIC vulcal_app_imgui PRIVATE yaml-cpp::yaml-cpp )

add_executable(editor main.cpp)
target_link_libraries(editor PRIVATE vulcal_editor)
