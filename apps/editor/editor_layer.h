#pragma once

#include "asset/asset_manager.h"
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
namespace vulcal::asset {
class RenderAssetCache;
}

namespace editor {

// EditorLayer 的职责应该是集中管理各个 Panel 以及 Kernel 中的模块，处理 on_xxx 钩子函数，其余的操作应该都放到
// 对应的模块中处理，或者 Editor Context 中处理，EditorLayer 不应该直接操作 ProjectManager 或者其他模块。
// EditorLayer 应该作为一种“调度者”的存在，其他模块需要使用另一些模块的功能时，应该通过 EditorLayer 来调度
// 例如 EditorLayer 可以拥有 EditorContext，Renderer，AssetManager 等模块的所有权，其他模块需要使用这些模块的功能时，
// 应该通过 EditorLayer 来调度，而不是直接依赖这些模块
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


    void import_obj(); // TODO: 这里不应该存在 Import OBJ 或许应该由 Asset Pipeline 来处理，放在这里很莫名奇妙
    void reset_project_resources();

    // TODO: 菜单操作，Project 相关的去哪了，为什么 Project 的可以不存在而 Scene 的方法需要存在，这从某些角度体现出了设计问题
    void open_scene();
    void save_scene();
    void save_scene_as();

    void run_operation(const std::function<void()>& operation);

    // TODO: 也许应该放入 Viewport Panel 中，因为只有 Viewport Panel 才会使用到 Camera
    void update_camera(base::Timestep dt);
    void release_camera();

    std::unique_ptr<apps::ImGuiIntegration> imgui_;

    vulcal::asset::AssetManager assets_;

    std::unique_ptr<vulcal::render::Renderer> renderer_;
    std::unique_ptr<vulcal::asset::RenderAssetCache> render_assets_;
    std::unique_ptr<vulcal::scene::SceneExtractor> extractor_;

    EditorContext context_;
    EditorCamera camera_; // TODO: 也许放入 Viewport Panel 中更合适

    HierarchyPanel hierarchy_;
    InspectorPanel inspector_;
    ViewportPanel viewport_;

    bool hierarchy_open_        = true;
    bool inspector_open_        = true;
    bool viewport_open_         = true;
    bool reset_layout_          = false;
    bool controlling_camera_    = false;
    float saved_mouse_x_        = 0.0f;
    float saved_mouse_y_        = 0.0f;

    bool new_project_requested_ = false;
    std::string new_project_name_;
    std::filesystem::path new_project_directory_;
    std::string new_project_error_;

    bool error_requested_ = false;
    std::string error_message_;
};

} // namespace editor
