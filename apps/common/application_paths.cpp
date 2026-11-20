#include "common/application_paths.h"
#include "base/path_utils.h"

#include <SDL3/SDL_filesystem.h>
#include <SDL3/SDL_stdinc.h>

#include <system_error>
#include <vector>

namespace apps::application_paths {
namespace {

std::vector<std::filesystem::path> executable_names(std::string_view name) {
    const auto path = base::path_from_utf8(name);
    if (path.has_extension()) return { path };
#ifdef _WIN32
    return { std::filesystem::path{ std::string(name) + ".exe" }, path };
#else
    return { path };
#endif
}

std::filesystem::path find_in_directory(const std::filesystem::path& directory, const std::vector<std::filesystem::path>& names) {
    if (directory.empty()) return {};
    for (const auto& name: names) {
        const auto candidate = directory / name;
        std::error_code error;
        if (std::filesystem::is_regular_file(candidate, error) && !error) return candidate.lexically_normal();
    }
    return {};
}

} // namespace

std::filesystem::path executable_directory() {
    const auto* base_path = SDL_GetBasePath();
    return base_path ? base::path_from_utf8(base_path) : std::filesystem::path{};
}

std::filesystem::path find_executable(std::string_view name, const std::filesystem::path& executable_dir, std::string_view search_path) {
    const auto names = executable_names(name);
    if (const auto beside_app = find_in_directory(executable_dir, names); !beside_app.empty()) return beside_app;

#ifdef _WIN32
    constexpr char separator = ';';
#else
    constexpr char separator = ':';
#endif
    std::size_t begin = 0;
    while (begin <= search_path.size()) {
        const auto end   = search_path.find(separator, begin);
        const auto entry = search_path.substr(begin, end == std::string_view::npos ? search_path.size() - begin : end - begin);
        if (!entry.empty()) {
            if (const auto candidate = find_in_directory(base::path_from_utf8(entry), names); !candidate.empty()) return candidate;
        }
        if (end == std::string_view::npos) break;
        begin = end + 1;
    }
    return {};
}

std::filesystem::path find_executable(std::string_view name) {
    auto* environment = SDL_GetEnvironment();
    const auto* path  = environment ? SDL_GetEnvironmentVariable(environment, "PATH") : nullptr;
    return find_executable(name, executable_directory(), path ? std::string_view{ path } : std::string_view{});
}

} // namespace apps::application_paths
