#include "asset/asset_library.h"
#include "asset/texture_asset.h"
#include "asset_import/asset_importer.h"
#include "asset_import/importers/material_asset_importer.h"
#include "asset_import/meta_data.h"
#include "asset_import/importers/obj_asset_importer.h"
#include "asset_import/importers/texture_asset_importer.h"

#include <chrono>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>
#include <stdexcept>
#include <string>

namespace {

class TemporaryDirectory {
public:
    TemporaryDirectory() {
        const auto suffix = std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());
        path_             = std::filesystem::temp_directory_path() / ("vulcal_import_test_" + suffix);
        std::filesystem::create_directories(path_);
    }

    ~TemporaryDirectory() { std::filesystem::remove_all(path_); }

    const std::filesystem::path& path() const { return path_; }

private:
    std::filesystem::path path_;
};

void write_bmp(const std::filesystem::path& path, std::uint8_t red, std::uint8_t green, std::uint8_t blue) {
    constexpr std::int32_t width       = 2;
    constexpr std::int32_t height      = 2;
    constexpr std::uint32_t row_stride = 8;
    constexpr std::uint32_t image_size = row_stride * height;

    std::ofstream output(path, std::ios::binary);
    auto write_u16 = [&](std::uint16_t value) {
        const char bytes[2]{ static_cast<char>(value & 0xff), static_cast<char>((value >> 8) & 0xff) };
        output.write(bytes, 2);
    };
    auto write_u32 = [&](std::uint32_t value) {
        const char bytes[4]{ static_cast<char>(value & 0xff), static_cast<char>((value >> 8) & 0xff),
                             static_cast<char>((value >> 16) & 0xff), static_cast<char>((value >> 24) & 0xff) };
        output.write(bytes, 4);
    };

    output.write("BM", 2);
    write_u32(14 + 40 + image_size);
    write_u16(0);
    write_u16(0);
    write_u32(54);
    write_u32(40);
    write_u32(static_cast<std::uint32_t>(width));
    write_u32(static_cast<std::uint32_t>(height));
    write_u16(1);
    write_u16(24);
    write_u32(0);
    write_u32(image_size);
    write_u32(2835);
    write_u32(2835);
    write_u32(0);
    write_u32(0);
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            output.put(static_cast<char>(blue));
            output.put(static_cast<char>(green));
            output.put(static_cast<char>(red));
        }
        output.put('\0');
        output.put('\0');
    }
}

TEST(ObjAssetImporter, ReimportKeepsIdAndAssetLibraryLoadsCookedMesh) {
    TemporaryDirectory directory;
    const auto assets  = directory.path() / "Assets";
    const auto library = directory.path() / ".vulcal_engine";
    std::filesystem::create_directories(assets);
    const auto source = assets / "triangle.obj";
    {
        std::ofstream output(source);
        output << "v 0 0 0\nv 1 0 0\nv 0 1 0\nf 1 2 3\n";
    }

    const auto first_id  = apps::asset_import::ObjAssetImporter::import(source, assets, library);
    const auto second_id = apps::asset_import::ObjAssetImporter::import(source, assets, library);
    ASSERT_EQ(first_id, second_id);
    EXPECT_TRUE(std::filesystem::exists(source.string() + ".meta"));
    const auto metadata = apps::asset_import::read_metadata(apps::asset_import::metadata_path(source));
    EXPECT_EQ(metadata.id, first_id);
    EXPECT_EQ(metadata.format_version, apps::asset_import::metadata_format_version);

    const auto mesh = vulcal::asset::AssetLibrary{ library }.load_mesh(first_id);
    ASSERT_EQ(mesh.vertices_.size(), 3u);
    ASSERT_EQ(mesh.indices_.size(), 3u);
    EXPECT_FLOAT_EQ(mesh.vertices_[1].position.x, 1.0f);
    EXPECT_FLOAT_EQ(mesh.vertices_[2].position.y, 1.0f);
    EXPECT_EQ(metadata.type, vulcal::asset::AssetType::Mesh);
}

TEST(AssetImporter, DispatchesObjAndVmatSourcesByExtension) {
    TemporaryDirectory directory;
    const auto assets  = directory.path() / "Assets";
    const auto library = directory.path() / ".vulcal_engine";
    std::filesystem::create_directories(assets);

    const auto mesh_source = assets / "triangle.OBJ";
    {
        std::ofstream output(mesh_source);
        output << "v 0 0 0\nv 1 0 0\nv 0 1 0\nf 1 2 3\n";
    }

    const auto material_source = assets / "material.vmat";
    {
        std::ofstream output(material_source);
        output << "format_version: 1\ndiffuse: [0.2, 0.3, 0.4]\nspecular: [0.05, 0.06, 0.07]\nshininess: 8\n";
    }

    const auto mesh_id     = apps::asset_import::AssetImporter::import(mesh_source, assets, library);
    const auto material_id = apps::asset_import::AssetImporter::import(material_source, assets, library);

    EXPECT_TRUE(std::filesystem::exists(library / (mesh_id.to_string() + ".vmesh")));
    EXPECT_TRUE(std::filesystem::exists(library / (material_id.to_string() + ".vmatc")));
    EXPECT_EQ(apps::asset_import::read_metadata(apps::asset_import::metadata_path(mesh_source)).type, vulcal::asset::AssetType::Mesh);
    EXPECT_EQ(apps::asset_import::read_metadata(apps::asset_import::metadata_path(material_source)).type,
              vulcal::asset::AssetType::Material);
}

TEST(AssetImporter, DispatchesTextureSourcesByExtension) {
    TemporaryDirectory directory;
    const auto assets  = directory.path() / "Assets";
    const auto library = directory.path() / ".vulcal_engine";
    std::filesystem::create_directories(assets);

    const auto source = assets / "sprite.BMP";
    write_bmp(source, 10, 20, 30);

    const auto id = apps::asset_import::AssetImporter::import(source, assets, library);

    EXPECT_TRUE(std::filesystem::exists(library / (id.to_string() + ".vtex")));
    EXPECT_EQ(apps::asset_import::read_metadata(apps::asset_import::metadata_path(source)).type, vulcal::asset::AssetType::Texture);
}

TEST(TextureAssetImporter, ReimportKeepsIdAndAssetLibraryLoadsCookedTexture) {
    TemporaryDirectory directory;
    const auto assets  = directory.path() / "Assets";
    const auto library = directory.path() / ".vulcal_engine";
    std::filesystem::create_directories(assets);
    const auto source = assets / "sprite.bmp";
    write_bmp(source, 10, 20, 30);

    const auto first_id = apps::asset_import::TextureAssetImporter::import(source, assets, library);
    write_bmp(source, 40, 50, 60);
    const auto second_id = apps::asset_import::TextureAssetImporter::import(source, assets, library);
    ASSERT_EQ(first_id, second_id);

    const auto metadata = apps::asset_import::read_metadata(apps::asset_import::metadata_path(source));
    EXPECT_EQ(metadata.id, first_id);
    EXPECT_EQ(metadata.type, vulcal::asset::AssetType::Texture);
    EXPECT_TRUE(std::filesystem::exists(library / (first_id.to_string() + ".vtex")));

    const auto texture = vulcal::asset::AssetLibrary{ library }.load_texture(first_id);
    EXPECT_EQ(texture.width_, 2u);
    EXPECT_EQ(texture.height_, 2u);
    EXPECT_EQ(texture.format_, vulcal::render::TextureFormat::RGBA8);
    ASSERT_EQ(texture.pixels_.size(), 16u);
    EXPECT_EQ(texture.pixels_[0], 40u);
    EXPECT_EQ(texture.pixels_[1], 50u);
    EXPECT_EQ(texture.pixels_[2], 60u);
    EXPECT_EQ(texture.pixels_[3], 255u);
}

TEST(AssetImporter, RejectsSourcesOutsideAssetsAndUnsupportedExtensions) {
    TemporaryDirectory directory;
    const auto assets  = directory.path() / "Assets";
    const auto library = directory.path() / ".vulcal_engine";
    std::filesystem::create_directories(assets);

    const auto outside_source = directory.path() / "outside.obj";
    {
        std::ofstream output(outside_source);
        output << "v 0 0 0\nv 1 0 0\nv 0 1 0\nf 1 2 3\n";
    }
    const auto unsupported_source = assets / "notes.txt";
    {
        std::ofstream output(unsupported_source);
        output << "not an importable asset\n";
    }

    EXPECT_THROW(apps::asset_import::AssetImporter::import(outside_source, assets, library), std::invalid_argument);
    EXPECT_THROW(apps::asset_import::AssetImporter::import(unsupported_source, assets, library), std::invalid_argument);
}

TEST(MaterialAssetImporter, ReimportKeepsIdAndAssetLibraryLoadsCookedMaterial) {
    TemporaryDirectory directory;
    const auto assets  = directory.path() / "Assets";
    const auto library = directory.path() / ".vulcal_engine";
    std::filesystem::create_directories(assets);
    const auto source = assets / "Materials" / "test.vmat";
    std::filesystem::create_directories(source.parent_path());

    auto write_source = [&](float diffuse, float shininess) {
        std::ofstream output(source);
        output << "format_version: 1\n"
                  "diffuse: ["
               << diffuse
               << ", 0.3, 0.4]\n"
                  "specular: [0.05, 0.06, 0.07]\n"
                  "shininess: "
               << shininess << "\n";
    };

    write_source(0.2f, 8.0f);
    const auto first_id = apps::asset_import::MaterialAssetImporter::import(source, assets, library);
    write_source(0.7f, 32.0f);
    const auto second_id = apps::asset_import::MaterialAssetImporter::import(source, assets, library);
    ASSERT_EQ(first_id, second_id);

    const auto metadata = apps::asset_import::read_metadata(apps::asset_import::metadata_path(source));
    EXPECT_EQ(metadata.id, first_id);
    EXPECT_EQ(metadata.type, vulcal::asset::AssetType::Material);
    EXPECT_TRUE(std::filesystem::exists(library / (first_id.to_string() + ".vmatc")));

    const auto material = vulcal::asset::AssetLibrary{ library }.load_material(first_id);
    EXPECT_FLOAT_EQ(material.diffuse_.x, 0.7f);
    EXPECT_FLOAT_EQ(material.diffuse_.y, 0.3f);
    EXPECT_FLOAT_EQ(material.diffuse_.z, 0.4f);
    EXPECT_FLOAT_EQ(material.specular_.x, 0.05f);
    EXPECT_FLOAT_EQ(material.shininess_, 32.0f);
}

TEST(MaterialAssetImporter, RejectsSourceWhoseMetadataDeclaresAnotherType) {
    TemporaryDirectory directory;
    const auto assets  = directory.path() / "Assets";
    const auto library = directory.path() / ".vulcal_engine";
    std::filesystem::create_directories(assets);
    const auto source = assets / "mismatch.vmat";
    {
        std::ofstream output(source);
        output << "format_version: 1\ndiffuse: [0.9, 0.9, 0.9]\nspecular: [0.08, 0.08, 0.08]\nshininess: 8\n";
    }
    apps::asset_import::write_metadata(apps::asset_import::metadata_path(source),
                                       { .id = vulcal::asset::AssetID{ 99 }, .type = vulcal::asset::AssetType::Mesh });

    EXPECT_THROW(apps::asset_import::MaterialAssetImporter::import(source, assets, library), std::invalid_argument);
    EXPECT_FALSE(std::filesystem::exists(library / "99.vmatc"));
}

TEST(AssetMetadata, RejectsLegacyMetadataWithoutType) {
    TemporaryDirectory directory;
    const auto path = directory.path() / "legacy.obj.meta";
    {
        std::ofstream output(path);
        output << "format_version: 1\nid: 7\n";
    }

    EXPECT_THROW(apps::asset_import::read_metadata(path), std::runtime_error);
}

TEST(AssetMetadata, RejectsUnsupportedFormatVersion) {
    TemporaryDirectory directory;
    const auto path = directory.path() / "future.obj.meta";
    {
        std::ofstream output(path);
        output << "format_version: 2\nid: 7\ntype: mesh\n";
    }

    EXPECT_THROW(apps::asset_import::read_metadata(path), std::runtime_error);
}

TEST(AssetLibrary, RejectsCorruptCookedMesh) {
    TemporaryDirectory directory;
    const base::UUID id{ 1311768467463790320ull };
    const auto path = directory.path() / (id.to_string() + ".vmesh");
    {
        std::ofstream output(path, std::ios::binary);
        output << "invalid";
    }

    EXPECT_THROW(vulcal::asset::AssetLibrary{ directory.path() }.load_mesh(id), std::runtime_error);
}

} // namespace
