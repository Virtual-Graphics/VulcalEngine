#include "asset/texture_asset.h"
#include "base/uuid.h"

#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>

namespace {

class TemporaryDirectory {
public:
    TemporaryDirectory() {
        path_ = std::filesystem::temp_directory_path() / ("vulcal_texture_asset_" + base::UUID::generate().to_string());
        std::filesystem::create_directories(path_);
    }

    ~TemporaryDirectory() { std::filesystem::remove_all(path_); }

    const std::filesystem::path& path() const { return path_; }

private:
    std::filesystem::path path_;
};

vulcal::render::TextureData test_texture(std::uint8_t first_red = 255) {
    vulcal::render::TextureData texture;
    texture.width_  = 2;
    texture.height_ = 1;
    texture.format_ = vulcal::render::TextureFormat::RGBA8;
    texture.pixels_ = { first_red, 0, 0, 255, 0, 255, 0, 128 };
    return texture;
}

} // namespace

TEST(TextureAsset, WritesReadableTextureWithoutAssetPipeline) {
    TemporaryDirectory directory;
    const auto path    = directory.path() / "texture.vtex";
    const auto texture = test_texture();

    vulcal::asset::TextureAsset::write(path, texture);
    const auto loaded = vulcal::asset::TextureAsset::read(path);

    EXPECT_EQ(loaded.width_, 2u);
    EXPECT_EQ(loaded.height_, 1u);
    EXPECT_EQ(loaded.format_, vulcal::render::TextureFormat::RGBA8);
    EXPECT_EQ(loaded.pixels_, texture.pixels_);
}

TEST(TextureAsset, RejectsInvalidPixelDataBeforeOverwritingFile) {
    TemporaryDirectory directory;
    const auto path = directory.path() / "texture.vtex";
    vulcal::asset::TextureAsset::write(path, test_texture());

    auto invalid = test_texture();
    invalid.pixels_.pop_back();
    EXPECT_THROW(vulcal::asset::TextureAsset::write(path, invalid), std::invalid_argument);

    const auto loaded = vulcal::asset::TextureAsset::read(path);
    EXPECT_EQ(loaded.pixels_.size(), 8u);
}

TEST(TextureAsset, RejectsCorruptCookedTexture) {
    TemporaryDirectory directory;
    const auto path = directory.path() / "texture.vtex";
    {
        std::ofstream output(path, std::ios::binary);
        output << "invalid";
    }

    EXPECT_THROW(vulcal::asset::TextureAsset::read(path), std::runtime_error);
}

TEST(TextureAsset, RejectsTruncatedPixels) {
    TemporaryDirectory directory;
    const auto path = directory.path() / "texture.vtex";
    vulcal::asset::TextureAsset::write(path, test_texture());

    const auto original_size = std::filesystem::file_size(path);
    std::filesystem::resize_file(path, original_size - 1);

    EXPECT_THROW(vulcal::asset::TextureAsset::read(path), std::runtime_error);
}
