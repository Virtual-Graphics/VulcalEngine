#include "editor/editor_actions.h"

#include "asset/asset_manager.h"
#include "asset/render_asset_cache.h"
#include "asset_import/asset_importer.h"
#include "base/logger.h"
#include "common/nfd/file_dialog.h"
#include "editor/editor_context.h"
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
const apps::nfd::FileFilter script_filter{ "Lua Script", "lua" };

} // namespace

EditorActions::EditorActions(EditorContext& context, vulcal::asset::AssetManager& assets, vulcal::asset::RenderAssetCache& render_assets)
        : EditorActions(context, assets, render_assets,
                        Dialogs{
                            .open_file = [](const auto& filter, const auto& directory) { return apps::nfd::open_file(filter, directory); },
                            .save_file = [](const auto& filter, const auto& directory,
                                            const auto& name) { return apps::nfd::save_file(filter, directory, name); },
                        }) {}

EditorActions::EditorActions(EditorContext& context, vulcal::asset::AssetManager& assets, vulcal::asset::RenderAssetCache& render_assets,
                             Dialogs dialogs)
        : context_(context),
          assets_(assets),
          render_assets_(render_assets),
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
        if (error || selected.extension() != ".lua" || !std::filesystem::is_regular_file(selected)) {
            throw std::invalid_argument("Select an existing .lua script inside the Assets directory");
        }
        const auto assets = std::filesystem::canonical(context_.assets_directory(), error);
        if (error) throw std::runtime_error("The project Assets directory is unavailable");
        auto asset_part    = assets.begin();
        auto selected_part = selected.begin();
        for (; asset_part != assets.end() && selected_part != selected.end(); ++asset_part, ++selected_part) {
            if (*asset_part != *selected_part) break;
        }
        if (asset_part != assets.end()) throw std::invalid_argument("Select a script inside the Assets directory");
        const auto project = std::filesystem::canonical(context_.project_directory(), error);
        if (error) throw std::runtime_error("The project directory is unavailable");
        const auto relative = std::filesystem::relative(selected, project, error);
        if (error || relative.empty() || relative.is_absolute() || *relative.begin() == "..") {
            throw std::invalid_argument("The selected script is outside the project directory");
        }
        target.get_component<vulcal::scene::ScriptComponent>().script_path_ = relative.generic_string();
    }
}

void EditorActions::import_mesh_asset() {
    const auto target = require_selected_mesh_renderer("mesh");
    if (auto path = dialogs_.open_file(obj_filter, context_.assets_directory())) {
        if (context_.selected_entity() != target) throw std::runtime_error("The selected entity changed during mesh import");
        const auto id = apps::asset_import::AssetImporter::import(*path, context_.assets_directory(), context_.library_directory());
        target.get_component<vulcal::scene::MeshRendererComponent>().mesh_asset_id_ = id;
        base::log::debug("Editor: assigned mesh asset {} to entity '{}' ({})", id.to_string(),
                         target.get_component<vulcal::scene::TagComponent>().name_,
                         target.get_component<vulcal::scene::IDComponent>().id_.to_string());
        render_assets_.invalidate(id);
        assets_.invalidate(id);
    }
}

void EditorActions::import_material_asset() {
    const auto target = require_selected_mesh_renderer("material");
    if (auto path = dialogs_.open_file(material_filter, context_.assets_directory())) {
        if (context_.selected_entity() != target) throw std::runtime_error("The selected entity changed during material import");
        const auto id = apps::asset_import::AssetImporter::import(*path, context_.assets_directory(), context_.library_directory());
        target.get_component<vulcal::scene::MeshRendererComponent>().material_asset_id_ = id;
        base::log::debug("Editor: assigned material asset {} to entity '{}' ({})", id.to_string(),
                         target.get_component<vulcal::scene::TagComponent>().name_,
                         target.get_component<vulcal::scene::IDComponent>().id_.to_string());
        render_assets_.invalidate(id);
        assets_.invalidate(id);
    }
}

void EditorActions::reset_project_resources() {
    render_assets_.clear();
    assets_.set_directory(context_.library_directory());
}

} // namespace editor
