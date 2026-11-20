#include "base/path_utils.h"

#include <system_error>

namespace base {
namespace {

std::filesystem::path normalized(const std::filesystem::path& path) {
    std::error_code error;
    auto result = std::filesystem::weakly_canonical(path, error);
    return error ? path.lexically_normal() : result;
}

} // namespace

std::string path_to_utf8(const std::filesystem::path& path) {
    const auto encoded = path.generic_u8string();
    return { reinterpret_cast<const char*>(encoded.data()), encoded.size() };
}

std::filesystem::path path_from_utf8(std::string_view path) {
    std::u8string encoded;
    encoded.reserve(path.size());
    for (const char character: path) encoded.push_back(static_cast<char8_t>(character));
    return std::filesystem::path{ encoded };
}

std::filesystem::path relative_to(const std::filesystem::path& path, const std::filesystem::path& root) {
    const auto relative = normalized(path).lexically_relative(normalized(root));
    if (relative.empty() || relative.is_absolute() || *relative.begin() == "..") return {};
    return relative;
}

bool is_within(const std::filesystem::path& path, const std::filesystem::path& root) { return !relative_to(path, root).empty(); }

} // namespace base
