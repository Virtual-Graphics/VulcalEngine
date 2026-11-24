#pragma once

#include "vulcal_editor/project/project_manager.h"
#include "scene/scene.h"

#include <cstdint>
#include <filesystem>
#include <memory>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>

namespace editor {

class ProjectSession;

// Shared document and selection state. The context owns the project and scene
// so panels never keep references to an old scene.
class EditorContext {
public:
    EditorContext() = default;
    EditorContext(const EditorContext&) = delete;
    EditorContext& operator=(const EditorContext&) = delete;

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

    // Editor mutations should go through the entity and component operations.
    vulcal::scene::Scene& scene();
    const vulcal::scene::Scene& scene() const;

    void set_scene(std::unique_ptr<vulcal::scene::Scene> scene);
    void new_scene();
    void open_scene(const std::filesystem::path& scene_file);
    SaveResult save_scene();
    void save_scene_as(std::filesystem::path scene_file);

    bool has_project() const { return project_manager_.has_project(); }
    std::filesystem::path project_directory() const;
    std::filesystem::path project_file() const;
    std::filesystem::path assets_directory() const;
    std::filesystem::path library_directory() const;
    std::filesystem::path build_directory() const;
    const ProjectInfo& project_info() const;
    void set_build_info(BuildInfo build_info);

    std::filesystem::path scene_dialog_directory() const;
    std::filesystem::path suggested_scene_name() const;

    vulcal::scene::Entity selected_entity() const;
    void select(vulcal::scene::Entity entity);
    vulcal::scene::Entity create_entity();
    void destroy_entity(vulcal::scene::Entity entity);

    // Advances for document replacement and successful edits, including edits
    // across frames of an active widget. Selection and Play do not change it.
    std::uint64_t scene_revision() const { return scene_revision_; }

    template<typename Component>
    void set_component(vulcal::scene::Entity entity, Component value) {
        static_assert(!std::is_same_v<Component, vulcal::scene::IDComponent>);
        require_edit_entity(entity);
        auto& current = scene().get_component<Component>(entity);
        if (current == value) return;
        current = std::move(value);
        mark_scene_changed();
    }

    template<typename Component>
    void add_component(vulcal::scene::Entity entity) {
        require_edit_entity(entity);
        if constexpr (std::is_same_v<Component, vulcal::scene::LightComponent> ||
                      std::is_same_v<Component, vulcal::scene::EnvironmentComponent>) {
            if (!scene().registry().view<Component>().empty()) throw std::logic_error("Only one of this component is supported per scene");
        }
        scene().add_component<Component>(entity);
        mark_scene_changed();
    }

    template<typename Component>
    void remove_component(vulcal::scene::Entity entity) {
        require_edit_entity(entity);
        if (!scene().has_component<Component>(entity)) return;
        scene().remove_component<Component>(entity);
        mark_scene_changed();
    }

private:
    friend class ProjectSession;
    void activate_project(ProjectManager&& project) noexcept;
    void require_edit_entity(vulcal::scene::Entity entity) const;
    void mark_scene_changed() noexcept { ++scene_revision_; }

    ProjectManager project_manager_;
    std::unique_ptr<vulcal::scene::Scene> runtime_scene_;
    vulcal::scene::Entity selected_;
    std::uint64_t scene_revision_ = 0;
};

} // namespace editor
