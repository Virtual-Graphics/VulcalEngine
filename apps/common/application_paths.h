#pragma once

#include <filesystem>
#include <string_view>

namespace apps::application_paths {

std::filesystem::path executable_directory();
std::filesystem::path find_executable(std::string_view name, const std::filesystem::path& executable_dir, std::string_view search_path);
std::filesystem::path find_executable(std::string_view name);

} // namespace apps::application_paths
