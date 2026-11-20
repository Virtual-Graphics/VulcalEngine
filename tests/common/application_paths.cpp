#include "common/application_paths.h"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>
#include <string>

namespace {

class TemporaryDirectory {
public:
    TemporaryDirectory() {
        path_ = std::filesystem::temp_directory_path() /
                ("vulcal_app_paths_" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
        std::filesystem::create_directories(path_);
    }

    ~TemporaryDirectory() { std::filesystem::remove_all(path_); }

    const std::filesystem::path& path() const { return path_; }

private:
    std::filesystem::path path_;
};

std::string executable_name(std::string name) {
#ifdef _WIN32
    name += ".exe";
#endif
    return name;
}

void create_executable(const std::filesystem::path& directory, const std::string& name) {
    std::filesystem::create_directories(directory);
    std::ofstream(directory / executable_name(name)).put('x');
}

TEST(ApplicationPaths, PrefersTheApplicationDirectoryToPath) {
    TemporaryDirectory directory;
    const auto application_dir = directory.path() / "app";
    const auto path_dir        = directory.path() / "path";
    create_executable(application_dir, "vulcal_build");
    create_executable(path_dir, "vulcal_build");

    const auto found = apps::app_paths::find_executable("vulcal_build", application_dir, path_dir.string());
    EXPECT_EQ(found, application_dir / executable_name("vulcal_build"));
}

TEST(ApplicationPaths, FindsExecutableInPathWhenItIsNotBesideTheApplication) {
    TemporaryDirectory directory;
    const auto application_dir = directory.path() / "app";
    const auto path_dir        = directory.path() / "tools";
    create_executable(path_dir, "vulcal_player");

    const auto found = apps::app_paths::find_executable("vulcal_player", application_dir, path_dir.string());
    EXPECT_EQ(found, path_dir / executable_name("vulcal_player"));
}

TEST(ApplicationPaths, ReturnsAnEmptyPathWhenTheExecutableCannotBeFound) {
    TemporaryDirectory directory;
    EXPECT_TRUE(apps::app_paths::find_executable("missing", directory.path(), {}).empty());
}

TEST(ApplicationPaths, FindsExecutableOnUtf8Path) {
    TemporaryDirectory directory;
    const auto tools = directory.path() / std::filesystem::path{ u8"\u5de5\u5177_\U0001f680" };
    create_executable(tools, "vulcal_player");
    const auto encoded = tools.generic_u8string();
    const std::string search_path{ reinterpret_cast<const char*>(encoded.data()), encoded.size() };
    EXPECT_EQ(apps::app_paths::find_executable("vulcal_player", directory.path(), search_path),
              tools / executable_name("vulcal_player"));
}

} // namespace
