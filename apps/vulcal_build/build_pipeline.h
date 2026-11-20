#pragma once

#include <filesystem>

namespace apps::vulcal_build {

struct BuildResult {
    std::filesystem::path pak_file;
    std::filesystem::path player_file;
};

BuildResult build_project(const std::filesystem::path& project_file, const std::filesystem::path& player_executable);

} // namespace apps::vulcal_build
