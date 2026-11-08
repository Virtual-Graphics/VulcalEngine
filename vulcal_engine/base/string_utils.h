#pragma once

#include <filesystem>
#include <string>
#include <string_view>

namespace base {

// ASCII-only lowercase conversion.
std::string to_lower(std::string_view value);

// Returns the lowercased extension including the leading dot, or an empty string.
std::string lowercase_extension(const std::filesystem::path& path);

// Case-insensitive extension comparison. The leading dot is optional.
bool has_extension(const std::filesystem::path& path, std::string_view extension);

} // namespace base
