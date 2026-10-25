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

    vulcal::scene::Scene& scene();
    const vulcal::scene::Scene& scene() const;

    void set_scene(std::unique_ptr<vulcal::scene::Scene> scene);
    void new_scene();
    void open_scene(const std::filesystem::path& scene_file);
    SaveResult save_scene();
    void save_scene_as(std::filesystem::path scene_file);

    void create_project(const std::filesystem::path& directory, const std::string& name);
    void open_project(const std::filesystem::path& project_file);

    std::filesystem::path scene_dialog_directory() const;
    std::filesystem::path suggested_scene_name() const;

    vulcal::scene::Entity selected_entity() const;
    void select(vulcal::scene::Entity entity);
    vulcal::scene::Entity create_entity();
    void destroy_entity(vulcal::scene::Entity entity);

private:
    ProjectManager project_manager_;
    vulcal::scene::Entity selected_;
};

} // namespace editor
