#include "common/nfd/file_dialog.h"

#include <nfd.h>

#include <memory>
#include <stdexcept>

namespace apps::nfd {
namespace {

[[noreturn]] void fail(const char* fallback) {
    const char* error = NFD_GetError();
    throw std::runtime_error(error && *error ? error : fallback);
}

struct NfdGuard {
    NfdGuard() {
        if (NFD_Init() != NFD_OKAY) fail("Failed to initialize native file dialog");
    }
    ~NfdGuard() { NFD_Quit(); }
};

void ensure_initialized() { [[maybe_unused]] static NfdGuard guard; }

std::string path_to_utf8(const std::filesystem::path& path) {
    const auto value = path.u8string();
    return std::string(reinterpret_cast<const char*>(value.data()), value.size());
}

std::filesystem::path path_from_utf8(const char* text) {
    return std::filesystem::path(std::u8string(reinterpret_cast<const char8_t*>(text)));
}

std::optional<std::filesystem::path> result(nfdresult_t status, nfdu8char_t* path) {
    const std::unique_ptr<nfdu8char_t, decltype(&NFD_FreePathU8)> owned(path, NFD_FreePathU8);
    if (status == NFD_CANCEL) return std::nullopt;
    if (status != NFD_OKAY) fail("Native file dialog failed");
    if (!path) return std::nullopt;
    return path_from_utf8(path);
}

std::vector<nfdu8filteritem_t> make_filters(const std::vector<FileFilter>& filters) {
    std::vector<nfdu8filteritem_t> items;
    items.reserve(filters.size());
    for (const auto& filter: filters) {
        items.push_back({ filter.name.c_str(), filter.extension.c_str() });
    }
    return items;
}

} // namespace

std::optional<std::filesystem::path> open_file(const std::vector<FileFilter>& filters, const std::filesystem::path& default_path) {
    ensure_initialized();
    const auto items = make_filters(filters);
    const auto initial = path_to_utf8(default_path);
    nfdu8char_t* path = nullptr;
    const auto status = NFD_OpenDialogU8(&path, items.empty() ? nullptr : items.data(), static_cast<nfdfiltersize_t>(items.size()),
            initial.empty() ? nullptr : initial.c_str());
    return result(status, path);
}

std::optional<std::filesystem::path> open_file(const FileFilter& filter, const std::filesystem::path& default_path) {
    return open_file(std::vector<FileFilter>{ filter }, default_path);
}

std::optional<std::filesystem::path> save_file(const std::vector<FileFilter>& filters, const std::filesystem::path& default_path,
        const std::filesystem::path& default_name) {
    ensure_initialized();
    const auto items = make_filters(filters);
    const auto initial = path_to_utf8(default_path);
    const auto name = path_to_utf8(default_name);
    nfdu8char_t* path = nullptr;
    const auto status = NFD_SaveDialogU8(&path, items.empty() ? nullptr : items.data(), static_cast<nfdfiltersize_t>(items.size()),
            initial.empty() ? nullptr : initial.c_str(), name.empty() ? nullptr : name.c_str());
    return result(status, path);
}

std::optional<std::filesystem::path> save_file(const FileFilter& filter, const std::filesystem::path& default_path,
        const std::filesystem::path& default_name) {
    return save_file(std::vector<FileFilter>{ filter }, default_path, default_name);
}

std::optional<std::filesystem::path> pick_folder(const std::filesystem::path& default_path) {
    ensure_initialized();
    const auto initial = path_to_utf8(default_path);
    nfdu8char_t* path = nullptr;
    const auto status = NFD_PickFolderU8(&path, initial.empty() ? nullptr : initial.c_str());
    return result(status, path);
}

} // namespace apps::nfd
