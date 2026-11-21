#include "vulcal_editor/editor_actions.h"

#include "asset/asset_manager.h"
#include "asset/gpu_resource_registry.h"
#include "asset_import/importers/material_asset_importer.h"
#include "asset_import/importers/obj_asset_importer.h"
#include "asset_import/importers/texture_asset_importer.h"
#include "base/logger.h"
#include "base/path_utils.h"
#include "base/string_utils.h"
#include "common/application_paths.h"
#include "common/nfd/file_dialog.h"
#include "common/process.h"
#include "common/project_paths.h"
#include "vulcal_editor/editor_context.h"
#include "scene/components.h"

#include <stdexcept>
#include <string>
#include <utility>

namespace editor {
namespace {

const apps::nfd::FileFilter project_filter{ "Vulcal Project", "vproj" };
const apps::nfd::FileFilter scene_filter{ "Vulcal Scene", "vscene" };
const apps::nfd::FileFilter obj_filter{ "Wavefront OBJ", "obj" };
const apps::nfd::FileFilter material_filter{ "Vulcal Material", "vmat" };
const apps::nfd::FileFilter texture_filter{ "Image", "png,jpg,jpeg,bmp,tga" };
const apps::nfd::FileFilter script_filter{ "Lua Script", "lua" };

std::filesystem::path absolute_path(const std::filesystem::path& path) {
    std::error_code error;
    const auto absolute = std::filesystem::absolute(path, error);
    if (error) throw std::runtime_error("Could not resolve absolute path: " + path.string());
    return absolute.lexically_normal();
}

} // namespace

EditorActions::EditorActions(EditorContext& context, vulcal::asset::AssetManager& assets, vulcal::asset::GPUResourceRegistry& gpu_resources)
        : EditorActions(context, assets, gpu_resources,
                        Dialogs{
                            .open_file = [](const auto& filter, const auto& directory) { return apps::nfd::open_file(filter, directory); },
                            .save_file = [](const auto& filter, const auto& directory,
                                            const auto& name) { return apps::nfd::save_file(filter, directory, name); },
                            .find_executable = [](std::string_view name) { return apps::app_paths::find_executable(name); },
                            .run_process = [](const auto& executable, const auto& arguments) {
                                return apps::process::run_and_wait(executable, arguments);
                            },
                        }) {}

EditorActions::EditorActions(EditorContext& context, vulcal::asset::AssetManager& assets, vulcal::asset::GPUResourceRegistry& gpu_resources,
                             Dialogs dialogs)
        : context_(context),
          assets_(assets),
          gpu_resources_(gpu_resources),
          dialogs_(std::move(dialogs)) {}

void EditorActions::create_project(const std::filesystem::path& directory, const std::string& name) {
    context_.create_project(directory, name);
    reset_project_resources();
}

void EditorActions::open_project() {
    context_.require_edit_mode();
    if (auto path = dialogs_.open_file(project_filter, {})) {
        context_.open_project(*path);
        reset_project_resources();
    }
}

void EditorActions::build_project() {
    context_.require_edit_mode();
    if (!context_.has_project()) throw std::logic_error("Open a project before building");
    const auto& build_info = context_.project_info().build_info;
    if (!build_info || build_info->open_scene.empty()) throw std::logic_error("Configure Build Settings before building");
    if (!dialogs_.find_executable || !dialogs_.run_process) throw std::logic_error("Build process services are unavailable");

    const auto build_tool = dialogs_.find_executable("vulcal_build");
    if (build_tool.empty()) throw std::runtime_error("Could not find vulcal_build beside the editor or on PATH");
    const auto player = dialogs_.find_executable("vulcal_player");
    if (player.empty()) throw std::runtime_error("Could not find vulcal_player beside the editor or on PATH");

    const std::vector<std::string> arguments{
        base::path_to_utf8(absolute_path(context_.project_file())),
        base::path_to_utf8(absolute_path(player)),
    };
    const auto exit_code = dialogs_.run_process(absolute_path(build_tool), arguments);
    if (exit_code != 0) throw std::runtime_error("Build failed with exit code " + std::to_string(exit_code));
}

void EditorActions::set_build_settings(const std::filesystem::path& scene_file, const std::filesystem::path& build_dir) {
    context_.require_edit_mode();
    if (!context_.has_project()) throw std::logic_error("Open a project before configuring build settings");
    std::error_code error;
    const auto scene = std::filesystem::canonical(scene_file, error);
    if (error || !std::filesystem::is_regular_file(scene)) {
        throw std::invalid_argument("Select an existing .vscene inside the project assets directory");
    }

    std::filesystem::path resolved_build_dir;
    if (!build_dir.empty()) {
        resolved_build_dir = absolute_path(build_dir);
    }
    const auto relative_scene  = base::relative_to(scene, context_.project_directory());
    const auto relative_assets = base::relative_to(context_.assets_directory(), context_.project_directory());
    if (relative_scene.empty() || relative_assets.empty()) {
        throw std::invalid_argument("Build scene must be inside the project directory");
    }
    apps::project_paths::validate_scene_path(relative_scene, relative_assets, "Build scene");
    context_.set_build_info(BuildInfo{ .open_scene = relative_scene, .build_dir = resolved_build_dir });
}

void EditorActions::new_scene() { context_.new_scene(); }

void EditorActions::open_scene() {
    context_.require_edit_mode();
    if (auto path = dialogs_.open_file(scene_filter, context_.scene_dialog_directory())) context_.open_scene(*path);
}

void EditorActions::save_scene() {
    if (context_.save_scene() == EditorContext::SaveResult::NeedsPath) save_scene_as();
}

void EditorActions::save_scene_as() {
    context_.require_edit_mode();
    if (auto path = dialogs_.save_file(scene_filter, context_.scene_dialog_directory(), context_.suggested_scene_name())) {
        context_.save_scene_as(*path);
    }
}

vulcal::scene::Entity EditorActions::require_selected_mesh_renderer(const char* asset_name) const {
    context_.require_edit_mode();
    if (!context_.has_project()) throw std::logic_error("Open a project before importing assets");
    const auto target = context_.selected_entity();
    if (!context_.has_scene() || !target || !target.has_component<vulcal::scene::MeshRendererComponent>()) {
        throw std::invalid_argument(std::string("Select an entity with a Mesh Renderer before importing a ") + asset_name);
    }
    return target;
}

vulcal::scene::Entity EditorActions::require_selected_sprite_renderer(const char* asset_name) const {
    context_.require_edit_mode();
    if (!context_.has_project()) throw std::logic_error("Open a project before importing assets");
    const auto target = context_.selected_entity();
    if (!context_.has_scene() || !target || !target.has_component<vulcal::scene::SpriteRendererComponent>()) {
        throw std::invalid_argument(std::string("Select an entity with a Sprite Renderer before importing a ") + asset_name);
    }
    return target;
}

vulcal::scene::Entity EditorActions::require_selected_script() const {
    context_.require_edit_mode();
    if (!context_.has_project()) throw std::logic_error("Open a project before assigning a script");
    const auto target = context_.selected_entity();
    if (!context_.has_scene() || !target || !target.has_component<vulcal::scene::ScriptComponent>()) {
        throw std::invalid_argument("Select an entity with a Script component before assigning a script");
    }
    return target;
}

void EditorActions::assign_script() {
    const auto target = require_selected_script();
    if (auto path = dialogs_.open_file(script_filter, context_.assets_directory())) {
        if (context_.selected_entity() != target) throw std::runtime_error("The selected entity changed during script assignment");
        std::error_code error;
        const auto selected = std::filesystem::canonical(*path, error);
        if (error || !base::has_extension(selected, ".lua") || !std::filesystem::is_regular_file(selected)) {
            throw std::invalid_argument("Select an existing .lua script inside the project directory");
        }
        const auto relative = base::relative_to(selected, context_.project_directory());
        if (relative.empty()) {
            throw std::invalid_argument("The selected script is outside the project directory");
        }
        target.get_component<vulcal::scene::ScriptComponent>().script_path_ = base::path_to_utf8(relative);
    }
}

void EditorActions::import_mesh_asset() {
    const auto target = require_selected_mesh_renderer("mesh");
    if (auto path = dialogs_.open_file(obj_filter, context_.assets_directory())) {
        if (context_.selected_entity() != target) throw std::runtime_error("The selected entity changed during mesh import");
        const auto id = apps::asset_import::ObjAssetImporter::import(*path, context_.project_directory(), context_.library_directory());
        target.get_component<vulcal::scene::MeshRendererComponent>().mesh_asset_id_ = id;
        base::log::debug("Editor: assigned mesh asset {} to entity '{}' ({})", id.to_string(),
                         target.get_component<vulcal::scene::TagComponent>().name_,
                         target.get_component<vulcal::scene::IDComponent>().id_.to_string());
        gpu_resources_.invalidate(id);
        assets_.invalidate(id);
    }
}

void EditorActions::import_material_asset() {
    const auto target = require_selected_mesh_renderer("material");
    if (auto path = dialogs_.open_file(material_filter, context_.assets_directory())) {
        if (context_.selected_entity() != target) throw std::runtime_error("The selected entity changed during material import");
        const auto id =
                apps::asset_import::MaterialAssetImporter::import(*path, context_.project_directory(), context_.library_directory());
        target.get_component<vulcal::scene::MeshRendererComponent>().material_asset_id_ = id;
        base::log::debug("Editor: assigned material asset {} to entity '{}' ({})", id.to_string(),
                         target.get_component<vulcal::scene::TagComponent>().name_,
                         target.get_component<vulcal::scene::IDComponent>().id_.to_string());
        gpu_resources_.invalidate(id);
        assets_.invalidate(id);
    }
}

void EditorActions::import_texture_asset() {
    const auto target = require_selected_sprite_renderer("texture");
    if (auto path = dialogs_.open_file(texture_filter, context_.assets_directory())) {
        if (context_.selected_entity() != target) throw std::runtime_error("The selected entity changed during texture import");
        const auto id = apps::asset_import::TextureAssetImporter::import(*path, context_.project_directory(), context_.library_directory());
        target.get_component<vulcal::scene::SpriteRendererComponent>().texture_asset_id_ = id;
        base::log::debug("Editor: assigned texture asset {} to entity '{}' ({})", id.to_string(),
                         target.get_component<vulcal::scene::TagComponent>().name_,
                         target.get_component<vulcal::scene::IDComponent>().id_.to_string());
        gpu_resources_.invalidate(id);
        assets_.invalidate(id);
    }
}

void EditorActions::reset_project_resources() {
    gpu_resources_.clear();
    assets_.set_library(context_.library_directory());
}

} // namespace editor
