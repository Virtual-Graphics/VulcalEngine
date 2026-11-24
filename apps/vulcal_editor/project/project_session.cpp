#include "vulcal_editor/project/project_session.h"

#include "asset/asset_manager.h"
#include "asset/gpu_resource_registry.h"
#include "base/logger.h"
#include "vulcal_editor/editor_context.h"

#include <exception>
#include <utility>

namespace editor {

ProjectSession::ProjectSession(EditorContext& context, vulcal::asset::AssetManager& assets,
                               vulcal::asset::GPUResourceRegistry* gpu_resources)
        : context_(context), assets_(assets), gpu_resources_(gpu_resources) {}

void ProjectSession::create(const std::filesystem::path& directory, const std::string& name) {
    context_.require_edit_mode();
    ProjectManager project;
    project.set_scene(project.create_scene());
    project.create_project(directory, name);
    activate(std::move(project));
}

void ProjectSession::open(const std::filesystem::path& project_file) {
    context_.require_edit_mode();
    ProjectManager project;
    project.open_project(project_file);
    const auto last_scene = project.resolved_last_edited_scene();
    if (!last_scene.empty()) {
        std::error_code error;
        if (std::filesystem::is_regular_file(last_scene, error)) {
            try {
                project.open_scene(last_scene);
            } catch (const std::exception& open_error) {
                base::log::warn("Failed to open last edited scene '{}': {}", last_scene.string(), open_error.what());
            }
        } else {
            base::log::warn("Last edited scene '{}' is missing; starting a new scene", last_scene.string());
        }
    }
    if (!project.has_scene()) project.set_scene(project.create_scene());
    activate(std::move(project));
}

void ProjectSession::activate(ProjectManager&& project) {
    // Resolving the new library can fail. Keep the current scene and caches
    // until the replacement project and library have been prepared.
    assets_.set_library(project.current_library_dir());
    if (gpu_resources_) gpu_resources_->clear();
    context_.activate_project(std::move(project));
}

} // namespace editor
