#include "vulcal_build/build_pipeline.h"
#include "base/path_utils.h"
#include "vulcal_editor/project/project.h"
#include "resource/pak_provider.h"
#include "scene/scene_serializer.h"
#include "script/script_runtime.h"

#include <miniz.h>

#include <chrono>
#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>
#include <string>
#include <vector>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

namespace {

class TemporaryDirectory {
public:
    TemporaryDirectory() {
        path_ = std::filesystem::temp_directory_path() /
                ("vulcal_build_pipeline_" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
        std::filesystem::create_directories(path_);
    }

    ~TemporaryDirectory() { std::filesystem::remove_all(path_); }
    const std::filesystem::path& path() const { return path_; }

private:
    std::filesystem::path path_;
};

std::string read_entry(mz_zip_archive& archive, const char* name) {
    std::size_t size = 0;
    auto* bytes      = mz_zip_reader_extract_file_to_heap(&archive, name, &size, 0);
    if (!bytes) throw std::runtime_error(std::string("Missing ZIP entry: ") + name);
    std::string value(static_cast<const char*>(bytes), size);
    mz_free(bytes);
    return value;
}

std::string read_file(const std::filesystem::path& path) {
    std::ifstream input(path, std::ios::binary);
    return { std::istreambuf_iterator<char>{ input }, std::istreambuf_iterator<char>{} };
}

struct Fixture {
    explicit Fixture(const std::filesystem::path& root)
            : project_dir(root / "game"),
              project_file(project_dir / "game.vproj"),
              player(root / "vulcal_player.exe") {
        std::filesystem::create_directories(project_dir / "assets/scenes");
        std::filesystem::create_directories(project_dir / "assets/scripts");
        std::filesystem::create_directories(project_dir / "scripts");
        std::filesystem::create_directories(project_dir / ".vulcal_engine");
        std::ofstream(project_dir / "assets/scenes/main.vscene") << "format_version: 1\nentities: []\n";
        std::ofstream(project_dir / "assets/scripts/main.lua") << "return 7\n";
        std::ofstream(project_dir / "scripts/custom.lua") << "return 9\n";
        std::ofstream(project_dir / "assets/ignore.txt") << "not packaged";
        std::ofstream(project_dir / ".vulcal_engine/42.vmesh", std::ios::binary) << "cooked";
        std::ofstream(player, std::ios::binary) << "player-binary";

        editor::ProjectInfo info;
        info.name              = "game";
        info.description       = "";
        info.author            = "";
        info.assets_path       = "assets";
        info.last_edited_scene = "assets/scenes/main.vscene";
        info.build_info        = editor::BuildInfo{ .open_scene = "assets/scenes/main.vscene" };
        if (!editor::save_project_info(project_file, info)) throw std::runtime_error("Could not write fixture project");
    }

    std::filesystem::path project_dir;
    std::filesystem::path project_file;
    std::filesystem::path player;
};

void expect_only_published_files(const std::filesystem::path& output) {
    for (const auto& entry: std::filesystem::directory_iterator(output)) {
        EXPECT_TRUE(entry.path().filename() == "game.vpak" || entry.path().filename() == "vulcal_player.exe")
                << "Unexpected staging/backup file: " << entry.path();
    }
}

TEST(BuildPipeline, RejectsControlCharactersAndReservedProjectNamesBeforeTouchingOutputs) {
    TemporaryDirectory directory;
    Fixture fixture(directory.path());
    const auto output = fixture.project_dir / "build";
    std::filesystem::create_directories(output);
    std::ofstream(output / "vulcal_player.exe", std::ios::binary) << "old-player";
    editor::ProjectInfo info;
    ASSERT_TRUE(editor::load_project_info(fixture.project_file, info));
    std::vector<std::string> invalid_names{ "CON",         "prn",    "AUX.ext", "nul",   "COM1",  "lpt9.foo", "COM\xc2\xb9",
                                            "LPT\xc2\xb2", "CONIN$", "CONOUT$", "COM0.", "game.", "game " };
    for (int character = 0; character < 32; ++character)
        invalid_names.push_back("vulcal_player.exe" + std::string(1, static_cast<char>(character)) + "suffix");
    invalid_names.emplace_back("game\x7f");
    for (const auto& name: invalid_names) {
        SCOPED_TRACE(name);
        info.name = name;
        ASSERT_TRUE(editor::save_project_info(fixture.project_file, info));
        EXPECT_THROW(apps::vulcal_build::build_project(fixture.project_file, fixture.player), std::invalid_argument);
        EXPECT_EQ(read_file(output / "vulcal_player.exe"), "old-player");
    }
    EXPECT_EQ(std::distance(std::filesystem::directory_iterator(output), std::filesystem::directory_iterator{}), 1);
}

TEST(BuildPipeline, RejectsDirectoryOutputsWithoutMovingUserFiles) {
    for (const auto* filename: { "game.vpak", "vulcal_player.exe" }) {
        SCOPED_TRACE(filename);
        TemporaryDirectory directory;
        Fixture fixture(directory.path());
        const auto target = fixture.project_dir / "build" / filename;
        std::filesystem::create_directories(target);
        std::ofstream(target / "user-data") << "keep";
        EXPECT_THROW(apps::vulcal_build::build_project(fixture.project_file, fixture.player), std::invalid_argument);
        EXPECT_TRUE(std::filesystem::is_directory(target));
        EXPECT_EQ(read_file(target / "user-data"), "keep");
        EXPECT_EQ(std::distance(std::filesystem::directory_iterator(target.parent_path()), std::filesystem::directory_iterator{}), 1);
    }
}

TEST(BuildPipeline, RejectsOutputHardLinksToPlayerSource) {
    for (const auto* filename: { "game.vpak", "vulcal_player.exe" }) {
        TemporaryDirectory directory;
        Fixture fixture(directory.path());
        const auto target = fixture.project_dir / "build" / filename;
        std::filesystem::create_directories(target.parent_path());
        std::filesystem::create_hard_link(fixture.player, target);
        EXPECT_THROW(apps::vulcal_build::build_project(fixture.project_file, fixture.player), std::invalid_argument);
        EXPECT_TRUE(std::filesystem::equivalent(target, fixture.player));
        EXPECT_EQ(read_file(fixture.player), "player-binary");
        EXPECT_EQ(std::distance(std::filesystem::directory_iterator(target.parent_path()), std::filesystem::directory_iterator{}), 1);
    }
}

TEST(BuildPipeline, RejectsHardLinkedOutputTargets) {
    TemporaryDirectory directory;
    Fixture fixture(directory.path());
    const auto output = fixture.project_dir / "build";
    std::filesystem::create_directories(output);
    std::ofstream(output / "game.vpak") << "old-output";
    std::filesystem::create_hard_link(output / "game.vpak", output / "vulcal_player.exe");
    EXPECT_THROW(apps::vulcal_build::build_project(fixture.project_file, fixture.player), std::invalid_argument);
    EXPECT_TRUE(std::filesystem::equivalent(output / "game.vpak", output / "vulcal_player.exe"));
    EXPECT_EQ(read_file(output / "game.vpak"), "old-output");
    expect_only_published_files(output);
}

TEST(BuildPipeline, RejectsResourceSymlinksInsteadOfSilentlyOmittingFiles) {
    for (const auto* filename: { "assets/scripts/linked.lua", ".vulcal_engine/linked.vmesh", "scripts/linked.lua" }) {
        TemporaryDirectory directory;
        Fixture fixture(directory.path());
        const auto target = fixture.project_dir / filename;
        std::error_code error;
        std::filesystem::create_symlink(fixture.project_dir / "assets/scripts/main.lua", target, error);
        if (error) GTEST_SKIP() << "Creating symlinks is unavailable: " << error.message();
        EXPECT_THROW(apps::vulcal_build::build_project(fixture.project_file, fixture.player), std::runtime_error);
        EXPECT_FALSE(std::filesystem::exists(fixture.project_dir / "build/game.vpak"));
    }
}

TEST(BuildPipeline, RejectsSymlinkOutputsWithoutChangingTheirTargets) {
    for (const auto* filename: { "game.vpak", "vulcal_player.exe" }) {
        TemporaryDirectory directory;
        Fixture fixture(directory.path());
        const auto output = fixture.project_dir / "build";
        std::filesystem::create_directories(output);
        const auto user_file = directory.path() / "user-data";
        std::ofstream(user_file) << "keep";
        std::error_code error;
        std::filesystem::create_symlink(user_file, output / filename, error);
        if (error) GTEST_SKIP() << "Creating symlinks is unavailable: " << error.message();
        EXPECT_THROW(apps::vulcal_build::build_project(fixture.project_file, fixture.player), std::invalid_argument);
        EXPECT_TRUE(std::filesystem::is_symlink(output / filename));
        EXPECT_EQ(read_file(user_file), "keep");
    }
}

TEST(BuildPipeline, PreservesExecutablePermissionsOnCopiedPlayer) {
#ifdef _WIN32
    GTEST_SKIP() << "POSIX executable permissions do not apply on Windows";
#else
    TemporaryDirectory directory;
    Fixture fixture(directory.path());
    const auto permissions = std::filesystem::perms::owner_read | std::filesystem::perms::owner_write | std::filesystem::perms::owner_exec |
                             std::filesystem::perms::group_exec;
    std::filesystem::permissions(fixture.player, permissions);
    const auto result = apps::vulcal_build::build_project(fixture.project_file, fixture.player);
    EXPECT_EQ(std::filesystem::status(result.player_file).permissions(), permissions);
#endif
}

TEST(BuildPipeline, RejectsCaseAliasedOutputNamesOnWindows) {
#ifdef _WIN32
    TemporaryDirectory directory;
    Fixture fixture(directory.path());
    const auto player = directory.path() / "GAME.VPAK";
    std::ofstream(player) << "player-binary";
    EXPECT_THROW(apps::vulcal_build::build_project(fixture.project_file, player), std::invalid_argument);
    EXPECT_FALSE(std::filesystem::exists(fixture.project_dir / "build/game.vpak"));
#else
    GTEST_SKIP() << "Windows case-insensitive filename behavior";
#endif
}

TEST(BuildPipeline, RestoresPackageWhenLockedPlayerCannotBeBackedUp) {
#ifdef _WIN32
    TemporaryDirectory directory;
    Fixture fixture(directory.path());
    const auto output = fixture.project_dir / "build";
    std::filesystem::create_directories(output);
    std::ofstream(output / "game.vpak", std::ios::binary) << "old-pak";
    std::ofstream(output / "vulcal_player.exe", std::ios::binary) << "old-player";
    struct LockedFile {
        HANDLE handle;
        ~LockedFile() {
            if (handle != INVALID_HANDLE_VALUE) CloseHandle(handle);
        }
    } locked{ CreateFileW((output / "vulcal_player.exe").c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING,
                          FILE_ATTRIBUTE_NORMAL, nullptr) };
    ASSERT_NE(locked.handle, INVALID_HANDLE_VALUE);
    EXPECT_THROW(apps::vulcal_build::build_project(fixture.project_file, fixture.player), std::runtime_error);
    EXPECT_EQ(read_file(output / "game.vpak"), "old-pak");
    EXPECT_EQ(read_file(output / "vulcal_player.exe"), "old-player");
    expect_only_published_files(output);
#else
    GTEST_SKIP() << "Windows delete-sharing lock is required to force the second backup rename to fail";
#endif
}

TEST(BuildPipeline, PackagesRuntimeFilesManifestAndPlayer) {
    TemporaryDirectory directory;
    Fixture fixture(directory.path());

    const auto result = apps::vulcal_build::build_project(fixture.project_file, fixture.player);
    EXPECT_EQ(std::filesystem::weakly_canonical(result.pak_file),
              std::filesystem::weakly_canonical(fixture.project_dir / "build" / "game.vpak"));
    EXPECT_EQ(std::filesystem::weakly_canonical(result.player_file),
              std::filesystem::weakly_canonical(fixture.project_dir / "build" / fixture.player.filename()));
    ASSERT_TRUE(std::filesystem::exists(result.pak_file));
    EXPECT_EQ(std::filesystem::file_size(result.player_file), std::filesystem::file_size(fixture.player));

    mz_zip_archive archive{};
    ASSERT_NE(mz_zip_reader_init_file(&archive, result.pak_file.string().c_str(), 0), 0);
    EXPECT_EQ(read_entry(archive, "assets/scenes/main.vscene"), read_file(fixture.project_dir / "assets/scenes/main.vscene"));
    EXPECT_EQ(read_entry(archive, "assets/scripts/main.lua"), read_file(fixture.project_dir / "assets/scripts/main.lua"));
    ASSERT_NE(mz_zip_reader_locate_file(&archive, "scripts/custom.lua", nullptr, 0), -1);
    EXPECT_EQ(read_entry(archive, "scripts/custom.lua"), read_file(fixture.project_dir / "scripts/custom.lua"));
    EXPECT_EQ(read_entry(archive, ".vulcal_engine/42.vmesh"), "cooked");
    EXPECT_EQ(mz_zip_reader_locate_file(&archive, "assets/ignore.txt", nullptr, 0), -1);
    const auto manifest = read_entry(archive, "vulcal_game.vmanifest");
    EXPECT_NE(manifest.find("version: 1"), std::string::npos);
    EXPECT_NE(manifest.find("engine_version: 0.0.5"), std::string::npos);
    EXPECT_NE(manifest.find("open_scene: assets/scenes/main.vscene"), std::string::npos);
    mz_zip_reader_end(&archive);
}

TEST(BuildPipeline, RunsSerializedSceneAndProjectRootLuaDirectlyFromPackage) {
    TemporaryDirectory directory;
    Fixture fixture(directory.path());
    vulcal::scene::Scene scene;
    auto entity                                                         = scene.create_entity("PackagedScript");
    entity.add_component<vulcal::scene::ScriptComponent>().script_path_ = "scripts/custom.lua";
    vulcal::scene::SceneSerializer::serialize(scene, fixture.project_dir / "assets/scenes/main.vscene");
    std::ofstream(fixture.project_dir / "scripts/custom.lua", std::ios::binary)
            << "function on_create() entity:set_translation(11, 22, 33) end\n";

    const auto result = apps::vulcal_build::build_project(fixture.project_file, fixture.player);
    vulcal::resource::PakProvider resources(result.pak_file);
    auto packaged_scene = vulcal::scene::SceneSerializer::deserialize(resources, "assets/scenes/main.vscene");
    ASSERT_TRUE(packaged_scene);
    const auto scripts = packaged_scene->registry().view<vulcal::scene::ScriptComponent>();
    ASSERT_EQ(scripts.size(), 1u);
    EXPECT_EQ(scripts.get<vulcal::scene::ScriptComponent>(scripts.front()).script_path_, "scripts/custom.lua");
    vulcal::script::ScriptRuntime runtime;
    runtime.start(*packaged_scene, resources);
    EXPECT_TRUE(runtime.running());
    EXPECT_EQ(packaged_scene->registry().get<vulcal::scene::TransformComponent>(scripts.front()).translation_,
              (glm::vec3{ 11.0f, 22.0f, 33.0f }));
    runtime.stop();
}

TEST(BuildPipeline, BuildsUtf8ProjectNamesScenesAndOutputDirectories) {
    TemporaryDirectory directory;
    const auto fixture_root = directory.path() / std::filesystem::path(u8"\u9879\u76ee\U0001f3ae");
    Fixture fixture(fixture_root);
    const auto scene_relative = std::filesystem::path(u8"assets/scenes/\u542f\u7a0b\U0001f3ae.vscene");
    std::filesystem::rename(fixture.project_dir / "assets/scenes/main.vscene", fixture.project_dir / scene_relative);
    const auto player = fixture_root / std::filesystem::path(u8"\u64ad\u653e\u5668\U0001f3ae.exe");
    std::filesystem::rename(fixture.player, player);
    editor::ProjectInfo info;
    ASSERT_TRUE(editor::load_project_info(fixture.project_file, info));
    const auto project_name     = std::filesystem::path(u8"\u6e38\u620f\U0001f3ae");
    info.name                   = base::path_to_utf8(project_name);
    info.build_info->open_scene = scene_relative;
    info.build_info->build_dir  = std::filesystem::canonical(fixture.project_dir) / std::filesystem::path(u8"\u53d1\u5e03\U0001f3ae");
    ASSERT_TRUE(editor::save_project_info(fixture.project_file, info));

    const auto result = apps::vulcal_build::build_project(fixture.project_file, player);
    EXPECT_EQ(result.pak_file, info.build_info->build_dir / std::filesystem::path(u8"\u6e38\u620f\U0001f3ae.vpak"));
    EXPECT_EQ(result.player_file, info.build_info->build_dir / player.filename());
    EXPECT_EQ(read_file(result.player_file), "player-binary");
    vulcal::resource::PakProvider resources(result.pak_file);
    EXPECT_EQ(resources.read_text(base::path_to_utf8(scene_relative)), read_file(fixture.project_dir / scene_relative));
    const auto manifest = YAML::Load(resources.read_text("vulcal_game.vmanifest"));
    EXPECT_EQ(manifest["name"].as<std::string>(), base::path_to_utf8(project_name));
    EXPECT_EQ(manifest["open_scene"].as<std::string>(), base::path_to_utf8(scene_relative));
}

TEST(BuildPipeline, FailedValidationPreservesPreviouslyPublishedFiles) {
    TemporaryDirectory directory;
    Fixture fixture(directory.path());
    const auto output = fixture.project_dir / "build";
    std::filesystem::create_directories(output);
    const auto old_pak    = output / "game.vpak";
    const auto old_player = output / fixture.player.filename();
    std::ofstream(old_pak, std::ios::binary) << "old-pak";
    std::ofstream(old_player, std::ios::binary) << "old-player";
    std::filesystem::remove(fixture.project_dir / "assets/scenes/main.vscene");

    EXPECT_THROW(apps::vulcal_build::build_project(fixture.project_file, fixture.player), std::runtime_error);
    std::ifstream pak_input(old_pak, std::ios::binary);
    std::ifstream player_input(old_player, std::ios::binary);
    EXPECT_EQ(std::string(std::istreambuf_iterator<char>{ pak_input }, {}), "old-pak");
    EXPECT_EQ(std::string(std::istreambuf_iterator<char>{ player_input }, {}), "old-player");
}

TEST(BuildPipeline, AcceptsUppercaseSceneExtensionFromEditorSettings) {
    TemporaryDirectory directory;
    Fixture fixture(directory.path());
    const auto scene = fixture.project_dir / "assets/scenes/MAIN.VSCENE";
    std::filesystem::rename(fixture.project_dir / "assets/scenes/main.vscene", scene);
    editor::ProjectInfo info;
    ASSERT_TRUE(editor::load_project_info(fixture.project_file, info));
    info.build_info->open_scene = "assets/scenes/MAIN.VSCENE";
    ASSERT_TRUE(editor::save_project_info(fixture.project_file, info));
    const auto result = apps::vulcal_build::build_project(fixture.project_file, fixture.player);
    vulcal::resource::PakProvider resources(result.pak_file);
    EXPECT_EQ(resources.read_text("assets/scenes/MAIN.VSCENE"), read_file(scene));
}

TEST(BuildPipeline, KeepsScriptsWhenOutputDirectoryContainsProjectScripts) {
    TemporaryDirectory directory;
    Fixture fixture(directory.path());
    editor::ProjectInfo info;
    ASSERT_TRUE(editor::load_project_info(fixture.project_file, info));
    info.build_info->build_dir = std::filesystem::absolute(fixture.project_dir / "scripts");
    ASSERT_TRUE(editor::save_project_info(fixture.project_file, info));
    const auto result = apps::vulcal_build::build_project(fixture.project_file, fixture.player);
    const vulcal::resource::PakProvider resources(result.pak_file);
    EXPECT_TRUE(resources.contains("scripts/custom.lua"));
}

TEST(BuildPipeline, RejectsOutputInsideCookedResourcesBeforeWritingArtifacts) {
    TemporaryDirectory directory;
    Fixture fixture(directory.path());
    editor::ProjectInfo info;
    ASSERT_TRUE(editor::load_project_info(fixture.project_file, info));
    info.build_info->build_dir = std::filesystem::absolute(fixture.project_dir / ".vulcal_engine/release");
    ASSERT_TRUE(editor::save_project_info(fixture.project_file, info));
    EXPECT_THROW(apps::vulcal_build::build_project(fixture.project_file, fixture.player), std::invalid_argument);
    EXPECT_FALSE(std::filesystem::exists(info.build_info->build_dir / "game.vpak"));
    EXPECT_EQ(read_file(fixture.project_dir / ".vulcal_engine/42.vmesh"), "cooked");
}

TEST(BuildPipeline, RejectsCollidingOutputsWithoutCreatingPartialPackage) {
    TemporaryDirectory directory;
    Fixture fixture(directory.path());
    const auto conflicting_player = directory.path() / "game.vpak";
    std::ofstream(conflicting_player, std::ios::binary) << "player-binary";

    EXPECT_THROW(apps::vulcal_build::build_project(fixture.project_file, conflicting_player), std::invalid_argument);
    EXPECT_FALSE(std::filesystem::exists(fixture.project_dir / "build" / "game.vpak"));
}

TEST(BuildPipeline, RejectsScenePathOutsideProjectAssets) {
    TemporaryDirectory directory;
    Fixture fixture(directory.path());
    editor::ProjectInfo info;
    ASSERT_TRUE(editor::load_project_info(fixture.project_file, info));
    info.build_info->open_scene = "../escape.vscene";
    ASSERT_TRUE(editor::save_project_info(fixture.project_file, info));

    EXPECT_THROW(apps::vulcal_build::build_project(fixture.project_file, fixture.player), std::invalid_argument);
}

} // namespace
