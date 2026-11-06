#include "asset/asset_manager.h"
#include "base/binary_io.h"

#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>
#include <utility>

namespace {

void write_material_file(const std::filesystem::path& path, const vulcal::render::Material& material) {
    std::ofstream output(path, std::ios::binary);
    output.write("VULMATC\0", 8);
    base::binary::write_u32_le(output, 1);
    for (float value: { material.diffuse_.x, material.diffuse_.y, material.diffuse_.z, material.specular_.x, material.specular_.y,
                        material.specular_.z, material.shininess_ }) {
        base::binary::write_float_le(output, value);
    }
}

class AssetManagerTest : public testing::Test {
protected:
    void SetUp() override {
        directory_ = std::filesystem::temp_directory_path() / ("vulcal_assets_" + base::UUID::generate().to_string());
        std::filesystem::create_directories(directory_);
    }

    void TearDown() override { std::filesystem::remove_all(directory_); }

    void write_mesh(const std::filesystem::path& directory, vulcal::asset::AssetID id, float x) {
        std::filesystem::create_directories(directory);
        std::ofstream output(directory / (id.to_string() + ".vmesh"), std::ios::binary);
        output.write("VULMESH\0", 8);
        base::binary::write_u32_le(output, 1);
        base::binary::write_u64_le(output, 1);
        base::binary::write_u64_le(output, 1);
        for (float value: { x, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f }) base::binary::write_float_le(output, value);
        base::binary::write_u32_le(output, 0);
    }

    std::filesystem::path directory_;
};

TEST_F(AssetManagerTest, ReusesLoadedDataUntilInvalidated) {
    const vulcal::asset::AssetID id{ 1 };
    write_mesh(directory_, id, 1.0f);
    vulcal::asset::AssetManager assets(directory_);
    const auto first = assets.load_mesh(id);
    ASSERT_TRUE(first);
    EXPECT_FLOAT_EQ(first->vertices_[0].position.x, 1.0f);

    write_mesh(directory_, id, 2.0f);
    EXPECT_EQ(assets.load_mesh(id), first);
    assets.invalidate(id);
    const auto second = assets.load_mesh(id);
    ASSERT_TRUE(second);
    EXPECT_NE(second, first);
    EXPECT_FLOAT_EQ(second->vertices_[0].position.x, 2.0f);
    EXPECT_FLOAT_EQ(first->vertices_[0].position.x, 1.0f);
}

TEST_F(AssetManagerTest, MissingMeshCanBeRetriedAfterInvalidation) {
    const vulcal::asset::AssetID id{ 2 };
    vulcal::asset::AssetManager assets(directory_);
    EXPECT_FALSE(assets.load_mesh(id));
    write_mesh(directory_, id, 3.0f);
    EXPECT_FALSE(assets.load_mesh(id));

    assets.invalidate(id);
    const auto mesh = assets.load_mesh(id);
    ASSERT_TRUE(mesh);
    EXPECT_FLOAT_EQ(mesh->vertices_[0].position.x, 3.0f);
}

TEST_F(AssetManagerTest, ChangingDirectoryDropsCachedDataAndFailures) {
    const vulcal::asset::AssetID loaded{ 1 }, missing{ 2 }, loaded_material{ 3 };
    const auto other = directory_ / "other";
    write_mesh(directory_, loaded, 1.0f);
    write_mesh(other, loaded, 2.0f);
    write_mesh(other, missing, 3.0f);
    vulcal::render::Material first_material;
    first_material.shininess_ = 12.0f;
    write_material_file(directory_ / (loaded_material.to_string() + ".vmatc"), first_material);
    vulcal::render::Material second_material;
    second_material.shininess_ = 24.0f;
    write_material_file(other / (loaded_material.to_string() + ".vmatc"), second_material);
    vulcal::asset::AssetManager assets(directory_);
    const auto old = assets.load_mesh(loaded);
    ASSERT_TRUE(old);
    EXPECT_FALSE(assets.load_mesh(missing));
    const auto old_material = assets.load_material(loaded_material);
    ASSERT_TRUE(old_material);
    EXPECT_FLOAT_EQ(old_material->shininess_, 12.0f);

    assets.set_directory(other);
    const auto replacement = assets.load_mesh(loaded);
    ASSERT_TRUE(replacement);
    EXPECT_FLOAT_EQ(replacement->vertices_[0].position.x, 2.0f);
    EXPECT_TRUE(assets.load_mesh(missing));
    const auto replacement_material = assets.load_material(loaded_material);
    ASSERT_TRUE(replacement_material);
    EXPECT_FLOAT_EQ(replacement_material->shininess_, 24.0f);
    EXPECT_FLOAT_EQ(old->vertices_[0].position.x, 1.0f);
    EXPECT_FLOAT_EQ(old_material->shininess_, 12.0f);
}

TEST(AssetManager, RegistersProceduralDataWithoutRenderer) {
    vulcal::asset::AssetManager assets;
    vulcal::render::MeshData mesh;
    mesh.vertices_.push_back({
        { 4.0f, 0.0f, 0.0f },
        { 0.0f, 1.0f, 0.0f }
    });
    mesh.indices_      = { 0 };
    const auto mesh_id = assets.add_mesh(std::move(mesh));
    vulcal::render::Material material;
    material.shininess_    = 17.0f;
    const auto material_id = assets.add_material(material);

    EXPECT_FALSE(mesh_id.is_nil());
    EXPECT_FALSE(material_id.is_nil());
    const auto loaded = assets.load_mesh(mesh_id);
    ASSERT_TRUE(loaded);
    EXPECT_FLOAT_EQ(loaded->vertices_[0].position.x, 4.0f);
    const auto loaded_material = assets.load_material(material_id);
    ASSERT_TRUE(loaded_material);
    EXPECT_FLOAT_EQ(loaded_material->shininess_, 17.0f);
}

TEST(AssetManager, SharesDefaultMaterialOnlyForNilId) {
    vulcal::asset::AssetManager assets;
    const auto first = assets.load_material({});
    ASSERT_TRUE(first);
    EXPECT_EQ(assets.load_material({}), first);
    EXPECT_FALSE(assets.load_material(vulcal::asset::AssetID{ 123 }));
    EXPECT_FALSE(assets.load_mesh({}));
}

TEST_F(AssetManagerTest, LoadsCompiledMaterialCachesUntilInvalidated) {
    const vulcal::asset::AssetID id{ 31 };
    const auto path = directory_ / (id.to_string() + ".vmatc");

    vulcal::render::Material material;
    material.diffuse_ = { 0.2f, 0.3f, 0.4f };
    write_material_file(path, material);

    vulcal::asset::AssetManager assets(directory_);
    const auto first = assets.load_material(id);
    ASSERT_TRUE(first);
    EXPECT_EQ(first->diffuse_, material.diffuse_);

    material.diffuse_ = { 0.7f, 0.6f, 0.5f };
    write_material_file(path, material);
    EXPECT_EQ(assets.load_material(id), first);

    assets.invalidate(id);
    const auto second = assets.load_material(id);
    ASSERT_TRUE(second);
    EXPECT_NE(second, first);
    EXPECT_EQ(second->diffuse_, material.diffuse_);
}

TEST_F(AssetManagerTest, MissingMaterialCanBeRetriedAfterInvalidation) {
    const vulcal::asset::AssetID id{ 32 };
    vulcal::asset::AssetManager assets(directory_);

    EXPECT_FALSE(assets.load_material(id));
    vulcal::render::Material material;
    material.shininess_ = 24.0f;
    write_material_file(directory_ / (id.to_string() + ".vmatc"), material);
    EXPECT_FALSE(assets.load_material(id));

    assets.invalidate(id);
    const auto loaded = assets.load_material(id);
    ASSERT_TRUE(loaded);
    EXPECT_FLOAT_EQ(loaded->shininess_, 24.0f);
}

TEST(AssetManager, InvalidatingOneAssetKeepsOthersAndClearReleasesRegisteredData) {
    vulcal::asset::AssetManager assets;
    const auto first    = assets.add_mesh({});
    const auto second   = assets.add_mesh({});
    const auto material = assets.add_material({});
    const auto retained = assets.load_mesh(second);
    assets.invalidate(first);
    EXPECT_FALSE(assets.load_mesh(first));
    EXPECT_EQ(assets.load_mesh(second), retained);
    EXPECT_TRUE(assets.load_material(material));

    assets.clear();
    EXPECT_FALSE(assets.load_mesh(second));
    EXPECT_FALSE(assets.load_material(material));
    EXPECT_TRUE(retained);
}

} // namespace
