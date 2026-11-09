#pragma once

#include <filesystem>

namespace base {

std::filesystem::path relative_to(const std::filesystem::path& path, const std::filesystem::path& root);
bool is_within(const std::filesystem::path& path, const std::filesystem::path& root);

} // namespace base
