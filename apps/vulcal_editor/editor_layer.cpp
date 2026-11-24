#include "vulcal_editor/editor_layer.h"

#include "base/profiler.h"

#include "asset/gpu_resource_registry.h"
#include "base/logger.h"
#include "common/imgui/imgui_integration.h"
#include "vulcal_editor/dialogs/build_settings_dialog.h"
#include "vulcal_editor/dialogs/error_dialog.h"
#include "vulcal_editor/dialogs/new_project_dialog.h"
#include "vulcal_editor/editor_actions.h"
#include "engine/application.h"
#include "gfx/command.h"
#include "render/renderer.h"

#include <imgui.h>
#include <imgui_internal.h>
#include <utility>

namespace editor {

EditorLayer::EditorLayer()
        : engine::Layer("EditorLayer") {}

EditorLayer::~EditorLayer() { stop(); }

void EditorLayer::play() {
    if (context_.playing()) return;
    viewport_.release_camera();
    context_.begin_play();
    if (!context_.uses_primary_camera()) {
        base::log::warn("Editor: scene has no primary camera; using the editor camera while playing");
    }
    try {
        script_runtime_.start(context_.scene(), context_.project_directory());
    } catch (...) {
        stop();
        throw;
    }
}

void EditorLayer::stop() {
    script_runtime_.stop();
    context_.end_play();
}

void EditorLayer::on_attach() {
    imgui_   = std::make_unique<apps::ImGuiIntegration>(application().get_window());
    auto& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.IniFilename                       = "vulcal_editor.ini";
    io.ConfigWindowsMoveFromTitleBarOnly = true;

    renderer_                            = std::make_unique<vulcal::render::Renderer>();
    gpu_resources_                       = std::make_unique<vulcal::asset::GPUResourceRegistry>(assets_, *renderer_);
    extractor_                           = std::make_unique<vulcal::scene::SceneExtractor>(*gpu_resources_);
    actions_                             = std::make_unique<EditorActions>(context_, assets_, *gpu_resources_);
    new_project_dialog_                  = std::make_unique<NewProjectDialog>(context_, *actions_);
    build_settings_dialog_               = std::make_unique<BuildSettingsDialog>(context_, *actions_);
    error_dialog_                        = std::make_unique<ErrorDialog>();

    actions_->new_scene();
    const auto entity_count = context_.scene().registry().view<vulcal::scene::TagComponent>().size();
    base::log::info("Editor initialized with {} entities", entity_count);
}

void EditorLayer::on_detach() {
    stop();
    viewport_.release_resources();
    error_dialog_.reset();
    build_settings_dialog_.reset();
    new_project_dialog_.reset();
    actions_.reset();
    imgui_.reset();
    extractor_.reset();
    gpu_resources_.reset();
    assets_.clear();
    renderer_.reset();
}

void EditorLayer::draw_file_menu() {
    if (!ImGui::BeginMenu("File")) return;

    ImGui::BeginDisabled(context_.playing());

    if (ImGui::MenuItem("New Project...")) {
        viewport_.release_camera();
        new_project_dialog_->open();
    }
    if (ImGui::MenuItem("Open Project...")) actions_->run_guarded([this] { actions_->open_project(); });

    ImGui::Separator();

    ImGui::BeginDisabled(!context_.has_project());
    if (ImGui::MenuItem("Build Settings...", nullptr, false, context_.has_project())) {
        build_settings_dialog_->open();
    }
    if (ImGui::MenuItem("Build", nullptr, false, context_.has_project() && !context_.playing()))
        actions_->run_guarded([this] { actions_->build_project(); });
    ImGui::Separator();
    if (ImGui::MenuItem("New Scene", "Ctrl+N")) actions_->run_guarded([this] { actions_->new_scene(); });
    if (ImGui::MenuItem("Open Scene...", "Ctrl+O")) actions_->run_guarded([this] { actions_->open_scene(); });

    ImGui::BeginDisabled(!context_.has_scene());
    if (ImGui::MenuItem("Save Scene", "Ctrl+S")) actions_->run_guarded([this] { actions_->save_scene(); });
    if (ImGui::MenuItem("Save Scene As...", "Ctrl+Shift+S")) actions_->run_guarded([this] { actions_->save_scene_as(); });
    ImGui::EndDisabled();

    ImGui::EndDisabled();

    ImGui::EndDisabled();

    ImGui::EndMenu();
}

void EditorLayer::handle_shortcuts() {
    if (!context_.has_project() || context_.playing() || viewport_.controlling_camera() || ImGui::GetIO().WantTextInput ||
        ImGui::IsAnyItemActive() || ImGui::IsPopupOpen(nullptr, ImGuiPopupFlags_AnyPopupId | ImGuiPopupFlags_AnyPopupLevel))
        return;

    const bool new_scene     = ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_N, ImGuiInputFlags_RouteGlobal);
    const bool open_scene    = ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_O, ImGuiInputFlags_RouteGlobal);
    const bool save_scene_as = ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiMod_Shift | ImGuiKey_S, ImGuiInputFlags_RouteGlobal);
    const bool save_scene    = ImGui::Shortcut(ImGuiMod_Ctrl | ImGuiKey_S, ImGuiInputFlags_RouteGlobal);
    if (new_scene) {
        actions_->run_guarded([this] { actions_->new_scene(); });
    } else if (open_scene) {
        actions_->run_guarded([this] { actions_->open_scene(); });
    } else if (context_.has_scene() && save_scene_as) {
        actions_->run_guarded([this] { actions_->save_scene_as(); });
    } else if (context_.has_scene() && save_scene) {
        actions_->run_guarded([this] { actions_->save_scene(); });
    }
}

void EditorLayer::draw_workspace() {
    if (ImGui::BeginMainMenuBar()) {
        draw_file_menu();
        if (ImGui::BeginMenu("Scene")) {
            if (ImGui::MenuItem("Play", nullptr, false, context_.has_scene() && !context_.playing()))
                actions_->run_guarded([this] { play(); });
            if (ImGui::MenuItem("Stop", nullptr, false, context_.playing()))
                actions_->run_guarded([this] { stop(); });
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("View")) {
            ImGui::MenuItem("Hierarchy", nullptr, &hierarchy_open_);
            ImGui::MenuItem("Inspector", nullptr, &inspector_open_);
            ImGui::MenuItem("Viewport", nullptr, &viewport_open_);
            ImGui::Separator();
            if (ImGui::MenuItem("Reset Layout")) {
                reset_layout_   = true;
                hierarchy_open_ = inspector_open_ = viewport_open_ = true;
            }
            ImGui::EndMenu();
        }
        ImGui::Separator();
        ImGui::Text("%.1f FPS", ImGui::GetIO().Framerate);
        ImGui::EndMainMenuBar();
    }

    const auto* viewport    = ImGui::GetMainViewport();
    const ImGuiID dockspace = ImGui::GetID("EditorDockSpace");
    if (reset_layout_ || ImGui::DockBuilderGetNode(dockspace) == nullptr) {
        // DockBuilder's internal API is confined to initial layout creation.
        ImGui::DockBuilderRemoveNode(dockspace);
        ImGui::DockBuilderAddNode(dockspace, ImGuiDockNodeFlags_DockSpace);
        ImGui::DockBuilderSetNodeSize(dockspace, viewport->WorkSize);
        ImGuiID center      = dockspace;
        const ImGuiID left  = ImGui::DockBuilderSplitNode(center, ImGuiDir_Left, 0.18f, nullptr, &center);
        const ImGuiID right = ImGui::DockBuilderSplitNode(center, ImGuiDir_Right, 0.28f, nullptr, &center);
        ImGui::DockBuilderDockWindow("Hierarchy", left);
        ImGui::DockBuilderDockWindow("Inspector", right);
        ImGui::DockBuilderDockWindow("Viewport", center);
        ImGui::DockBuilderFinish(dockspace);
        reset_layout_ = false;
    }
    ImGui::DockSpaceOverViewport(dockspace, viewport);
}

void EditorLayer::on_update(base::Timestep dt) {
    VULCAL_PROFILE_SCOPE();
    if (context_.playing()) script_runtime_.update(dt.get_seconds());
    imgui_->begin_frame();
    draw_workspace();
    hierarchy_.draw(context_, hierarchy_open_);
    inspector_.draw(context_, inspector_open_, actions_.get());
    viewport_.draw(viewport_open_, application().get_graphics_context().limits());
    new_project_dialog_->draw();
    build_settings_dialog_->draw();
    if (!context_.playing()) {
        viewport_.update_camera(dt.get_seconds(), application().get_window().handle());
    }
    handle_shortcuts();
    if (auto error = actions_->take_error()) error_dialog_->open(std::move(*error));
    error_dialog_->draw();
    imgui_->end_frame();
}

void EditorLayer::on_render() {
    VULCAL_PROFILE_SCOPE();
    if (viewport_.visible()) {
        auto& target = viewport_.target();
        vulcal::scene::SceneExtractor::Options options;
        options.aspect_ = viewport_.aspect();
        if (!context_.uses_primary_camera()) {
            options.camera_override_ = viewport_.camera_data();
        }
        renderer_->render(extractor_->extract(context_.scene(), options), &target);
    }
    vulcal::gfx::RenderTarget::unbind();
    const auto& window = application().get_window();
    vulcal::gfx::Command::set_viewport(window.width(), window.height());
    vulcal::gfx::Command::clear(glm::vec4{ 0.08f, 0.08f, 0.08f, 1.0f });
    imgui_->render();
}

} // namespace editor
