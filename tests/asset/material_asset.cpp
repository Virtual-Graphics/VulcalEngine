#include "asset/material_asset.h"

#include "base/binary_io.h"
#include "base/uuid.h"

#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>
#include <limits>
#include <stdexcept>

namespace {

class MaterialAssetTest : public testing::Test {
protected:
    void SetUp() override {
        directory_ = std::filesystem::temp_directory_path() / ("vulcal_material_io_" + base::UUID::generate().to_string());
        std::filesystem::create_directories(directory_);
    }

    void TearDown() override { std::filesystem::remove_all(directory_); }

    std::filesystem::path directory_;
};

TEST_F(MaterialAssetTest, WritesReadableMaterialWithoutAssetPipeline) {
    const auto path = directory_ / "generated" / "surface.vmatc";
    vulcal::render::Material material;
    material.diffuse_   = { 0.2f, 0.3f, 0.4f };
    material.specular_  = { 0.05f, 0.06f, 0.07f };
    material.shininess_ = 32.0f;

    vulcal::asset::MaterialAsset::write(path, material);
    const auto loaded = vulcal::asset::MaterialAsset::read(path);

    EXPECT_EQ(loaded.diffuse_, material.diffuse_);
    EXPECT_EQ(loaded.specular_, material.specular_);
    EXPECT_FLOAT_EQ(loaded.shininess_, material.shininess_);
}

TEST_F(MaterialAssetTest, RejectsUnsupportedVersion) {
    const auto path = directory_ / "surface.vmatc";
    vulcal::render::Material material;
    vulcal::asset::MaterialAsset::write(path, material);
    {
        std::fstream file(path, std::ios::binary | std::ios::in | std::ios::out);
        file.seekp(8);
        base::binary::write_u32_le(file, 99);
    }

    EXPECT_THROW(vulcal::asset::MaterialAsset::read(path), std::runtime_error);
}

TEST_F(MaterialAssetTest, RejectsInvalidMaterialBeforeOverwritingFile) {
    const auto path = directory_ / "surface.vmatc";
    vulcal::render::Material material;
    material.shininess_ = 12.0f;
    vulcal::asset::MaterialAsset::write(path, material);

    auto invalid       = material;
    invalid.shininess_ = std::numeric_limits<float>::infinity();
    EXPECT_THROW(vulcal::asset::MaterialAsset::write(path, invalid), std::invalid_argument);

    const auto loaded = vulcal::asset::MaterialAsset::read(path);
    EXPECT_FLOAT_EQ(loaded.shininess_, 12.0f);
}

} // namespace
