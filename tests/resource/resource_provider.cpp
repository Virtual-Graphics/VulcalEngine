#include "resource/directory_provider.h"
#include "resource/pak_provider.h"

#include <miniz.h>

#include <array>
#include <chrono>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {

class TemporaryDirectory {
public:
    TemporaryDirectory() {
        const auto suffix = std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());
        path_             = std::filesystem::temp_directory_path() / ("vulcal_resource_test_" + suffix);
        std::filesystem::create_directories(path_);
    }

    ~TemporaryDirectory() { std::filesystem::remove_all(path_); }

    const std::filesystem::path& path() const { return path_; }

private:
    std::filesystem::path path_;
};

void write_archive(const std::filesystem::path& path, const std::vector<std::pair<std::string, std::string>>& entries) {
    mz_zip_archive archive{};
    ASSERT_NE(mz_zip_writer_init_file(&archive, path.string().c_str(), 0), 0);
    for (const auto& [name, content]: entries) {
        ASSERT_NE(mz_zip_writer_add_mem(&archive, name.c_str(), content.data(), content.size(), MZ_DEFAULT_COMPRESSION), 0);
    }
    ASSERT_NE(mz_zip_writer_finalize_archive(&archive), 0);
    ASSERT_NE(mz_zip_writer_end(&archive), 0);
}

TEST(DirectoryProvider, ReadsTextAndBinaryFromLogicalPaths) {
    TemporaryDirectory directory;
    std::filesystem::create_directories(directory.path() / "assets" / "scenes");
    {
        std::ofstream output(directory.path() / "assets" / "scenes" / "demo.vscene", std::ios::binary);
        output << "format_version: 1\n";
    }
    const std::array<unsigned char, 3> binary{ 0, 12, 255 };
    {
        std::ofstream output(directory.path() / "asset.bin", std::ios::binary);
        output.write(reinterpret_cast<const char*>(binary.data()), static_cast<std::streamsize>(binary.size()));
    }

    const vulcal::resource::DirectoryProvider provider(directory.path());
    EXPECT_TRUE(provider.contains("assets/scenes/demo.vscene"));
    EXPECT_EQ(provider.read_text("assets/scenes/demo.vscene"), "format_version: 1\n");
    const auto bytes = provider.read_binary("asset.bin");
    ASSERT_EQ(bytes.size(), binary.size());
    EXPECT_EQ(std::to_integer<unsigned char>(bytes[0]), 0);
    EXPECT_EQ(std::to_integer<unsigned char>(bytes[1]), 12);
    EXPECT_EQ(std::to_integer<unsigned char>(bytes[2]), 255);
}

TEST(DirectoryProvider, ReadsUtf8LogicalPaths) {
    TemporaryDirectory directory;
    const auto filename = std::string("\xe6\xb5\x8b\xe8\xaf\x95.txt");
    std::u8string utf8_filename;
    for (const char character: filename) utf8_filename.push_back(static_cast<char8_t>(character));
    {
        std::ofstream output(directory.path() / std::filesystem::path(utf8_filename), std::ios::binary);
        output << "utf8";
    }

    const vulcal::resource::DirectoryProvider provider(directory.path());
    EXPECT_TRUE(provider.contains(filename));
    EXPECT_EQ(provider.read_text(filename), "utf8");
}

TEST(ResourceProviders, RejectsControlCharactersInLogicalPaths) {
    TemporaryDirectory directory;
    {
        std::ofstream output(directory.path() / "main.lua", std::ios::binary);
        output << "return 1";
    }
    const auto pak = directory.path() / "game.vpak";
    write_archive(pak, {
                           { "main.lua", "return 1" }
    });

    const vulcal::resource::DirectoryProvider directory_provider(directory.path());
    const vulcal::resource::PakProvider pak_provider(pak);
    const std::array<const vulcal::resource::ResourceProvider*, 2> providers{ &directory_provider, &pak_provider };

    std::vector<std::string> controls;
    for (unsigned int control = 0; control < 0x20; ++control) controls.emplace_back(1, static_cast<char>(control));
    controls.emplace_back(1, '\x7f');
    for (unsigned int control = 0x80; control <= 0x9f; ++control) {
        controls.push_back(std::string("\xc2") + static_cast<char>(control));
    }
    for (std::size_t index = 0; index < providers.size(); ++index) {
        SCOPED_TRACE(index == 0 ? "DirectoryProvider" : "PakProvider");
        for (const auto& control: controls) {
            const auto path = "main.lua" + control + ".txt";
            SCOPED_TRACE(::testing::PrintToString(path));
            ASSERT_THROW(providers[index]->read_text(path), std::invalid_argument);
            EXPECT_THROW(providers[index]->read_binary(path), std::invalid_argument);
            EXPECT_FALSE(providers[index]->contains(path));
        }
    }
}

TEST(DirectoryProvider, RequiresExactCaseForEveryLogicalPathComponent) {
    TemporaryDirectory directory;
    std::filesystem::create_directories(directory.path() / "Scripts");
    {
        std::ofstream output(directory.path() / "Scripts" / "main.lua", std::ios::binary);
        output << "return 1";
    }

    const vulcal::resource::DirectoryProvider provider(directory.path());
    EXPECT_TRUE(provider.contains("Scripts/main.lua"));
    EXPECT_EQ(provider.read_text("Scripts/main.lua"), "return 1");
    for (const auto path: { "scripts/main.lua", "Scripts/MAIN.lua", "SCRIPTS/MAIN.LUA" }) {
        SCOPED_TRACE(path);
        EXPECT_FALSE(provider.contains(path));
        EXPECT_THROW(provider.read_text(path), std::runtime_error);
        EXPECT_THROW(provider.read_binary(path), std::runtime_error);
    }
}

TEST(DirectoryProvider, RejectsMissingAndNonRelativePaths) {
    TemporaryDirectory directory;
    const vulcal::resource::DirectoryProvider provider(directory.path());

    EXPECT_FALSE(provider.contains("missing.txt"));
    EXPECT_FALSE(provider.contains("../outside.txt"));
    EXPECT_FALSE(provider.contains("/absolute.txt"));
    EXPECT_FALSE(provider.contains("C:/absolute.txt"));
    EXPECT_FALSE(provider.contains("folder\\file.txt"));
    EXPECT_THROW(provider.read_binary("missing.txt"), std::runtime_error);
    EXPECT_THROW(provider.read_text("../outside.txt"), std::invalid_argument);
}

TEST(DirectoryProvider, ReadsSymlinksWithinRoot) {
    TemporaryDirectory directory;
    std::filesystem::create_directories(directory.path() / "Scripts");
    {
        std::ofstream output(directory.path() / "Scripts" / "main.lua", std::ios::binary);
        output << "return 1";
    }
    std::error_code error;
    std::filesystem::create_directory_symlink(directory.path() / "Scripts", directory.path() / "Linked", error);
    if (error) GTEST_SKIP() << "Directory symlinks are unavailable: " << error.message();

    const vulcal::resource::DirectoryProvider provider(directory.path());
    EXPECT_TRUE(provider.contains("Linked/main.lua"));
    EXPECT_EQ(provider.read_text("Linked/main.lua"), "return 1");
    EXPECT_FALSE(provider.contains("linked/main.lua"));
    EXPECT_FALSE(provider.contains("Linked/MAIN.lua"));
}

TEST(DirectoryProvider, RejectsSymlinkEscape) {
    TemporaryDirectory directory;
    TemporaryDirectory outside;
    {
        std::ofstream output(outside.path() / "secret.txt");
        output << "secret";
    }
    std::error_code error;
    std::filesystem::create_directory_symlink(outside.path(), directory.path() / "linked", error);
    if (error) GTEST_SKIP() << "Directory symlinks are unavailable: " << error.message();

    const vulcal::resource::DirectoryProvider provider(directory.path());
    EXPECT_FALSE(provider.contains("linked/secret.txt"));
    EXPECT_THROW(provider.read_text("linked/secret.txt"), std::runtime_error);
}

TEST(PakProvider, ReadsTextAndBinaryWithoutExtractingTheArchive) {
    TemporaryDirectory directory;
    const auto pak = directory.path() / "game.vpak";
    write_archive(pak, {
                           { "assets/scenes/demo.vscene", "entities: []\n" },
                           { "asset.bin", std::string("\0\x7f\xff", 3) }
    });

    const vulcal::resource::PakProvider provider(pak);
    EXPECT_TRUE(provider.contains("assets/scenes/demo.vscene"));
    EXPECT_EQ(provider.read_text("assets/scenes/demo.vscene"), "entities: []\n");
    const auto bytes = provider.read_binary("asset.bin");
    ASSERT_EQ(bytes.size(), 3u);
    EXPECT_EQ(std::to_integer<unsigned char>(bytes[0]), 0);
    EXPECT_EQ(std::to_integer<unsigned char>(bytes[1]), 0x7f);
    EXPECT_EQ(std::to_integer<unsigned char>(bytes[2]), 0xff);
    EXPECT_FALSE(std::filesystem::exists(directory.path() / "assets"));
}

TEST(PakProvider, ReadsUtf8LogicalPaths) {
    TemporaryDirectory directory;
    const auto pak      = directory.path() / "unicode.vpak";
    const auto filename = std::string("\xe6\xb5\x8b\xe8\xaf\x95.txt");
    write_archive(pak, {
                           { filename, "utf8" }
    });

    const vulcal::resource::PakProvider provider(pak);
    EXPECT_TRUE(provider.contains(filename));
    EXPECT_EQ(provider.read_text(filename), "utf8");
}

TEST(PakProvider, RequiresExactCaseForEveryLogicalPathComponent) {
    TemporaryDirectory directory;
    const auto pak = directory.path() / "game.vpak";
    write_archive(pak, {
                           { "Scripts/main.lua", "return 1" }
    });

    const vulcal::resource::PakProvider provider(pak);
    EXPECT_TRUE(provider.contains("Scripts/main.lua"));
    EXPECT_EQ(provider.read_text("Scripts/main.lua"), "return 1");
    for (const auto path: { "scripts/main.lua", "Scripts/MAIN.lua", "SCRIPTS/MAIN.LUA" }) {
        SCOPED_TRACE(path);
        EXPECT_FALSE(provider.contains(path));
        EXPECT_THROW(provider.read_text(path), std::runtime_error);
        EXPECT_THROW(provider.read_binary(path), std::runtime_error);
    }
}

TEST(PakProvider, RejectsMissingAndUnsafePaths) {
    TemporaryDirectory directory;
    const auto pak = directory.path() / "game.vpak";
    write_archive(pak, {
                           { "assets/script.lua", "return 1" }
    });

    const vulcal::resource::PakProvider provider(pak);
    EXPECT_FALSE(provider.contains("missing.lua"));
    EXPECT_FALSE(provider.contains("../secret.lua"));
    EXPECT_THROW(provider.read_binary("missing.lua"), std::runtime_error);
    EXPECT_THROW(provider.read_text("../secret.lua"), std::invalid_argument);
}

TEST(PakProvider, RejectsUnsafeArchiveEntriesAndCorruptArchives) {
    TemporaryDirectory directory;
    const auto unsafe = directory.path() / "unsafe.vpak";
    write_archive(unsafe, {
                              { "../escape.txt", "escape" }
    });
    EXPECT_THROW({ vulcal::resource::PakProvider provider(unsafe); }, std::runtime_error);

    const auto corrupt = directory.path() / "corrupt.vpak";
    {
        std::ofstream output(corrupt, std::ios::binary);
        output << "not a zip archive";
    }
    EXPECT_THROW({ vulcal::resource::PakProvider provider(corrupt); }, std::runtime_error);
}

TEST(PakProvider, RejectsDuplicateArchiveEntries) {
    TemporaryDirectory directory;
    const auto pak = directory.path() / "duplicate.vpak";
    write_archive(pak, {
                           { "assets/script.lua",  "first" },
                           { "assets/script.lua", "second" }
    });

    EXPECT_THROW({ vulcal::resource::PakProvider provider(pak); }, std::runtime_error);
}

} // namespace
