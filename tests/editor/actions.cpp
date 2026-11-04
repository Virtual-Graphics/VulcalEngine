#include "asset/asset_manager.h"
#include "asset/render_asset_cache.h"
#include "asset/runtime_mesh_asset.h"
#include "common/nfd/file_dialog.h"
#include "editor/editor_actions.h"
#include "editor/editor_context.h"
#include "editor/project/scene/scene_serializer.h"
#include "gfx/context.h"
#include "render/renderer.h"

#include <SDL3/SDL.h>
#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>
#include <memory>
#include <optional>

namespace {

vulcal::MeshData triangle() {
    return { .vertices_ = {
                 { { -1.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 1.0f } },
                 { { 1.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 1.0f } },
                 { { 0.0f, 1.0f, 0.0f }, { 0.0f, 0.0f, 1.0f } },
             },
             .indices_ = { 0, 1, 2 } };
}

class EditorActionsTest : public testing::Test {
protected:
    void SetUp() override {
        directory_ = std::filesystem::temp_directory_path() / ("vulcal_actions_" + base::UUID::generate().to_string());
        std::filesystem::create_directories(directory_ / "project");
        vulcal::gfx::Context::prepare_attributes();
        window_ = SDL_CreateWindow("Editor actions test", 64, 64, SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN);
        ASSERT_NE(window_, nullptr) << SDL_GetError();
        graphics_ = std::make_unique<vulcal::gfx::Context>(window_);
        renderer_ = std::make_unique<vulcal::render::Renderer>();
        cache_    = std::make_unique<vulcal::asset::RenderAssetCache>(assets_, *renderer_);
        actions_  = std::make_unique<editor::EditorActions>(
                context_, assets_, *cache_,
                editor::EditorActions::Dialogs{
                    .open_file = [this](const auto&, const auto&) { return open_path_; },
                    .save_file = [this](const auto&, const auto&, const auto&) { return save_path_; },
                });
        actions_->create_project(directory_ / "project", "Test");
    }

    void TearDown() override {
        actions_.reset();
        cache_.reset();
        renderer_.reset();
        graphics_.reset();
        if (window_) SDL_DestroyWindow(window_);
        SDL_QuitSubSystem(SDL_INIT_VIDEO);
        std::filesystem::remove_all(directory_);
    }

    void draw(vulcal::render::MeshHandle mesh) {
        vulcal::render::RenderData data;
        data.items_.push_back({ mesh, cache_->resolve_material({}), glm::mat4{ 1.0f } });
        renderer_->render(data);
    }

    void write_obj(float x) {
        std::ofstream output(*open_path_);
        output << "v " << x << " 0 0\nv 1 0 0\nv 0 1 0\nf 1 2 3\n";
    }

    std::filesystem::path directory_;
    SDL_Window* window_ = nullptr;
    std::unique_ptr<vulcal::gfx::Context> graphics_;
    std::unique_ptr<vulcal::render::Renderer> renderer_;
    vulcal::asset::AssetManager assets_;
    std::unique_ptr<vulcal::asset::RenderAssetCache> cache_;
    editor::EditorContext context_;
    std::unique_ptr<editor::EditorActions> actions_;
    std::optional<std::filesystem::path> open_path_;
    std::optional<std::filesystem::path> save_path_;
};

TEST_F(EditorActionsTest, SavesUnnamedSceneThenReusesItsPath) {
    const auto entity = context_.create_entity();
    save_path_        = directory_ / "level";
    actions_->save_scene();
    ASSERT_TRUE(std::filesystem::exists(directory_ / "level.vscene"));

    entity.get_component<vulcal::scene::TagComponent>().name_ = "Saved again";
    save_path_.reset();
    actions_->save_scene();
    const auto loaded = editor::SceneSerializer::deserialize(directory_ / "level.vscene");
    bool found        = false;
    for (const auto e: loaded->registry().view<vulcal::scene::TagComponent>()) {
        found |= loaded->registry().get<vulcal::scene::TagComponent>(e).name_ == "Saved again";
    }
    EXPECT_TRUE(found);
}

TEST_F(EditorActionsTest, CancelledSaveLeavesSceneUnnamedAndSelectionUnchanged) {
    const auto selected = context_.create_entity();
    actions_->save_scene();
    EXPECT_EQ(context_.save_scene(), editor::EditorContext::SaveResult::NeedsPath);
    EXPECT_EQ(context_.selected_entity(), selected);
    EXPECT_FALSE(std::filesystem::exists(directory_ / "Untitled.vscene"));
}

TEST_F(EditorActionsTest, CancelledProjectOpenPreservesSceneAndCaches) {
    const auto selected = context_.create_entity();
    const auto id       = assets_.add_mesh(triangle());
    const auto cpu      = assets_.load_mesh(id);
    const auto gpu      = cache_->resolve_mesh(id);
    actions_->open_project();
    EXPECT_EQ(context_.selected_entity(), selected);
    EXPECT_EQ(assets_.load_mesh(id), cpu);
    EXPECT_EQ(cache_->resolve_mesh(id), gpu);
}

TEST_F(EditorActionsTest, ProjectSwitchClearsCachesAndUsesNewRuntimeDirectory) {
    const auto old_id = assets_.add_mesh(triangle());
    const auto gpu    = cache_->resolve_mesh(old_id);
    context_.create_entity();
    editor::EditorContext other;
    std::filesystem::create_directories(directory_ / "other");
    other.create_project(directory_ / "other", "Other");
    const vulcal::asset::AssetID new_id{ 12 };
    vulcal::asset::RuntimeMeshAsset::write(other.library_directory() / "12.vmesh", triangle());
    open_path_ = directory_ / "other" / "Other.vproj";

    actions_->open_project();
    EXPECT_FALSE(context_.selected_entity());
    EXPECT_EQ(context_.library_directory(), other.library_directory());
    EXPECT_FALSE(assets_.load_mesh(old_id));
    ASSERT_TRUE(assets_.load_mesh(new_id));
    EXPECT_THROW(draw(gpu), std::invalid_argument);
    EXPECT_NO_THROW(draw(cache_->resolve_mesh(new_id)));
}

TEST_F(EditorActionsTest, FailedProjectOpenPreservesSceneAndCaches) {
    const auto selected = context_.create_entity();
    const auto id       = assets_.add_mesh(triangle());
    const auto cpu      = assets_.load_mesh(id);
    const auto gpu      = cache_->resolve_mesh(id);
    open_path_          = directory_ / "missing.vproj";
    EXPECT_THROW(actions_->open_project(), std::exception);
    EXPECT_EQ(context_.selected_entity(), selected);
    EXPECT_EQ(assets_.load_mesh(id), cpu);
    EXPECT_EQ(cache_->resolve_mesh(id), gpu);
}

TEST_F(EditorActionsTest, SceneOpenClearsSelectionWithoutResettingAssets) {
    save_path_ = directory_ / "level.vscene";
    actions_->save_scene_as();
    context_.create_entity();
    const auto id  = assets_.add_mesh(triangle());
    const auto gpu = cache_->resolve_mesh(id);
    open_path_     = save_path_;
    actions_->open_scene();
    EXPECT_FALSE(context_.selected_entity());
    EXPECT_EQ(context_.scene().registry().view<vulcal::scene::TagComponent>().size(), 3u);
    EXPECT_EQ(cache_->resolve_mesh(id), gpu);
}

TEST_F(EditorActionsTest, ReimportAssignsSelectedEntityAndRefreshesCpuAndGpuData) {
    const auto selected = context_.create_entity();
    open_path_          = context_.assets_directory() / "triangle.obj";
    write_obj(-1.0f);
    actions_->import_obj();
    ASSERT_TRUE(selected.has_component<vulcal::scene::MeshRendererComponent>());
    const auto id  = selected.get_component<vulcal::scene::MeshRendererComponent>().mesh_asset_id_;
    const auto cpu = assets_.load_mesh(id);
    ASSERT_TRUE(cpu);
    const auto gpu      = cache_->resolve_mesh(id);
    const auto material = cache_->resolve_material({});

    write_obj(-2.0f);
    actions_->import_obj();
    EXPECT_EQ(selected.get_component<vulcal::scene::MeshRendererComponent>().mesh_asset_id_, id);
    const auto reloaded = assets_.load_mesh(id);
    ASSERT_TRUE(reloaded);
    EXPECT_NE(reloaded, cpu);
    EXPECT_FLOAT_EQ(reloaded->vertices_[0].position.x, -2.0f);
    EXPECT_THROW(draw(gpu), std::invalid_argument);
    EXPECT_NE(cache_->resolve_mesh(id), gpu);
    EXPECT_EQ(cache_->resolve_material({}), material);
}

TEST_F(EditorActionsTest, ImportWithoutSelectionDoesNotCreateAnEntity) {
    const auto count = context_.scene().registry().view<vulcal::scene::TagComponent>().size();
    open_path_       = context_.assets_directory() / "triangle.obj";
    write_obj(-1.0f);
    actions_->import_obj();
    EXPECT_FALSE(context_.selected_entity());
    EXPECT_EQ(context_.scene().registry().view<vulcal::scene::TagComponent>().size(), count);
    EXPECT_TRUE(std::filesystem::exists(open_path_->string() + ".meta"));
}

TEST_F(EditorActionsTest, CancelledImportPreservesSelectedMeshAndCaches) {
    const auto selected                                                           = context_.create_entity();
    const auto id                                                                 = assets_.add_mesh(triangle());
    selected.add_component<vulcal::scene::MeshRendererComponent>().mesh_asset_id_ = id;
    const auto cpu                                                                = assets_.load_mesh(id);
    const auto gpu                                                                = cache_->resolve_mesh(id);

    actions_->import_obj();
    EXPECT_EQ(context_.selected_entity(), selected);
    EXPECT_EQ(selected.get_component<vulcal::scene::MeshRendererComponent>().mesh_asset_id_, id);
    EXPECT_EQ(assets_.load_mesh(id), cpu);
    EXPECT_EQ(cache_->resolve_mesh(id), gpu);
}

TEST_F(EditorActionsTest, FailedImportPreservesSelectedMeshAndCaches) {
    const auto selected                                                           = context_.create_entity();
    const auto id                                                                 = assets_.add_mesh(triangle());
    selected.add_component<vulcal::scene::MeshRendererComponent>().mesh_asset_id_ = id;
    const auto cpu                                                                = assets_.load_mesh(id);
    const auto gpu                                                                = cache_->resolve_mesh(id);
    open_path_                                                                    = context_.assets_directory() / "missing.obj";

    EXPECT_THROW(actions_->import_obj(), std::invalid_argument);
    EXPECT_EQ(context_.selected_entity(), selected);
    EXPECT_EQ(selected.get_component<vulcal::scene::MeshRendererComponent>().mesh_asset_id_, id);
    EXPECT_EQ(assets_.load_mesh(id), cpu);
    EXPECT_EQ(cache_->resolve_mesh(id), gpu);
}

} // namespace
