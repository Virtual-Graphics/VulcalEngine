#include "asset/res/mesh_asset.h"
#include "asset/asset_manager.h"

#include <filesystem>
#include <gtest/gtest.h>
#include <stdexcept>

namespace {

class MeshAssetTest : public testing::Test {
protected:
    void SetUp() override {
        directory_ = std::filesystem::temp_directory_path() / ("vulcal_mesh_io_" + base::UUID::generate().to_string());
        std::filesystem::create_directories(directory_);
    }

    void TearDown() override { std::filesystem::remove_all(directory_); }

    vulcal::render::MeshData triangle() const {
        vulcal::render::MeshData mesh;
        mesh.vertices_ = {
            { { -1.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 1.0f } },
            {  { 1.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 1.0f } },
            {  { 0.0f, 1.0f, 0.0f }, { 0.0f, 0.0f, 1.0f } },
        };
        mesh.indices_ = { 0, 2, 1 };
        return mesh;
    }

    std::filesystem::path directory_;
};

TEST_F(MeshAssetTest, WritesReadableMeshWithoutAssetPipeline) {
    const auto path = directory_ / "generated" / "triangle.vmesh";
    vulcal::asset::MeshAsset::write(path, triangle());

    const auto mesh = vulcal::asset::MeshAsset::read(path);
    ASSERT_EQ(mesh.vertices_.size(), 3u);
    EXPECT_EQ(mesh.vertices_[0].position, (glm::vec3{ -1.0f, 0.0f, 0.0f }));
    EXPECT_EQ(mesh.vertices_[1].position, (glm::vec3{ 1.0f, 0.0f, 0.0f }));
    EXPECT_EQ(mesh.vertices_[2].position, (glm::vec3{ 0.0f, 1.0f, 0.0f }));
    for (const auto& vertex: mesh.vertices_) EXPECT_EQ(vertex.normal, (glm::vec3{ 0.0f, 0.0f, 1.0f }));
    EXPECT_EQ(mesh.indices_, (std::vector<std::uint32_t>{ 0, 2, 1 }));
}

TEST_F(MeshAssetTest, RewritesEditedMeshAndReloadsAfterInvalidation) {
    const vulcal::asset::AssetID id{ 7 };
    const auto path = directory_ / (id.to_string() + ".vmesh");
    vulcal::asset::MeshAsset::write(path, triangle());
    vulcal::asset::AssetManager assets(directory_);
    const auto original = assets.load_mesh(id);
    ASSERT_TRUE(original);

    auto edited = vulcal::asset::MeshAsset::read(path);
    edited.vertices_.resize(1);
    edited.vertices_[0].position = { 2.0f, 3.0f, 4.0f };
    edited.vertices_[0].normal   = { 0.0f, 1.0f, 0.0f };
    edited.indices_              = { 0 };
    vulcal::asset::MeshAsset::write(path, edited);
    EXPECT_EQ(assets.load_mesh(id), original);

    assets.invalidate(id);
    const auto reloaded = assets.load_mesh(id);
    ASSERT_TRUE(reloaded);
    ASSERT_EQ(reloaded->vertices_.size(), 1u);
    EXPECT_EQ(reloaded->vertices_[0].position, (glm::vec3{ 2.0f, 3.0f, 4.0f }));
    EXPECT_EQ(reloaded->vertices_[0].normal, (glm::vec3{ 0.0f, 1.0f, 0.0f }));
    EXPECT_EQ(reloaded->indices_, (std::vector<std::uint32_t>{ 0 }));
    EXPECT_EQ(original->vertices_.size(), 3u);
}

TEST_F(MeshAssetTest, RejectsInvalidIndicesBeforeOverwritingFile) {
    const auto path = directory_ / "triangle.vmesh";
    vulcal::asset::MeshAsset::write(path, triangle());
    auto invalid        = triangle();
    invalid.indices_[0] = 3;

    EXPECT_THROW(vulcal::asset::MeshAsset::write(path, invalid), std::invalid_argument);
    const auto mesh = vulcal::asset::MeshAsset::read(path);
    ASSERT_EQ(mesh.vertices_.size(), 3u);
    EXPECT_EQ(mesh.indices_, (std::vector<std::uint32_t>{ 0, 2, 1 }));
}

} // namespace
