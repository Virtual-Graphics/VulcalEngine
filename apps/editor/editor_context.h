#pragma once

#include "editor/project/project_manager.h"
#include "scene/scene.h"

#include <filesystem>
#include <memory>
#include <string>

namespace editor {

// Shared document and selection state. The context owns the project and scene
// so panels never keep references to an old scene.
class EditorContext {
public:
    enum class SaveResult {
        Saved,
        NeedsPath,
    };

    bool has_scene() const { return project_manager_.has_scene(); }

    bool playing() const { return runtime_scene_ != nullptr; }
    bool uses_primary_camera() const;
    // Switch the active scene and rebind the current selection by UUID.
    void begin_play();
    // The owner must stop scripts before releasing their referenced scene.
    void end_play();
    void require_edit_mode() const;

    vulcal::scene::Scene& scene();
    const vulcal::scene::Scene& scene() const;

    void set_scene(std::unique_ptr<vulcal::scene::Scene> scene);
    void new_scene();
    void open_scene(const std::filesystem::path& scene_file);
    SaveResult save_scene();
    void save_scene_as(std::filesystem::path scene_file);

    void create_project(const std::filesystem::path& directory, const std::string& name);
    void open_project(const std::filesystem::path& project_file);
    bool has_project() const { return project_manager_.has_project(); }
    std::filesystem::path project_directory() const;
    std::filesystem::path assets_directory() const;
    std::filesystem::path library_directory() const;

    std::filesystem::path scene_dialog_directory() const;
    std::filesystem::path suggested_scene_name() const;

    vulcal::scene::Entity selected_entity() const;
    void select(vulcal::scene::Entity entity);
    vulcal::scene::Entity create_entity();
    void destroy_entity(vulcal::scene::Entity entity);

private:
    ProjectManager project_manager_;
    std::unique_ptr<vulcal::scene::Scene> runtime_scene_;
    vulcal::scene::Entity selected_;
};

} // namespace editor
