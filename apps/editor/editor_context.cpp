#include "editor/editor_context.h"

#include "base/logger.h"

#include <stdexcept>
#include <utility>

namespace editor {
namespace {

base::UUID selection_id(vulcal::scene::Entity entity) {
    return entity ? entity.get_component<vulcal::scene::IDComponent>().id_ : base::UUID{};
}

vulcal::scene::Entity find_entity(vulcal::scene::Scene& scene, base::UUID id) {
    vulcal::scene::Entity found;
    if (!id.is_nil()) scene.each<vulcal::scene::IDComponent>([&](auto entity, const auto& component) {
        if (component.id_ == id) found = entity;
    });
    return found;
}

} // namespace

void EditorContext::require_edit_mode() const {
    if (playing()) throw std::logic_error("Stop playing before editing or replacing the scene or project");
}

void EditorContext::begin_play() {
    if (playing()) return;
    const auto id = selection_id(selected_entity());
    auto copy = std::make_unique<vulcal::scene::Scene>(scene());
    selected_ = find_entity(*copy, id);
    runtime_scene_ = std::move(copy);
}

void EditorContext::end_play() {
    if (!playing()) return;
    const auto id = selection_id(selected_entity());
    selected_ = find_entity(*project_manager_.current_scene(), id);
    runtime_scene_.reset();
}

vulcal::scene::Scene& EditorContext::scene() {
    auto* scene = playing() ? runtime_scene_.get() : project_manager_.current_scene();
    if (!scene) throw std::logic_error("No scene is currently open");
    return *scene;
}

const vulcal::scene::Scene& EditorContext::scene() const {
    const auto* scene = playing() ? runtime_scene_.get() : project_manager_.current_scene();
    if (!scene) throw std::logic_error("No scene is currently open");
    return *scene;
}

void EditorContext::set_scene(std::unique_ptr<vulcal::scene::Scene> scene) {
    require_edit_mode();
    project_manager_.set_scene(std::move(scene));
    selected_ = {};
}

void EditorContext::new_scene() {
    require_edit_mode();
    set_scene(project_manager_.create_scene());
    base::log::debug("EditorContext: created a new scene and cleared selection");
}

void EditorContext::open_scene(const std::filesystem::path& scene_file) {
    require_edit_mode();
    project_manager_.open_scene(scene_file);
    selected_ = {};
}

EditorContext::SaveResult EditorContext::save_scene() {
    require_edit_mode();
    if (!project_manager_.has_scene()) throw std::logic_error("No scene is currently open");
    if (project_manager_.current_scene_file().empty()) {
        base::log::debug("EditorContext: scene has no file path; Save As is required");
        return SaveResult::NeedsPath;
    }
    project_manager_.save_scene();
    return SaveResult::Saved;
}

void EditorContext::save_scene_as(std::filesystem::path scene_file) {
    require_edit_mode();
    if (scene_file.extension() != ".vscene") scene_file.replace_extension(".vscene");
    project_manager_.save_as_scene(scene_file);
}

void EditorContext::create_project(const std::filesystem::path& directory, const std::string& name) {
    require_edit_mode();
    project_manager_.create_project(directory, name);
    new_scene();
}

void EditorContext::open_project(const std::filesystem::path& project_file) {
    require_edit_mode();
    project_manager_.open_project(project_file);
    new_scene();
}

std::filesystem::path EditorContext::project_directory() const { return project_manager_.current_project_dir(); }

std::filesystem::path EditorContext::assets_directory() const { return project_manager_.current_assets_dir(); }

std::filesystem::path EditorContext::library_directory() const { return project_manager_.current_library_dir(); }

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
    const auto* scene = playing() ? runtime_scene_.get() : project_manager_.current_scene();
    return scene && scene->contains(selected_) ? selected_ : vulcal::scene::Entity{};
}

void EditorContext::select(vulcal::scene::Entity entity) {
    if (entity) {
        const auto* scene = playing() ? runtime_scene_.get() : project_manager_.current_scene();
        if (!scene || !scene->contains(entity)) throw std::invalid_argument("Entity belongs to a different scene");
    }
    const auto selection = entity ? entity : vulcal::scene::Entity{};
    if (selection != selected_) {
        if (selection) {
            base::log::trace("EditorContext: selected entity '{}' ({})", selection.get_component<vulcal::scene::TagComponent>().name_,
                             selection.get_component<vulcal::scene::IDComponent>().id_.to_string());
        } else {
            base::log::trace("EditorContext: cleared selection");
        }
    }
    selected_ = selection;
}

vulcal::scene::Entity EditorContext::create_entity() {
    require_edit_mode();
    auto entity = scene().create_entity();
    select(entity);
    return entity;
}

void EditorContext::destroy_entity(vulcal::scene::Entity entity) {
    require_edit_mode();
    if (!entity) return;
    if (!scene().contains(entity)) {
        base::log::error("Cannot destroy an entity that belongs to a different scene");
        throw std::invalid_argument("Entity belongs to a different scene");
    }
    if (selected_ == entity) selected_ = {};
    scene().destroy_entity(entity);
}

} // namespace editor
