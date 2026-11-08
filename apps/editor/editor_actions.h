#pragma once

#include <exception>
#include <filesystem>
#include <functional>
#include <optional>
#include <string>

#include "scene/entity.h"

namespace apps::nfd {
struct FileFilter;
}
namespace vulcal::asset {
class AssetManager;
class RenderAssetCache;
} // namespace vulcal::asset

namespace editor {

class EditorContext;

// User operation workflows shared by menus and shortcuts. Dependencies outlive
// the actions; errors propagate to the UI for presentation.
class EditorActions {
public:
    struct Dialogs {
        std::function<std::optional<std::filesystem::path>(const apps::nfd::FileFilter&, const std::filesystem::path&)> open_file;
        std::function<std::optional<std::filesystem::path>(const apps::nfd::FileFilter&, const std::filesystem::path&,
                                                           const std::filesystem::path&)>
                save_file;
    };

    EditorActions(EditorContext& context, vulcal::asset::AssetManager& assets, vulcal::asset::RenderAssetCache& render_assets);
    EditorActions(EditorContext& context, vulcal::asset::AssetManager& assets, vulcal::asset::RenderAssetCache& render_assets,
                  Dialogs dialogs);

    void create_project(const std::filesystem::path& directory, const std::string& name);
    void open_project();
    void new_scene();
    void open_scene();
    void save_scene();
    void save_scene_as();
    void import_mesh_asset();
    void import_material_asset();
    void import_texture_asset();
    void assign_script();

    template<typename Operation>
    void run_guarded(Operation&& operation) noexcept {
        try {
            operation();
        } catch (const std::exception& error) {
            error_ = error.what();
        } catch (...) {
            error_ = "Unknown editor operation error";
        }
    }

    std::optional<std::string> take_error() {
        auto error = error_;
        error_.reset();
        return error;
    }

private:
    vulcal::scene::Entity require_selected_mesh_renderer(const char* asset_name) const;
    vulcal::scene::Entity require_selected_sprite_renderer(const char* asset_name) const;
    vulcal::scene::Entity require_selected_script() const;
    void reset_project_resources();

    EditorContext& context_;
    vulcal::asset::AssetManager& assets_;
    vulcal::asset::RenderAssetCache& render_assets_;
    Dialogs dialogs_;
    std::optional<std::string> error_;
};

} // namespace editor
