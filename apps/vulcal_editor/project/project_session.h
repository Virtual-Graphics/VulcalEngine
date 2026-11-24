#pragma once

#include <filesystem>
#include <string>

namespace vulcal::asset {
class AssetManager;
class GPUResourceRegistry;
}

namespace editor {

class EditorContext;
class ProjectManager;

// Switches the active document and its asset resources together.
class ProjectSession {
public:
    ProjectSession(EditorContext& context, vulcal::asset::AssetManager& assets,
                   vulcal::asset::GPUResourceRegistry* gpu_resources = nullptr);

    void create(const std::filesystem::path& directory, const std::string& name);
    void open(const std::filesystem::path& project_file);

private:
    void activate(ProjectManager&& project);

    EditorContext& context_;
    vulcal::asset::AssetManager& assets_;
    vulcal::asset::GPUResourceRegistry* gpu_resources_;
};

} // namespace editor
