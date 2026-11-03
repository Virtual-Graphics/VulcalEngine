#pragma once

#include "editor/editor_camera.h"
#include "editor/editor_context.h"
#include "editor/panels/hierarchy_panel.h"
#include "editor/panels/inspector_panel.h"
#include "editor/panels/viewport_panel.h"
#include "engine/layer.h"
#include "scene/scene_extractor.h"

#include <filesystem>
#include <functional>
#include <memory>
#include <string>

namespace apps {
class ImGuiIntegration;
}
namespace vulcal::render {
class Renderer;
}

namespace editor {

class EditorLayer : public engine::Layer {
public:
    EditorLayer();
    ~EditorLayer() override;

    void on_attach() override;
    void on_detach() override;
    void on_update(base::Timestep dt) override;
    void on_render() override;

private:
    void draw_workspace();
    void draw_file_menu();
    void draw_new_project_dialog();
    void draw_error_dialog();
    void open_scene();
    void save_scene();
    void save_scene_as();
    void run_operation(const std::function<void()>& operation);
    void update_camera(base::Timestep dt);
    void release_camera();

    std::unique_ptr<apps::ImGuiIntegration> imgui_;
    std::unique_ptr<vulcal::render::Renderer> renderer_;
    EditorContext context_;
    EditorCamera camera_;
    vulcal::scene::SceneExtractor extractor_;
    HierarchyPanel hierarchy_;
    InspectorPanel inspector_;
    ViewportPanel viewport_;
    bool hierarchy_open_ = true;
    bool inspector_open_ = true;
    bool viewport_open_ = true;
    bool reset_layout_ = false;
    bool controlling_camera_ = false;
    float saved_mouse_x_ = 0.0f;
    float saved_mouse_y_ = 0.0f;

    bool new_project_requested_ = false;
    std::string new_project_name_;
    std::filesystem::path new_project_directory_;
    std::string new_project_error_;

    bool error_requested_ = false;
    std::string error_message_;
};

} // namespace editor
