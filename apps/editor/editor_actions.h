#pragma once

#include <filesystem>
#include <functional>
#include <optional>
#include <string>

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
    void import_obj();

private:
    void reset_project_resources();

    EditorContext& context_;
    vulcal::asset::AssetManager& assets_;
    vulcal::asset::RenderAssetCache& render_assets_;
    Dialogs dialogs_;
};

} // namespace editor
