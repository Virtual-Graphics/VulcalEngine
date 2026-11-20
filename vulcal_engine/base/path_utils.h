#pragma once

#include <filesystem>
#include <string>
#include <string_view>

namespace base {

std::string path_to_utf8(const std::filesystem::path& path);
std::filesystem::path path_from_utf8(std::string_view path);

std::filesystem::path relative_to(const std::filesystem::path& path, const std::filesystem::path& root);
bool is_within(const std::filesystem::path& path, const std::filesystem::path& root);

} // namespace base
