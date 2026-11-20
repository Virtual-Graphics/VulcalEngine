#pragma once

#include "base/path_utils.h"
#include "base/string_utils.h"
#include "resource/resource_provider.h"

#include <filesystem>
#include <stdexcept>
#include <string>
#include <string_view>

namespace apps::project_paths {

// Validates a path that must be relative to a project root. Rejects absolute
// paths (including Windows roots), empty/"."/".." components, and logical
// paths that are not valid UTF-8 with '/' separators.
inline std::filesystem::path validate_relative_path(const std::filesystem::path& path, std::string_view field) {
    if (path.empty() || path.is_absolute() || path.has_root_name() || path.has_root_directory()) {
        throw std::invalid_argument(std::string(field) + " must be a project-relative path");
    }
    for (const auto& component: path) {
        if (component.empty() || component == "." || component == "..") {
            throw std::invalid_argument(std::string(field) + " contains an unsafe path component");
        }
    }
    try {
        vulcal::resource::detail::normalize_logical_path(base::path_to_utf8(path));
    } catch (const std::invalid_argument&) {
        throw std::invalid_argument(std::string(field) + " must use a UTF-8 relative path with '/' separators");
    }
    return path;
}

// Validates a scene path that must be a .vscene file inside the assets
// directory. Both paths are relative to the same project root; pass "." as the
// assets directory when the root is a package.
inline std::filesystem::path validate_scene_path(const std::filesystem::path& scene, const std::filesystem::path& assets_path,
                                                 std::string_view field) {
    const auto relative = validate_relative_path(scene, field);
    if (!base::has_extension(relative, ".vscene")) {
        throw std::invalid_argument(std::string(field) + " must reference a .vscene file");
    }
    // "." means the package root; the path is already root-relative.
    if (assets_path != "." && !base::is_within(relative, assets_path)) {
        throw std::invalid_argument(std::string(field) + " must be inside the assets directory");
    }
    return relative;
}

} // namespace apps::project_paths
