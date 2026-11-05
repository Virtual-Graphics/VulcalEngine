#include "editor/editor_actions.h"

#include "asset/asset_manager.h"
#include "asset/render_asset_cache.h"
#include "asset_import/material_asset_importer.h"
#include "asset_import/obj_asset_importer.h"
#include "base/logger.h"
#include "common/nfd/file_dialog.h"
#include "editor/editor_context.h"

#include <stdexcept>
#include <utility>

namespace editor {
namespace {

const apps::nfd::FileFilter project_filter{ "Vulcal Project", "vproj" };
const apps::nfd::FileFilter scene_filter{ "Vulcal Scene", "vscene" };
const apps::nfd::FileFilter obj_filter{ "Wavefront OBJ", "obj" };
const apps::nfd::FileFilter material_filter{ "Vulcal Material", "vmat" };

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
    if (auto path = dialogs_.open_file(project_filter, {})) {
        context_.open_project(*path);
        reset_project_resources();
    }
}

void EditorActions::new_scene() { context_.new_scene(); }

void EditorActions::open_scene() {
    if (auto path = dialogs_.open_file(scene_filter, context_.scene_dialog_directory())) context_.open_scene(*path);
}

void EditorActions::save_scene() {
    if (context_.save_scene() == EditorContext::SaveResult::NeedsPath) save_scene_as();
}

void EditorActions::save_scene_as() {
    if (auto path = dialogs_.save_file(scene_filter, context_.scene_dialog_directory(), context_.suggested_scene_name())) {
        context_.save_scene_as(*path);
    }
}

void EditorActions::import_obj() {
    if (!context_.has_project()) throw std::logic_error("Open a project before importing assets");
    if (auto path = dialogs_.open_file(obj_filter, context_.assets_directory())) {
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
        render_assets_.invalidate(id);
        assets_.invalidate(id);
    }
}

void EditorActions::import_material() {
    if (!context_.has_project()) {
        throw std::logic_error("Open a project before importing assets");
    }

    if (auto path = dialogs_.open_file(material_filter, context_.assets_directory())) {
        const auto id = apps::asset_import::MaterialAssetImporter::import(*path, context_.assets_directory(), context_.library_directory());
        const auto selected = context_.selected_entity();

        // clang-format off
        if (selected) {
            vulcal::scene::MeshRendererComponent& component = selected.has_component<vulcal::scene::MeshRendererComponent>()
                                                            ? selected.get_component<vulcal::scene::MeshRendererComponent>()
                                                            : selected.add_component<vulcal::scene::MeshRendererComponent>();
            component.material_asset_id_ = id;

            base::log::debug("Editor: assigned material asset {} to entity '{}' ({})",
                             id.to_string(),
                             selected.get_component<vulcal::scene::TagComponent>().name_,
                             selected.get_component<vulcal::scene::IDComponent>().id_.to_string());
        }
        // clang-format on

        render_assets_.invalidate(id);
        assets_.invalidate(id);
    }
}

void EditorActions::reset_project_resources() {
    render_assets_.clear();
    assets_.set_directory(context_.library_directory());
}

} // namespace editor
