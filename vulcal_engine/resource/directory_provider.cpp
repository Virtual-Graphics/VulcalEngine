#include "resource/directory_provider.h"

#include "base/path_utils.h"

#include <fstream>
#include <limits>
#include <stdexcept>
#include <system_error>

namespace vulcal::resource {

DirectoryProvider::DirectoryProvider(std::filesystem::path root) {
    std::error_code error;
    root_ = std::filesystem::canonical(root, error);
    if (error || !std::filesystem::is_directory(root_)) {
        throw std::invalid_argument("Resource directory does not exist: " + root.string());
    }
}

// 把逻辑路径解析成磁盘真实的绝对路径
std::filesystem::path DirectoryProvider::resolve(std::string_view logical_path) const {
    const auto normalized = detail::normalize_logical_path(logical_path);
    std::u8string utf8_path;
    utf8_path.reserve(normalized.size());
    for (const char character: normalized) { 
        utf8_path.push_back(static_cast<char8_t>(character));
    }

    const auto requested = std::filesystem::path(utf8_path);
    std::error_code error;
    auto candidate = root_;
    // Native lookups can ignore case, so compare each stored name before following the component.
    for (const auto& component: requested) {
        bool found = false;
        for (std::filesystem::directory_iterator iterator(candidate, error), end; !error && iterator != end; iterator.increment(error)) {
            if (iterator->path().filename().native() == component.native()) {
                found = true;
                candidate = iterator->path();
                break;
            }
        }
        if (error || !found) {
            throw std::runtime_error("Resource file does not exist or has different path case: " + normalized);
        }
    }
    const auto resolved = std::filesystem::canonical(candidate, error);
    if (error || !std::filesystem::is_regular_file(resolved) || !base::is_within(resolved, root_)) {
        throw std::runtime_error("Resource file does not exist or resolves outside its root: " + normalized);
    }
    return resolved;
}

bool DirectoryProvider::contains(std::string_view path) const {
    try {
        static_cast<void>(resolve(path));
        return true;
    } catch (...) {
        return false;
    }
}

std::vector<std::byte> DirectoryProvider::read_binary(std::string_view path) const {
    const auto resolved = resolve(path);
    std::error_code error;
    const auto size = std::filesystem::file_size(resolved, error);
    if (error || 
        size > std::numeric_limits<std::size_t>::max() ||
        size > static_cast<std::uintmax_t>(std::numeric_limits<std::streamsize>::max())) 
    {
        throw std::runtime_error("Resource file is too large to read: " + std::string(path));
    }
    std::ifstream input(resolved, std::ios::binary);
    if (!input) { 
        throw std::runtime_error("Could not open resource file: " + std::string(path));
    }

    std::vector<std::byte> bytes(static_cast<std::size_t>(size));
    if (!bytes.empty()) {
        input.read(reinterpret_cast<char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
        if (!input) {
            throw std::runtime_error("Could not read resource file: " + std::string(path));
        }
    }
    return bytes;
}

std::string DirectoryProvider::read_text(std::string_view path) const {
    const auto bytes = read_binary(path);
    if (bytes.empty()) { 
        return {};
    }
    return { reinterpret_cast<const char*>(bytes.data()), bytes.size() };
}

} // namespace vulcal::resource
