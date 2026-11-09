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

std::filesystem::path relative_to(const std::filesystem::path& path, const std::filesystem::path& root) {
    const auto relative = normalized(path).lexically_relative(normalized(root));
    if (relative.empty() || relative.is_absolute() || *relative.begin() == "..") return {};
    return relative;
}

bool is_within(const std::filesystem::path& path, const std::filesystem::path& root) { return !relative_to(path, root).empty(); }

} // namespace base
