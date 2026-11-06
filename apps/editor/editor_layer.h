#pragma once

#include "asset/asset_manager.h"
#include "editor/editor_context.h"
#include "editor/panels/hierarchy_panel.h"
#include "editor/panels/inspector_panel.h"
#include "editor/panels/viewport_panel.h"
#include "engine/layer.h"
#include "scene/scene_extractor.h"

#include <filesystem>
#include <memory>
#include <string>

namespace apps {
class ImGuiIntegration;
}
namespace vulcal::render {
class Renderer;
}
namespace vulcal::asset {
class RenderAssetCache;
}

namespace editor {

class EditorActions;

// Assembles editor services and panels, manages their lifetime and frame order.
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

    void handle_shortcuts();

    std::unique_ptr<apps::ImGuiIntegration> imgui_;

    vulcal::asset::AssetManager assets_;

    std::unique_ptr<vulcal::render::Renderer> renderer_;
    std::unique_ptr<vulcal::asset::RenderAssetCache> render_assets_;
    std::unique_ptr<vulcal::scene::SceneExtractor> extractor_;

    EditorContext context_;
    std::unique_ptr<EditorActions> actions_;

    HierarchyPanel hierarchy_;
    InspectorPanel inspector_;
    ViewportPanel viewport_;

    bool hierarchy_open_        = true;
    bool inspector_open_        = true;
    bool viewport_open_         = true;
    bool reset_layout_          = false;

    bool new_project_requested_ = false;
    std::string new_project_name_;
    std::filesystem::path new_project_directory_;
    std::string new_project_error_;

    bool error_requested_ = false;
    std::string error_message_;
};

} // namespace editor
