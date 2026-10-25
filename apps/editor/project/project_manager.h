#pragma once
#include "editor/project/project.h"

#include <filesystem>
#include <memory>

namespace vulcal::scene {
class Scene;
}

namespace editor {

// Temporary place for all project related functions. Eventually some of these
// might be moved to other classes.
class ProjectManager {
public:
    ProjectManager();
    ProjectManager(const ProjectManager&) = delete;
    ProjectManager& operator=(const ProjectManager&) = delete;
    ~ProjectManager();

    bool has_project() const;
    void create_project(const std::filesystem::path& directory, const std::string& name);
    void open_project(const std::filesystem::path& project_file);
    void save_project(const std::filesystem::path& project_file);
    void save_project();

    bool has_scene() const;
    std::unique_ptr<vulcal::scene::Scene> create_scene();
    void set_scene(std::unique_ptr<vulcal::scene::Scene> scene);
    void open_scene(const std::filesystem::path& scene_file);
    void save_scene(const std::filesystem::path& scene_file);
    void save_scene();
    void save_as_scene(const std::filesystem::path& scene_file);
    void close_scene();

    std::filesystem::path current_project_file() const;
    std::filesystem::path current_project_dir() const;
    const ProjectInfo& current_project_info() const;
    std::filesystem::path current_scene_file() const;

    vulcal::scene::Scene* current_scene();
    const vulcal::scene::Scene* current_scene() const;

private:
    std::filesystem::path current_project_file_;
    ProjectInfo current_project_info_;
    std::filesystem::path current_scene_file_;
    std::unique_ptr<vulcal::scene::Scene> opened_scene_;
};

} // namespace editor
