#include "editor/editor_context.h"

#include "base/logger.h"

#include <stdexcept>
#include <utility>

namespace editor {

vulcal::scene::Scene& EditorContext::scene() {
    auto* scene = project_manager_.current_scene();
    if (!scene) throw std::logic_error("No scene is currently open");
    return *scene;
}

const vulcal::scene::Scene& EditorContext::scene() const {
    const auto* scene = project_manager_.current_scene();
    if (!scene) throw std::logic_error("No scene is currently open");
    return *scene;
}

void EditorContext::set_scene(std::unique_ptr<vulcal::scene::Scene> scene) {
    project_manager_.set_scene(std::move(scene));
    selected_ = {};
}

void EditorContext::new_scene() { set_scene(project_manager_.create_scene()); }

void EditorContext::open_scene(const std::filesystem::path& scene_file) {
    project_manager_.open_scene(scene_file);
    selected_ = {};
}

EditorContext::SaveResult EditorContext::save_scene() {
    if (!project_manager_.has_scene()) throw std::logic_error("No scene is currently open");
    if (project_manager_.current_scene_file().empty()) return SaveResult::NeedsPath;
    project_manager_.save_scene();
    return SaveResult::Saved;
}

void EditorContext::save_scene_as(std::filesystem::path scene_file) {
    if (scene_file.extension() != ".vscene") scene_file.replace_extension(".vscene");
    project_manager_.save_as_scene(scene_file);
}

void EditorContext::create_project(const std::filesystem::path& directory, const std::string& name) {
    project_manager_.create_project(directory, name);
    new_scene();
}

void EditorContext::open_project(const std::filesystem::path& project_file) {
    project_manager_.open_project(project_file);
    new_scene();
}

std::filesystem::path EditorContext::scene_dialog_directory() const {
    auto directory = project_manager_.current_scene_file().parent_path();
    if (directory.empty()) directory = project_manager_.current_project_dir();
    if (directory.empty()) directory = std::filesystem::current_path();
    return directory;
}

std::filesystem::path EditorContext::suggested_scene_name() const {
    const auto scene_file = project_manager_.current_scene_file();
    return scene_file.empty() ? std::filesystem::path{ "Untitled.vscene" } : scene_file.filename();
}

vulcal::scene::Entity EditorContext::selected_entity() const {
    const auto* scene = project_manager_.current_scene();
    return scene && scene->contains(selected_) ? selected_ : vulcal::scene::Entity{};
}

void EditorContext::select(vulcal::scene::Entity entity) {
    if (entity) {
        const auto* scene = project_manager_.current_scene();
        if (!scene || !scene->contains(entity)) throw std::invalid_argument("Entity belongs to a different scene");
    }
    selected_ = entity ? entity : vulcal::scene::Entity{};
}

vulcal::scene::Entity EditorContext::create_entity() {
    auto entity = scene().create_entity();
    select(entity);
    return entity;
}

void EditorContext::destroy_entity(vulcal::scene::Entity entity) {
    if (!entity) return;
    if (!scene().contains(entity)) {
        base::log::error("Cannot destroy an entity that belongs to a different scene");
        throw std::invalid_argument("Entity belongs to a different scene");
    }
    if (selected_ == entity) selected_ = {};
    scene().destroy_entity(entity);
}

} // namespace editor
