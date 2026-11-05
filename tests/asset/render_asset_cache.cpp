#include "asset/render_asset_cache.h"
#include "asset/asset_manager.h"
#include "base/binary_io.h"
#include "gfx/context.h"
#include "render/renderer.h"
#include "scene/scene.h"
#include "scene/scene_extractor.h"

#include <SDL3/SDL.h>
#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>
#include <memory>
#include <stdexcept>

namespace {

vulcal::render::MeshData triangle() {
    vulcal::render::MeshData mesh;
    mesh.vertices_ = {
        { { -0.5f, -0.5f, 0.0f }, { 0.0f, 0.0f, 1.0f } },
        {  { 0.5f, -0.5f, 0.0f }, { 0.0f, 0.0f, 1.0f } },
        {   { 0.0f, 0.5f, 0.0f }, { 0.0f, 0.0f, 1.0f } },
    };
    mesh.indices_ = { 0, 1, 2 };
    return mesh;
}

class RenderAssets : public testing::Test {
protected:
    void SetUp() override {
        directory_ = std::filesystem::temp_directory_path() / ("vulcal_render_assets_" + base::UUID::generate().to_string());
        std::filesystem::create_directories(directory_);
        vulcal::gfx::Context::prepare_attributes();
        window_ = SDL_CreateWindow("Resource cache test", 64, 64, SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN);
        ASSERT_NE(window_, nullptr) << SDL_GetError();
        context_     = std::make_unique<vulcal::gfx::Context>(window_);
        renderer_    = std::make_unique<vulcal::render::Renderer>();
        cache_       = std::make_unique<vulcal::asset::RenderAssetCache>(assets_, *renderer_);
        mesh_id_     = assets_.add_mesh(triangle());
        material_id_ = assets_.add_material({});
    }

    void TearDown() override {
        cache_.reset();
        renderer_.reset();
        context_.reset();
        if (window_) SDL_DestroyWindow(window_);
        SDL_QuitSubSystem(SDL_INIT_VIDEO);
        std::filesystem::remove_all(directory_);
    }

    void draw(vulcal::render::MeshHandle mesh, vulcal::render::MaterialHandle material) {
        vulcal::render::RenderData data;
        data.items_.push_back({ mesh, material, glm::mat4{ 1.0f } });
        renderer_->render(data);
    }

    void write_triangle(vulcal::asset::AssetID id, float x) {
        std::ofstream output(directory_ / (id.to_string() + ".vmesh"), std::ios::binary);
        output.write("VULMESH\0", 8);
        base::binary::write_u32_le(output, 1);
        base::binary::write_u64_le(output, 3);
        base::binary::write_u64_le(output, 3);
        auto mesh                    = triangle();
        mesh.vertices_[0].position.x = x;
        for (const auto& vertex: mesh.vertices_) {
            for (float value:
                 { vertex.position.x, vertex.position.y, vertex.position.z, vertex.normal.x, vertex.normal.y, vertex.normal.z }) {
                base::binary::write_float_le(output, value);
            }
        }
        for (const auto index: mesh.indices_) base::binary::write_u32_le(output, index);
    }

    std::filesystem::path directory_;
    SDL_Window* window_ = nullptr;
    std::unique_ptr<vulcal::gfx::Context> context_;
    std::unique_ptr<vulcal::render::Renderer> renderer_;
    vulcal::asset::AssetManager assets_;
    std::unique_ptr<vulcal::asset::RenderAssetCache> cache_;
    vulcal::asset::AssetID mesh_id_;
    vulcal::asset::AssetID material_id_;
};

TEST_F(RenderAssets, ReusesHandlesAcrossFrames) {
    const auto mesh     = cache_->resolve_mesh(mesh_id_);
    const auto material = cache_->resolve_material(material_id_);
    ASSERT_NE(mesh.value_, 0u);
    ASSERT_NE(material.value_, 0u);
    EXPECT_EQ(cache_->resolve_mesh(mesh_id_), mesh);
    EXPECT_EQ(cache_->resolve_material(material_id_), material);
    EXPECT_NO_THROW(draw(mesh, material));
}

TEST_F(RenderAssets, UsesSharedDefaultOnlyForNilMaterialId) {
    const auto first = cache_->resolve_material({});
    EXPECT_NE(first.value_, 0u);
    EXPECT_EQ(cache_->resolve_material({}), first);
    EXPECT_EQ(cache_->resolve_material(vulcal::asset::AssetID{ 123 }).value_, 0u);
    EXPECT_EQ(cache_->resolve_mesh({}).value_, 0u);
}

TEST_F(RenderAssets, CpuInvalidationRemovesStaleHandles) {
    const auto mesh     = cache_->resolve_mesh(mesh_id_);
    const auto material = cache_->resolve_material(material_id_);
    assets_.invalidate(mesh_id_);
    EXPECT_EQ(cache_->resolve_mesh(mesh_id_).value_, 0u);
    EXPECT_THROW(draw(mesh, material), std::invalid_argument);
    EXPECT_EQ(cache_->resolve_material(material_id_), material);
}

TEST_F(RenderAssets, CanRebuildGpuResourcesWithoutReloadingCpuData) {
    const auto mesh     = cache_->resolve_mesh(mesh_id_);
    const auto material = cache_->resolve_material(material_id_);
    const auto cpu_mesh = assets_.load_mesh(mesh_id_);
    cache_->invalidate(mesh_id_);
    EXPECT_EQ(assets_.load_mesh(mesh_id_), cpu_mesh);
    EXPECT_THROW(draw(mesh, material), std::invalid_argument);
    const auto replacement = cache_->resolve_mesh(mesh_id_);
    EXPECT_NE(replacement, mesh);
    EXPECT_EQ(cache_->resolve_material(material_id_), material);
    EXPECT_NO_THROW(draw(replacement, material));
}

TEST_F(RenderAssets, ReimportReplacesOnlyChangedHandle) {
    assets_.set_directory(directory_);
    const vulcal::asset::AssetID id{ 7 };
    write_triangle(id, -0.5f);
    const auto material = cache_->resolve_material({});
    const auto old      = cache_->resolve_mesh(id);
    ASSERT_NE(old.value_, 0u);

    write_triangle(id, -0.25f);
    EXPECT_EQ(cache_->resolve_mesh(id), old);
    assets_.invalidate(id);
    const auto replacement = cache_->resolve_mesh(id);
    EXPECT_NE(replacement, old);
    EXPECT_NE(replacement.value_, 0u);
    EXPECT_EQ(cache_->resolve_material({}), material);
    EXPECT_THROW(draw(old, material), std::invalid_argument);
    EXPECT_NO_THROW(draw(replacement, material));
    EXPECT_FLOAT_EQ(assets_.load_mesh(id)->vertices_[0].position.x, -0.25f);
}

TEST_F(RenderAssets, SeparateRenderersUseSeparateHandlesForSharedAssets) {
    vulcal::render::Renderer other_renderer;
    vulcal::asset::RenderAssetCache other_cache(assets_, other_renderer);
    const auto mesh           = cache_->resolve_mesh(mesh_id_);
    const auto material       = cache_->resolve_material(material_id_);
    const auto other_mesh     = other_cache.resolve_mesh(mesh_id_);
    const auto other_material = other_cache.resolve_material(material_id_);
    EXPECT_NE(other_mesh, mesh);
    EXPECT_NE(other_material, material);
    other_cache.clear();
    EXPECT_NO_THROW(draw(mesh, material));
}

TEST_F(RenderAssets, ClearAndDestructionReleaseOnlyOwnedHandles) {
    const auto external_mesh     = renderer_->create_mesh(triangle());
    const auto external_material = renderer_->create_material({});
    const auto mesh              = cache_->resolve_mesh(mesh_id_);
    const auto material          = cache_->resolve_material(material_id_);
    cache_->clear();
    EXPECT_THROW(draw(mesh, external_material), std::invalid_argument);
    EXPECT_THROW(draw(external_mesh, material), std::invalid_argument);
    EXPECT_NO_THROW(draw(external_mesh, external_material));

    const auto next_mesh     = cache_->resolve_mesh(mesh_id_);
    const auto next_material = cache_->resolve_material(material_id_);
    cache_.reset();
    EXPECT_THROW(draw(next_mesh, external_material), std::invalid_argument);
    EXPECT_THROW(draw(external_mesh, next_material), std::invalid_argument);
    EXPECT_NO_THROW(draw(external_mesh, external_material));
    renderer_->destroy_mesh(external_mesh);
    renderer_->destroy_material(external_material);
}

TEST_F(RenderAssets, ExtractorUsesBoundCacheAndSkipsMissingOrInvisibleAssets) {
    vulcal::scene::Scene scene;
    auto entity                                                            = scene.create_entity("Visible");
    auto& mesh                                                             = entity.add_component<vulcal::scene::MeshRendererComponent>();
    mesh.mesh_asset_id_                                                    = mesh_id_;
    entity.get_component<vulcal::scene::TransformComponent>().translation_ = { 1.0f, 2.0f, 3.0f };
    auto hidden                                                            = scene.create_entity("Hidden");
    auto& hidden_mesh                                                      = hidden.add_component<vulcal::scene::MeshRendererComponent>();
    hidden_mesh.mesh_asset_id_                                             = mesh_id_;
    hidden_mesh.visible_                                                   = false;
    scene.create_entity("Unassigned").add_component<vulcal::scene::MeshRendererComponent>();

    vulcal::scene::SceneExtractor extractor(*cache_);
    vulcal::scene::SceneExtractor::Options options;
    options.camera_override_ = vulcal::render::CameraData{};
    const auto first         = extractor.extract(scene, options);
    ASSERT_EQ(first.items_.size(), 1u);
    EXPECT_EQ(first.items_[0].mesh_, cache_->resolve_mesh(mesh_id_));
    EXPECT_EQ(first.items_[0].material_, cache_->resolve_material({}));
    EXPECT_EQ(glm::vec3(first.items_[0].world_transform_[3]), (glm::vec3{ 1.0f, 2.0f, 3.0f }));
    EXPECT_EQ(extractor.extract(scene, options).items_[0].mesh_, first.items_[0].mesh_);

    // EnTT may move components when adding another entity with the same component.
    auto& current              = entity.get_component<vulcal::scene::MeshRendererComponent>();
    current.material_asset_id_ = vulcal::asset::AssetID{ 123 };
    EXPECT_TRUE(extractor.extract(scene, options).items_.empty());
    current.material_asset_id_ = material_id_;
    EXPECT_EQ(extractor.extract(scene, options).items_.size(), 1u);
    assets_.invalidate(mesh_id_);
    EXPECT_TRUE(extractor.extract(scene, options).items_.empty());
}

} // namespace
