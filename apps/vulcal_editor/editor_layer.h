#pragma once

#include "asset/asset_manager.h"
#include "vulcal_editor/editor_context.h"
#include "vulcal_editor/panels/hierarchy_panel.h"
#include "vulcal_editor/panels/inspector_panel.h"
#include "vulcal_editor/panels/viewport_panel.h"
#include "engine/layer.h"
#include "scene/scene_extractor.h"
#include "script/script_runtime.h"

#include <memory>

namespace apps {
class ImGuiIntegration;
}
namespace vulcal::render {
class Renderer;
}
namespace vulcal::asset {
class GPUResourceRegistry;
}

namespace editor {

class BuildSettingsDialog;
class EditorActions;
class ErrorDialog;
class NewProjectDialog;

// Assembles editor services and panels, manages their lifetime and frame order.
class EditorLayer : public engine::Layer {
public:
    EditorLayer();
    ~EditorLayer() override;

    void on_attach() override;
    void on_detach() override;
    void on_update(base::Timestep dt) override;
    void on_render() override;

    EditorContext& context() { return context_; }
    const EditorContext& context() const { return context_; }
    void play();
    void stop();

private:
    void draw_workspace();
    void draw_file_menu();

    void handle_shortcuts();

    std::unique_ptr<apps::ImGuiIntegration> imgui_;

    vulcal::asset::AssetManager assets_;

    std::unique_ptr<vulcal::render::Renderer> renderer_;
    std::unique_ptr<vulcal::asset::GPUResourceRegistry> gpu_resources_;
    std::unique_ptr<vulcal::scene::SceneExtractor> extractor_;

    EditorContext context_;
    // Declared after context so scripts always die before their scene.
    vulcal::script::ScriptRuntime script_runtime_;
    std::unique_ptr<EditorActions> actions_;
    std::unique_ptr<NewProjectDialog> new_project_dialog_;
    std::unique_ptr<BuildSettingsDialog> build_settings_dialog_;
    std::unique_ptr<ErrorDialog> error_dialog_;

    HierarchyPanel hierarchy_;
    InspectorPanel inspector_;
    ViewportPanel viewport_;

    bool hierarchy_open_        = true;
    bool inspector_open_        = true;
    bool viewport_open_         = true;
    bool reset_layout_          = false;
};

} // namespace editor
