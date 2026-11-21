#include "vulcal_build/build_pipeline.h"

#include "base/path_utils.h"
#include "base/string_utils.h"
#include "common/project_paths.h"
#include "vulcal_editor/project/project.h"

#include <miniz.h>

#include <algorithm>
#include <chrono>
#include <fstream>
#include <limits>
#include <stdexcept>
#include <string>
#include <system_error>
#include <vector>
#include <yaml-cpp/yaml.h>

#ifdef _WIN32
    #ifndef NOMINMAX
        #define NOMINMAX
    #endif
    #include <windows.h>
#endif

namespace apps::vulcal_build {
namespace {

constexpr std::string_view engine_version = "0.0.5";
constexpr std::string_view manifest_name  = "vulcal_game.vmanifest";


bool valid_project_name(std::string_view name) {
    // 拒绝 空名、结尾字符、非法字符
    if (name.empty() || name.back() == '.' || name.back() == ' ' || name.find_first_of("<>:\"/\\|?*") != std::string_view::npos) {
        return false;
    }

    // 拒绝 ASCII 小于 32 和 127 的控制字符
    if (std::any_of(name.begin(), name.end(), [](unsigned char value) { return value < 32 || value == 127; })) {
        return false;
    }

    // 拒绝 Windows 设备保留名
    auto stem = std::string(name.substr(0, name.find('.')));
    while (!stem.empty() && stem.back() == ' ') stem.pop_back();
    stem = base::to_upper(stem);
    if (stem == "CON" || stem == "PRN" || stem == "AUX" || stem == "NUL" || stem == "CONIN$" || stem == "CONOUT$") {
        return false;
    }
    if (stem.starts_with("COM") || stem.starts_with("LPT")) {
        const auto suffix = std::string_view(stem).substr(3);
        if ((suffix.size() == 1 && suffix.front() >= '1' && suffix.front() <= '9') || suffix == "\xc2\xb9" || suffix == "\xc2\xb2" ||
            suffix == "\xc2\xb3") {
            return false;
        }
    }
    return true;
}

// 确保构建输出目标不存在或目标普通文件。拒绝目录与符号链接。
void validate_output_target(const std::filesystem::path& target) {
    std::error_code error;
    const auto status = std::filesystem::symlink_status(target, error);

    if (error == std::errc::no_such_file_or_directory) {
        return;
    }
    if (error) { 
        throw std::runtime_error("Could not inspect build output: " + error.message());
    }
    if (status.type() != std::filesystem::file_type::not_found && !std::filesystem::is_regular_file(status)) {
        throw std::invalid_argument("Existing build output must be a regular file, not a directory or symlink: " + target.string());
    }
}

bool is_same_file(const std::filesystem::path& first, const std::filesystem::path& second) {
    if (first == second) {
        return true;
    }

#ifdef _WIN32
    const auto& first_native  = first.native();
    const auto& second_native = second.native();

    const auto max_int = static_cast<std::size_t>(std::numeric_limits<int>::max());
    const bool fits_int = first_native.size() <= max_int && second_native.size() <= max_int;

    if (fits_int) {
        const auto result = CompareStringOrdinal(first_native.c_str(), static_cast<int>(first_native.size()),
                                                 second_native.c_str(), static_cast<int>(second_native.size()), TRUE);
        if (result == CSTR_EQUAL) {
            return true;
        }
    }
#endif

    std::error_code error;
    const bool equivalent = std::filesystem::equivalent(first, second, error);
    if (error && error != std::errc::no_such_file_or_directory) {
        throw std::runtime_error("Could not compare build output files: " + error.message());
    }
    return !error && equivalent;
}

struct InputFile {
    std::filesystem::path path;
    std::string archive_path;
};

void collect_tree(const std::filesystem::path& root, const std::filesystem::path& project_root, bool filter_extensions,
                  std::vector<InputFile>& output) {
    if (std::filesystem::is_symlink(std::filesystem::symlink_status(root))) {
        throw std::runtime_error("Resource directory symlinks are not supported: " + root.string());
    }
    if (!std::filesystem::exists(root)) {
        return;
    }

    std::error_code error;
    const auto canonical_root = std::filesystem::canonical(root, error);
    if (error || !std::filesystem::is_directory(canonical_root)) {
        throw std::runtime_error("Resource directory is unavailable: " + root.string());
    }

    for (std::filesystem::recursive_directory_iterator it(canonical_root), end; it != end; ++it) {
        const auto status = it->symlink_status();
        if (std::filesystem::is_symlink(status)) {
            throw std::runtime_error("Resource symlinks are not supported: " + it->path().string());
        }
        if (!std::filesystem::is_regular_file(status)) {
            continue;
        }
        if (filter_extensions) {
            const auto extension = base::lowercase_extension(it->path());
            if (extension != ".vscene" && extension != ".lua") {
                continue;
            }
        }
        const auto relative = base::relative_to(it->path(), project_root);
        if (relative.empty()) {
            throw std::runtime_error("Resource file is outside project: " + it->path().string());
        }
        const auto archive_path = base::path_to_utf8(apps::project_paths::validate_relative_path(relative, "packaged resource path"));
        output.push_back({ it->path(), archive_path });
    }
}

void collect_project_scripts(const std::filesystem::path& project_root, const std::filesystem::path& assets_root,
                             const std::filesystem::path& cooked_root, std::vector<InputFile>& output) {
    std::vector<std::filesystem::path> excluded_roots{ assets_root };
    for (const auto& candidate: { cooked_root, project_root / ".git" }) {
        std::error_code error;
        if (!std::filesystem::exists(candidate, error)) continue;
        const auto canonical = std::filesystem::canonical(candidate, error);
        if (!error && canonical != project_root && base::is_within(canonical, project_root)) excluded_roots.push_back(canonical);
    }

    std::error_code error;
    for (std::filesystem::recursive_directory_iterator it(project_root, error), end; !error && it != end; it.increment(error)) {
        const auto status = it->symlink_status(error);
        if (error) break;
        if (std::filesystem::is_symlink(status)) {
            throw std::runtime_error("Project resource symlinks are not supported: " + it->path().string());
        }
        if (std::filesystem::is_directory(status)) {
            if (std::any_of(excluded_roots.begin(), excluded_roots.end(),
                            [&](const auto& root) { return base::is_within(it->path(), root); })) {
                it.disable_recursion_pending();
            }
            continue;
        }
        if (!std::filesystem::is_regular_file(status)) continue;

        const auto extension = base::lowercase_extension(it->path());
        if (extension != ".lua") continue;

        const auto relative = base::relative_to(it->path(), project_root);
        if (relative.empty()) throw std::runtime_error("Project script is outside project: " + it->path().string());
        const auto archive_path = base::path_to_utf8(apps::project_paths::validate_relative_path(relative, "packaged script path"));
        output.push_back({ it->path(), archive_path });
    }
    if (error) throw std::runtime_error("Could not scan project scripts: " + error.message());
}

void add_file(mz_zip_archive& archive, const InputFile& file) {
    const auto source = base::path_to_utf8(file.path);
    if (!mz_zip_writer_add_file(&archive, file.archive_path.c_str(), source.c_str(), nullptr, 0, MZ_DEFAULT_COMPRESSION)) {
        throw std::runtime_error("Could not add resource to package: " + file.archive_path);
    }
}

struct TempOutputs {
    std::filesystem::path pak;
    std::filesystem::path player;
    std::filesystem::path pak_target;
    std::filesystem::path player_target;
    std::filesystem::path pak_backup;
    std::filesystem::path player_backup;
    bool pak_published    = false;
    bool player_published = false;
    bool pak_backed_up    = false;
    bool player_backed_up = false;
    bool committed        = false;

    ~TempOutputs() {
        std::error_code error;
        std::filesystem::remove(pak, error);
        std::filesystem::remove(player, error);
        if (committed) {
            std::filesystem::remove(pak_backup, error);
            std::filesystem::remove(player_backup, error);
            return;
        }
        if (pak_published) std::filesystem::remove(pak_target, error);
        if (player_published) std::filesystem::remove(player_target, error);
        if (pak_backed_up) {
            std::filesystem::rename(pak_backup, pak_target, error);
        } else {
            std::filesystem::remove(pak_backup, error);
        }
        if (player_backed_up) {
            std::filesystem::rename(player_backup, player_target, error);
        } else {
            std::filesystem::remove(player_backup, error);
        }
    }
};

std::filesystem::path unique_sibling(const std::filesystem::path& target, std::string_view tag) {
    const auto nonce = std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());
    auto filename    = target.filename();
    filename += base::path_from_utf8("." + std::string(tag) + "." + nonce);
    return target.parent_path() / filename;
}

void copy_player_file(const std::filesystem::path& source, const std::filesystem::path& target) {
    std::error_code error;
    if (!std::filesystem::copy_file(source, target, std::filesystem::copy_options::none, error)) {
        throw std::runtime_error("Could not copy staged Player executable: " + error.message());
    }
}

void publish(TempOutputs& staged, const std::filesystem::path& pak_target, const std::filesystem::path& player_target) {
    validate_output_target(pak_target);
    validate_output_target(player_target);
    std::error_code error;
    if (std::filesystem::exists(pak_target)) {
        std::filesystem::rename(pak_target, staged.pak_backup, error);
        if (error) throw std::runtime_error("Could not stage existing package for replacement: " + error.message());
        staged.pak_backed_up = true;
    }
    if (std::filesystem::exists(player_target)) {
        std::filesystem::rename(player_target, staged.player_backup, error);
        if (error) throw std::runtime_error("Could not stage existing Player for replacement: " + error.message());
        staged.player_backed_up = true;
    }

    std::filesystem::rename(staged.pak, pak_target, error);
    if (error) throw std::runtime_error("Could not publish package: " + error.message());
    staged.pak_published = true;
    std::filesystem::rename(staged.player, player_target, error);
    if (error) throw std::runtime_error("Could not publish Player: " + error.message());
    staged.player_published = true;

    staged.committed        = true;
}

} // namespace

BuildResult build_project(const std::filesystem::path& project_file, const std::filesystem::path& player_executable) {
   
    // 预处理 begin
    editor::ProjectInfo info;
    if (!editor::load_project_info(project_file, info)) { 
        throw std::runtime_error("Could not load project file: " + project_file.string());
    }
    if (!info.build_info) { 
        throw std::invalid_argument("Project has no build_info configuration");
    }
    if (!valid_project_name(info.name)) { 
        throw std::invalid_argument("Project name cannot be used as an output filename");
    }
    apps::project_paths::validate_relative_path(info.assets_path, "assets_path");
    const auto scene_relative = apps::project_paths::validate_scene_path(info.build_info->open_scene, info.assets_path, "build_info.open_scene");

    const auto project_root = std::filesystem::canonical(project_file).parent_path();
    auto assets_component   = project_root;
    for (const auto& component: info.assets_path) {
        assets_component /= component;
        if (std::filesystem::is_symlink(std::filesystem::symlink_status(assets_component))) {
            throw std::runtime_error("Asset directory symlinks are not supported: " + assets_component.string());
        }
    }
    const auto assets_root = std::filesystem::canonical(project_root / info.assets_path);
    const auto scene_path  = std::filesystem::canonical(project_root / scene_relative);
    if (!std::filesystem::is_regular_file(scene_path) || !base::is_within(scene_path, assets_root)) {
        throw std::invalid_argument("Build open_scene does not exist inside the assets directory");
    }

    std::error_code error;
    const auto player_source = std::filesystem::canonical(player_executable, error);
    if (error || !std::filesystem::is_regular_file(player_source))
        throw std::invalid_argument("Player executable does not exist: " + player_executable.string());

    auto build_dir = info.build_info->build_dir.empty() ? project_root / "build" : info.build_info->build_dir;
    if (!build_dir.is_absolute()) throw std::invalid_argument("Build output directory must be absolute");
    const auto cooked_root = project_root / ".vulcal_engine";
    if (base::is_within(build_dir, cooked_root)) {
        throw std::invalid_argument("Build output directory cannot be inside cooked resources");
    }
    // 预处理 end

    std::filesystem::create_directories(build_dir);
    build_dir                = std::filesystem::canonical(build_dir);

    const auto pak_target    = build_dir / base::path_from_utf8(info.name + ".vpak");
    const auto player_target = build_dir / player_source.filename();
    validate_output_target(pak_target);
    validate_output_target(player_target);
    if (is_same_file(pak_target, player_target)) { 
        throw std::invalid_argument("Build package and Player outputs refer to the same file");
    }
    if (is_same_file(pak_target, player_source) || is_same_file(player_target, player_source)) {
        throw std::invalid_argument("Build outputs cannot overwrite the source Player executable");
    }
    const auto staged_pak    = unique_sibling(pak_target, "tmp");
    const auto staged_player = unique_sibling(player_target, "tmp");
    TempOutputs staged{
        staged_pak, staged_player, pak_target, player_target, unique_sibling(pak_target, "bak"), unique_sibling(player_target, "bak")
    };

    std::vector<InputFile> files;
    if (std::filesystem::exists(cooked_root) && !base::is_within(std::filesystem::canonical(cooked_root), project_root)) {
        throw std::runtime_error("Cooked resource directory resolves outside project");
    }
    collect_tree(cooked_root, project_root, false, files);
    collect_tree(assets_root, project_root, true, files);
    collect_project_scripts(project_root, assets_root, cooked_root, files);
    std::sort(files.begin(), files.end(), [](const auto& lhs, const auto& rhs) { return lhs.archive_path < rhs.archive_path; });
    for (std::size_t i = 1; i < files.size(); ++i) {
        if (files[i - 1].archive_path == files[i].archive_path)
            throw std::runtime_error("Duplicate packaged path: " + files[i].archive_path);
    }
    const auto scene_archive_path = base::path_to_utf8(scene_relative);
    if (std::none_of(files.begin(), files.end(), [&](const auto& file) { return file.archive_path == scene_archive_path; })) {
        throw std::runtime_error("Build open_scene is not among the packaged scene files");
    }

    YAML::Emitter manifest;
    manifest << YAML::BeginMap << YAML::Key << "version" << YAML::Value << std::uint32_t{ 1 } << YAML::Key << "engine_version"
             << YAML::Value << std::string(engine_version) << YAML::Key << "name" << YAML::Value << info.name << YAML::Key << "open_scene"
             << YAML::Value << scene_archive_path << YAML::EndMap;
    if (!manifest.good()) throw std::runtime_error("Could not serialize game manifest");

    mz_zip_archive archive{};
    if (!mz_zip_writer_init_file(&archive, base::path_to_utf8(staged.pak).c_str(), 0)) {
        throw std::runtime_error("Could not create temporary game package");
    }
    bool archive_open = true;
    try {
        for (const auto& file: files) add_file(archive, file);
        const std::string manifest_text = manifest.c_str();
        if (!mz_zip_writer_add_mem(&archive, manifest_name.data(), manifest_text.data(), manifest_text.size(), MZ_DEFAULT_COMPRESSION)) {
            throw std::runtime_error("Could not add manifest to game package");
        }
        if (!mz_zip_writer_finalize_archive(&archive)) { 
            throw std::runtime_error("Could not finalize game package");
        }
        if (!mz_zip_writer_end(&archive)) { 
            throw std::runtime_error("Could not close game package");
        }
        archive_open = false;
    } catch (...) {
        if (archive_open) mz_zip_writer_end(&archive);
        throw;
    }

    copy_player_file(player_source, staged.player);
    publish(staged, pak_target, player_target);
    return { pak_target, player_target };
}

} // namespace apps::vulcal_build
