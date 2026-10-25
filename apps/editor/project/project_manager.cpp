#include "editor/project/project_manager.h"

#include "base/logger.h"
#include "editor/project/scene/default_scene.h"
#include "editor/project/scene/scene_serializer.h"
#include "scene/scene.h"

#include <stdexcept>
#include <string>
#include <utility>

namespace editor {

ProjectManager::ProjectManager() = default;

ProjectManager::~ProjectManager() = default;

bool ProjectManager::has_project() const { return !current_project_file_.empty(); }

void ProjectManager::create_project(const std::filesystem::path& directory, const std::string& name) {
    if (name.empty() || name.find_first_of("<>:\"/\\|?*") != std::string::npos) {
        throw std::invalid_argument("Enter a valid project name");
    }
    if (directory.empty() || !std::filesystem::is_directory(directory)) {
        throw std::invalid_argument("Select an existing project directory");
    }

    const auto project_file = directory / (name + ".vproj");
    if (std::filesystem::exists(project_file)) {
        throw std::runtime_error("A project file already exists at " + project_file.string());
    }

    ProjectInfo info;
    info.name = name;
    info.assets_path = "assets";
    std::filesystem::create_directories(directory / info.assets_path);
    if (!save_project_info(project_file, info)) {
        throw std::runtime_error("Failed to create project file at " + project_file.string());
    }

    current_project_file_ = project_file;
    current_project_info_ = std::move(info);
    base::log::info("Created project {} at {}", name, project_file.string());
}

void ProjectManager::open_project(const std::filesystem::path& project_file) {
    if (project_file.extension() != ".vproj") {
        throw std::invalid_argument("Select a .vproj project file");
    }

    ProjectInfo info;
    if (!load_project_info(project_file, info)) {
        base::log::error("Failed to load project info from {}", project_file.string());
        throw std::runtime_error("Failed to load project info from " + project_file.string());
    }
    current_project_file_ = project_file;
    current_project_info_ = std::move(info);
    base::log::info("Loaded project info from {}", project_file.string());
}

void ProjectManager::save_project(const std::filesystem::path& project_file) {
    if (!save_project_info(project_file, current_project_info_)) {
        base::log::error("Failed to save project info to {}", project_file.string());
        throw std::runtime_error("Failed to save project info to " + project_file.string());
    }
    current_project_file_ = project_file;
    base::log::info("Saved project info to {}", project_file.string());
}

void ProjectManager::save_project() {
    if (!has_project()) {
        base::log::error("No project is currently open");
        throw std::runtime_error("No project is currently open");
    }
    save_project(current_project_file_);
}

bool ProjectManager::has_scene() const { return opened_scene_ != nullptr; }

std::unique_ptr<vulcal::scene::Scene> ProjectManager::create_scene() { return create_default_scene(); }

void ProjectManager::set_scene(std::unique_ptr<vulcal::scene::Scene> scene) {
    if (!scene) {
        base::log::error("Cannot set an empty scene");
        throw std::invalid_argument("Cannot set an empty scene");
    }
    opened_scene_ = std::move(scene);
    current_scene_file_.clear();
    base::log::info("Set the current scene");
}

void ProjectManager::open_scene(const std::filesystem::path& scene_file) {
    std::unique_ptr<vulcal::scene::Scene> scene;
    try {
        scene = SceneSerializer::deserialize(scene_file);
    } catch (const std::exception& error) {
        base::log::error("Failed to deserialize scene file from {}: {}", scene_file.string(), error.what());
        throw std::runtime_error("Failed to deserialize scene file from " + scene_file.string() + ": " + error.what());
    }
    if (!scene) {
        base::log::error("Failed to deserialize scene file from {}", scene_file.string());
        throw std::runtime_error("Failed to deserialize scene file from " + scene_file.string());
    }
    opened_scene_ = std::move(scene);
    current_scene_file_ = scene_file;
    base::log::info("Loaded scene from {}", scene_file.string());
}

void ProjectManager::save_scene(const std::filesystem::path& scene_file) {
    if (!opened_scene_) {
        base::log::error("No scene is currently open");
        throw std::runtime_error("No scene is currently open");
    }
    try {
        SceneSerializer::serialize(*opened_scene_, scene_file);
    } catch (const std::exception& error) {
        base::log::error("Failed to serialize scene to {}: {}", scene_file.string(), error.what());
        throw std::runtime_error("Failed to serialize scene to " + scene_file.string() + ": " + error.what());
    }
    current_scene_file_ = scene_file;
    base::log::info("Saved scene to {}", scene_file.string());
}

void ProjectManager::save_scene() {
    if (current_scene_file_.empty()) {
        base::log::error("No scene file is associated with the current scene");
        throw std::runtime_error("No scene file is associated with the current scene");
    }
    save_scene(current_scene_file_);
}

void ProjectManager::save_as_scene(const std::filesystem::path& scene_file) { save_scene(scene_file); }

void ProjectManager::close_scene() {
    if (opened_scene_) {
        base::log::info("Closed scene {}", current_scene_file_.string());
    }
    opened_scene_.reset();
    current_scene_file_.clear();
}

std::filesystem::path ProjectManager::current_project_file() const { return current_project_file_; }

std::filesystem::path ProjectManager::current_project_dir() const {
    return has_project() ? current_project_file_.parent_path() : std::filesystem::path{};
}

const ProjectInfo& ProjectManager::current_project_info() const { return current_project_info_; }

std::filesystem::path ProjectManager::current_scene_file() const { return current_scene_file_; }

vulcal::scene::Scene* ProjectManager::current_scene() {
    if (!opened_scene_) {
        base::log::error("No scene is currently open");
        return nullptr;
    }
    return opened_scene_.get();
}

const vulcal::scene::Scene* ProjectManager::current_scene() const { return opened_scene_.get(); }

} // namespace editor
