#include "base/string_utils.h"

#include <algorithm>
#include <cctype>

namespace base {

std::string to_lower(std::string_view value) {
    std::string result{ value };
    std::transform(result.begin(), result.end(), result.begin(),
                   [](unsigned char character) { return static_cast<char>(std::tolower(character)); });
    return result;
}

std::string lowercase_extension(const std::filesystem::path& path) { return to_lower(path.extension().string()); }

bool has_extension(const std::filesystem::path& path, std::string_view extension) {
    if (extension.empty()) return false;

    auto expected = to_lower(extension);
    if (expected.front() != '.') expected.insert(expected.begin(), '.');
    return lowercase_extension(path) == expected;
}

} // namespace base
