include("${PROJECT_SOURCE_DIR}/cmake/third_party_imgui.cmake")

add_library(vulcal_app_imgui STATIC
    application_paths.cpp
    process.cpp
    imgui/imgui_integration.cpp
    imgui/imgui_render_target.cpp
    nfd/file_dialog.cpp
)

target_include_directories(vulcal_app_imgui PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/..")
target_link_libraries(vulcal_app_imgui
    PUBLIC
        vulcal_engine
        yaml-cpp::yaml-cpp
        nfd::nfd
        imgui::imgui
)
