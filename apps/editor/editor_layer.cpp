#include "editor/editor_layer.h"

#include "asset/render_asset_cache.h"
#include "asset_import/obj_asset_importer.h"
#include "base/input.h"
#include "base/logger.h"
#include "common/imgui/imgui_integration.h"
#include "common/nfd/file_dialog.h"
#include "engine/application.h"
#include "gfx/command.h"
#include "render/renderer.h"

#include <SDL3/SDL.h>
#include <imgui.h>
#include <imgui_internal.h>
#include <misc/cpp/imgui_stdlib.h>

namespace editor {
namespace {

const apps::nfd::FileFilter project_filter{ "Vulcal Project", "vproj" };
const apps::nfd::FileFilter scene_filter{ "Vulcal Scene", "vscene" };
const apps::nfd::FileFilter obj_filter{ "Wavefront OBJ", "obj" };

std::string to_utf8(const std::filesystem::path& path) {
    const auto text = path.u8string();
    return std::string(reinterpret_cast<const char*>(text.data()), text.size());
}

} // namespace

EditorLayer::EditorLayer()
        : engine::Layer("EditorLayer") {}

EditorLayer::~EditorLayer() = default;

void EditorLayer::on_attach() {
    imgui_   = std::make_unique<apps::ImGuiIntegration>(application().get_window());
    auto& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.IniFilename                       = "vulcal_editor.ini";
    io.ConfigWindowsMoveFromTitleBarOnly = true;

    renderer_                            = std::make_unique<vulcal::render::Renderer>();
    render_assets_                       = std::make_unique<vulcal::asset::RenderAssetCache>(assets_, *renderer_);
    extractor_                           = std::make_unique<vulcal::scene::SceneExtractor>(*render_assets_);

    context_.new_scene();
    const auto entity_count = context_.scene().registry().view<vulcal::scene::TagComponent>().size();
    base::log::info("Editor initialized with {} entities", entity_count);
}

void EditorLayer::on_detach() {
    release_camera();
    imgui_.reset();
    extractor_.reset();
    render_assets_.reset();
    assets_.clear();
    renderer_.reset();
}

void EditorLayer::run_operation(const std::function<void()>& operation) {
    release_camera();
    try {
        operation();
    } catch (const std::exception& error) {
        base::log::error("Editor operation failed: {}", error.what());
        error_message_   = error.what();
        error_requested_ = true;
    }
}

void EditorLayer::open_scene() {
    // TODO: 也许 apps::ndf 可以下沉到下层去 ndf 只需要 filter 和 context 的状态，让 EditorContext 依赖 ndf
    // 我不太希望再在 EditorLayer 包含 open_scene sava_scene sava_scene_as 等这类方法
    if (auto path = apps::nfd::open_file(scene_filter, context_.scene_dialog_directory())) {
        context_.open_scene(*path);
    }
}

void EditorLayer::save_scene() {
    // TODO: 很显然这里处理的逻辑只有 EditorContext 中的状态，没有使用到除了 EditorContext 之外的状态，应该将这个逻辑下沉到 EditorContext 中去处理
    if (context_.save_scene() == EditorContext::SaveResult::NeedsPath) save_scene_as();
}

void EditorLayer::save_scene_as() {
    const auto directory = context_.scene_dialog_directory();
    const auto name      = context_.suggested_scene_name();
    if (auto path = apps::nfd::save_file(scene_filter, directory, name)) {
        context_.save_scene_as(*path);
    }
}

void EditorLayer::draw_file_menu() {
    if (!ImGui::BeginMenu("File")) return;

    if (ImGui::MenuItem("New Project...")) {
        release_camera();
        new_project_name_.clear();
        new_project_directory_ = std::filesystem::current_path();
        new_project_error_.clear();
        new_project_requested_ = true;
    }
    if (ImGui::MenuItem("Open Project...")) {
        run_operation([this] {
            if (auto path = apps::nfd::open_file(project_filter)) {
                context_.open_project(*path);
                reset_project_resources();
            }
        });
    }

    if (ImGui::MenuItem("Import OBJ...", nullptr, false, context_.has_project())) run_operation([this] { import_obj(); });

    ImGui::Separator();

    if (ImGui::MenuItem("New Scene")) run_operation([this] { context_.new_scene(); });
    if (ImGui::MenuItem("Open Scene...")) run_operation([this] { open_scene(); });

    ImGui::BeginDisabled(!context_.has_scene());
    if (ImGui::MenuItem("Save Scene")) run_operation([this] { save_scene(); });
    if (ImGui::MenuItem("Save Scene As...")) run_operation([this] { save_scene_as(); });
    ImGui::EndDisabled();

    ImGui::EndMenu();
}

void EditorLayer::import_obj() {
    // TODO: 这里不应该存在 Import OBJ 或许应该由 Asset Pipeline 来处理，放在这里很莫名奇妙
    if (auto path = apps::nfd::open_file(obj_filter, context_.assets_directory())) {
        const auto id = apps::asset_import::ObjAssetImporter::import(*path, context_.assets_directory(), context_.library_directory());
        const auto selected = context_.selected_entity();
        if (selected) {
            auto& component          = selected.has_component<vulcal::scene::MeshRendererComponent>()
                                               ? selected.get_component<vulcal::scene::MeshRendererComponent>()
                                               : selected.add_component<vulcal::scene::MeshRendererComponent>();
            component.mesh_asset_id_ = id;
            base::log::debug("Editor: assigned mesh asset {} to entity '{}' ({})", id.to_string(),
                             selected.get_component<vulcal::scene::TagComponent>().name_,
                             selected.get_component<vulcal::scene::IDComponent>().id_.to_string());
        }
        render_assets_->invalidate(id);
        assets_.invalidate(id);
    }
}

void EditorLayer::reset_project_resources() {
    render_assets_->clear();
    assets_.set_directory(context_.library_directory());
}

void EditorLayer::draw_new_project_dialog() {
    if (new_project_requested_) {
        ImGui::OpenPopup("New Project");
        new_project_requested_ = false;
    }

    const auto center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, { 0.5f, 0.5f });
    if (!ImGui::BeginPopupModal("New Project", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) return;

    ImGui::SetNextItemWidth(360.0f);
    ImGui::InputText("Project name", &new_project_name_);
    ImGui::TextUnformatted("Project directory");
    ImGui::TextWrapped("%s", new_project_directory_.empty() ? "Choose an existing directory" : to_utf8(new_project_directory_).c_str());
    if (ImGui::Button("Browse...")) {
        try {
            if (auto path = apps::nfd::pick_folder(new_project_directory_)) {
                new_project_directory_ = *path;
                if (new_project_name_.empty()) new_project_name_ = to_utf8(new_project_directory_.filename());
            }
        } catch (const std::exception& error) {
            base::log::error("Editor: could not choose project directory: {}", error.what());
            new_project_error_ = error.what();
        }
    }

    if (!new_project_error_.empty()) {
        ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + 440.0f);
        ImGui::TextColored({ 1.0f, 0.4f, 0.3f, 1.0f }, "%s", new_project_error_.c_str());
        ImGui::PopTextWrapPos();
    }

    const bool can_create = !new_project_name_.empty() && !new_project_directory_.empty();
    ImGui::BeginDisabled(!can_create);
    if (ImGui::Button("Create")) {
        try {
            context_.create_project(new_project_directory_, new_project_name_);
            reset_project_resources();
            new_project_error_.clear();
            ImGui::CloseCurrentPopup();
        } catch (const std::exception& error) {
            base::log::error("Editor: could not create project '{}' in {}: {}", new_project_name_, new_project_directory_.string(),
                             error.what());
            new_project_error_ = error.what();
        }
    }
    ImGui::EndDisabled();
    ImGui::SameLine();
    if (ImGui::Button("Cancel")) {
        new_project_error_.clear();
        ImGui::CloseCurrentPopup();
    }

    ImGui::EndPopup();
}

void EditorLayer::draw_error_dialog() {
    if (error_requested_) {
        ImGui::OpenPopup("Operation Failed");
        error_requested_ = false;
    }

    const auto center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, { 0.5f, 0.5f });
    if (!ImGui::BeginPopupModal("Operation Failed", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) return;

    ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + 480.0f);
    ImGui::TextUnformatted(error_message_.c_str());
    ImGui::PopTextWrapPos();
    if (ImGui::Button("OK")) ImGui::CloseCurrentPopup();

    ImGui::EndPopup();
}

void EditorLayer::draw_workspace() {
    if (ImGui::BeginMainMenuBar()) {
        draw_file_menu();
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
    imgui_->begin_frame();
    draw_workspace();
    hierarchy_.draw(context_, hierarchy_open_);
    inspector_.draw(context_, inspector_open_);
    viewport_.draw(viewport_open_, camera_.speed(), application().get_graphics_context().limits());
    draw_new_project_dialog();
    draw_error_dialog();
    update_camera(dt);
    imgui_->end_frame();
}

void EditorLayer::update_camera(base::Timestep dt) {
    auto* window          = application().get_window().handle();
    const bool focused    = (SDL_GetWindowFlags(window) & SDL_WINDOW_INPUT_FOCUS) != 0;
    const bool popup      = ImGui::IsPopupOpen(nullptr, ImGuiPopupFlags_AnyPopupId | ImGuiPopupFlags_AnyPopupLevel);
    const bool escape     = base::Input::is_key_down(base::Key::Escape);
    const bool right_down = base::Input::is_mouse_button_down(base::MouseButton::Right);
    if (controlling_camera_ && (!right_down || escape || !focused || !viewport_.visible() || !viewport_.focused() || popup)) {
        release_camera();
    }

    bool started = false;
    if (!controlling_camera_ && viewport_.camera_requested() && right_down && focused && !escape && !popup) {
        const auto [x, y] = base::Input::mouse_position();
        saved_mouse_x_    = x;
        saved_mouse_y_    = y;
        if (SDL_SetWindowRelativeMouseMode(window, true)) {
            controlling_camera_ = true;
            started             = true;
            ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;
            SDL_HideCursor();
            base::log::debug("Editor camera control started");
        } else {
            base::log::warn("Could not capture editor camera mouse: {}", SDL_GetError());
        }
    }
    if (!controlling_camera_) return;

    camera_.update_from_input(dt.get_seconds(), started);
    ImGui::SetMouseCursor(ImGuiMouseCursor_None);
}

void EditorLayer::release_camera() {
    if (!controlling_camera_) return;
    auto* window = application().get_window().handle();
    SDL_SetWindowRelativeMouseMode(window, false);
    if ((SDL_GetWindowFlags(window) & SDL_WINDOW_INPUT_FOCUS) != 0) {
        SDL_WarpMouseInWindow(window, saved_mouse_x_, saved_mouse_y_);
    }
    SDL_ShowCursor();
    ImGui::GetIO().ConfigFlags &= ~ImGuiConfigFlags_NoMouseCursorChange;
    controlling_camera_ = false;
    base::log::debug("Editor camera control released");
}

void EditorLayer::on_render() {
    if (viewport_.visible()) {
        auto& target = viewport_.target();
        vulcal::scene::SceneExtractor::Options options;
        options.camera_override_ = camera_.data(static_cast<float>(target.width()) / static_cast<float>(target.height()));
        renderer_->render(extractor_->extract(context_.scene(), options), &target);
    }
    vulcal::gfx::RenderTarget::unbind();
    const auto& window = application().get_window();
    vulcal::gfx::Command::set_viewport(window.width(), window.height());
    vulcal::gfx::Command::clear(glm::vec4{ 0.08f, 0.08f, 0.08f, 1.0f });
    imgui_->render();
}

} // namespace editor
