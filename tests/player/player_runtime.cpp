#include "vulcal_player/player_runtime.h"
#include "base/path_utils.h"
#include "resource/directory_provider.h"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>

namespace {

class TemporaryDirectory {
public:
    TemporaryDirectory() {
        path_ = std::filesystem::temp_directory_path() /
                ("vulcal_player_test_" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
        std::filesystem::create_directories(path_);
    }
    ~TemporaryDirectory() { std::filesystem::remove_all(path_); }
    const std::filesystem::path& path() const { return path_; }

private:
    std::filesystem::path path_;
};

TEST(PlayerRuntime, UsesExplicitPakArgumentBeforeDirectoryContents) {
    TemporaryDirectory directory;
    const auto explicit_pak = directory.path() / "explicit.vpak";
    std::ofstream(explicit_pak).put('x');
    std::ofstream(directory.path() / "other.vpak").put('x');

    EXPECT_EQ(std::filesystem::canonical(vulcal::player::select_pak_file({ explicit_pak.string() }, directory.path())),
              std::filesystem::canonical(explicit_pak));
}

TEST(PlayerRuntime, RequiresExactlyOnePakBesidePlayerWithoutArgument) {
    TemporaryDirectory directory;
    EXPECT_THROW(vulcal::player::select_pak_file({}, directory.path()), std::runtime_error);
    std::ofstream(directory.path() / "game.vpak").put('x');
    EXPECT_EQ(std::filesystem::canonical(vulcal::player::select_pak_file({}, directory.path())),
              std::filesystem::canonical(directory.path() / "game.vpak"));
    std::ofstream(directory.path() / "second.vpak").put('x');
    EXPECT_THROW(vulcal::player::select_pak_file({}, directory.path()), std::runtime_error);
}

TEST(PlayerRuntime, AcceptsUtf8PackageArgument) {
    TemporaryDirectory directory;
    const auto pak = directory.path() / std::filesystem::path{ u8"\u6e38\u620f_\U0001f680.vpak" };
    std::ofstream(pak).put('x');
    const std::string argument = base::path_to_utf8(pak);
    EXPECT_EQ(vulcal::player::select_pak_file({ argument }, directory.path()), std::filesystem::canonical(pak));
}

TEST(PlayerRuntime, ReadsVersionedManifest) {
    TemporaryDirectory directory;
    const auto manifest = directory.path() / "vulcal_game.vmanifest";
    std::ofstream(manifest) << "version: 1\nengine_version: 0.0.5\nname: Demo\nopen_scene: assets/scenes/main.vscene\n";
    std::filesystem::create_directories(directory.path() / "assets" / "scenes");
    std::ofstream(directory.path() / "assets" / "scenes" / "main.vscene") << "format_version: 1\nentities: []\n";

    const vulcal::resource::DirectoryProvider resources(directory.path());
    const auto info = vulcal::player::load_manifest(resources);
    EXPECT_EQ(info.name, "Demo");
    EXPECT_EQ(info.open_scene, "assets/scenes/main.vscene");
}

TEST(PlayerRuntime, AcceptsUppercaseFileExtensions) {
    TemporaryDirectory directory;
    const auto pak = directory.path() / "GAME.VPAK";
    std::ofstream(pak).put('x');
    EXPECT_EQ(vulcal::player::select_pak_file({ pak.string() }, directory.path()), std::filesystem::canonical(pak));
    EXPECT_EQ(vulcal::player::select_pak_file({}, directory.path()), std::filesystem::canonical(pak));

    std::ofstream(directory.path() / "MAIN.VSCENE") << "format_version: 1\nentities: []\n";
    std::ofstream(directory.path() / "vulcal_game.vmanifest") << "version: 1\nengine_version: 0.0.5\nname: Demo\nopen_scene: MAIN.VSCENE\n";
    const vulcal::resource::DirectoryProvider resources(directory.path());
    EXPECT_EQ(vulcal::player::load_manifest(resources).open_scene, "MAIN.VSCENE");
}

TEST(PlayerRuntime, RejectsUnsupportedOrIncompleteManifest) {
    TemporaryDirectory directory;
    const auto manifest = directory.path() / "vulcal_game.vmanifest";
    std::ofstream(manifest) << "version: 2\nengine_version: 0.0.5\nname: Demo\nopen_scene: main.vscene\n";
    const vulcal::resource::DirectoryProvider resources(directory.path());
    EXPECT_THROW(vulcal::player::load_manifest(resources), std::runtime_error);

    std::ofstream(manifest, std::ios::trunc) << "version: 1\nname: Demo\n";
    EXPECT_THROW(vulcal::player::load_manifest(resources), std::runtime_error);
}

} // namespace
