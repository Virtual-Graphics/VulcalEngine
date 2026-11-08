#include "asset/asset_manager.h"
#include "asset/mesh_asset.h"
#include "asset/render_asset_cache.h"
#include "common/nfd/file_dialog.h"
#include "editor/editor_actions.h"
#include "editor/editor_context.h"
#include "editor/project/scene/scene_serializer.h"
#include "gfx/context.h"
#include "render/renderer.h"
#include "scene/components.h"

#include <SDL3/SDL.h>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>
#include <memory>
#include <optional>
#include <stdexcept>

namespace {

vulcal::render::MeshData triangle() {
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
                    .open_file =
                            [this](const auto& filter, const auto& directory) {
                                last_filter_    = filter.extension;
                                last_directory_ = directory;
                                return open_path_;
                            },
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

    void draw(vulcal::render::MeshHandle mesh, vulcal::render::MaterialHandle material) {
        vulcal::render::RenderData data;
        data.items_.push_back({ mesh, material, glm::mat4{ 1.0f } });
        renderer_->render(data);
    }

    void write_obj(float x) {
        std::ofstream output(*open_path_);
        output << "v " << x << " 0 0\nv 1 0 0\nv 0 1 0\nf 1 2 3\n";
    }

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
    std::string last_filter_;
    std::filesystem::path last_directory_;
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

TEST_F(EditorActionsTest, PlayingRejectsActionsBeforeOpeningDialogsOrChangingCaches) {
    const auto entity = context_.create_entity();
    entity.add_component<vulcal::scene::MeshRendererComponent>();
    entity.add_component<vulcal::scene::ScriptComponent>();
    const auto id = assets_.add_mesh(triangle());
    const auto cpu = assets_.load_mesh(id);
    const auto gpu = cache_->resolve_mesh(id);
    context_.begin_play();
    EXPECT_THROW(actions_->open_project(), std::logic_error);
    EXPECT_THROW(actions_->open_scene(), std::logic_error);
    EXPECT_THROW(actions_->save_scene_as(), std::logic_error);
    EXPECT_THROW(actions_->save_scene(), std::logic_error);
    EXPECT_THROW(actions_->new_scene(), std::logic_error);
    EXPECT_THROW(actions_->create_project(directory_ / "blocked", "Blocked"), std::logic_error);
    EXPECT_THROW(actions_->assign_script(), std::logic_error);
    EXPECT_THROW(actions_->import_mesh_asset(), std::logic_error);
    EXPECT_THROW(actions_->import_material_asset(), std::logic_error);
    EXPECT_THROW(actions_->import_texture_asset(), std::logic_error);
    EXPECT_TRUE(last_filter_.empty());
    EXPECT_EQ(assets_.load_mesh(id), cpu);
    EXPECT_EQ(cache_->resolve_mesh(id), gpu);
    EXPECT_FALSE(std::filesystem::exists(directory_ / "blocked"));
    context_.end_play();
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
    vulcal::asset::MeshAsset::write(other.library_directory() / "12.vmesh", triangle());
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

TEST_F(EditorActionsTest, ReimportRefreshesSelectedEntityCpuAndGpuData) {
    const auto target = context_.create_entity();
    target.add_component<vulcal::scene::MeshRendererComponent>();
    open_path_ = context_.assets_directory() / "triangle.obj";
    write_obj(-1.0f);
    actions_->import_mesh_asset();
    const auto id = target.get_component<vulcal::scene::MeshRendererComponent>().mesh_asset_id_;
    EXPECT_EQ(last_filter_, "obj");
    EXPECT_EQ(last_directory_, context_.assets_directory());
    EXPECT_EQ(context_.selected_entity(), target);
    const auto cpu = assets_.load_mesh(id);
    ASSERT_TRUE(cpu);
    const auto gpu      = cache_->resolve_mesh(id);
    const auto material = cache_->resolve_material({});

    write_obj(-2.0f);
    actions_->import_mesh_asset();
    EXPECT_EQ(target.get_component<vulcal::scene::MeshRendererComponent>().mesh_asset_id_, id);
    const auto reloaded = assets_.load_mesh(id);
    ASSERT_TRUE(reloaded);
    EXPECT_NE(reloaded, cpu);
    EXPECT_FLOAT_EQ(reloaded->vertices_[0].position.x, -2.0f);
    EXPECT_THROW(draw(gpu), std::invalid_argument);
    EXPECT_NE(cache_->resolve_mesh(id), gpu);
    EXPECT_EQ(cache_->resolve_material({}), material);
}

TEST_F(EditorActionsTest, ImportTextureRefreshesSelectedEntityTextureCache) {
    const auto target = context_.create_entity();
    target.add_component<vulcal::scene::SpriteRendererComponent>();
    open_path_ = context_.assets_directory() / "sprite.bmp";
    write_bmp(*open_path_, 10, 20, 30);

    actions_->import_texture_asset();
    const auto id = target.get_component<vulcal::scene::SpriteRendererComponent>().texture_asset_id_;
    EXPECT_EQ(last_filter_, "png,jpg,jpeg,bmp,tga");
    EXPECT_EQ(last_directory_, context_.assets_directory());
    EXPECT_EQ(context_.selected_entity(), target);
    const auto cpu = assets_.load_texture(id);
    ASSERT_TRUE(cpu);
    ASSERT_EQ(cpu->pixels_.size(), 16u);
    EXPECT_EQ(cpu->pixels_[0], 10u);
    EXPECT_EQ(cpu->pixels_[1], 20u);
    EXPECT_EQ(cpu->pixels_[2], 30u);
    const auto gpu = cache_->resolve_texture(id);
    ASSERT_NE(gpu.value_, 0u);

    write_bmp(*open_path_, 40, 50, 60);
    actions_->import_texture_asset();
    EXPECT_EQ(target.get_component<vulcal::scene::SpriteRendererComponent>().texture_asset_id_, id);
    const auto reloaded = assets_.load_texture(id);
    ASSERT_TRUE(reloaded);
    EXPECT_NE(reloaded, cpu);
    EXPECT_EQ(reloaded->pixels_[0], 40u);
    EXPECT_NE(cache_->resolve_texture(id), gpu);
}

TEST_F(EditorActionsTest, ImportTextureRequiresSelectedSpriteRenderer) {
    auto target = context_.create_entity();
    target.add_component<vulcal::scene::SpriteRendererComponent>();
    context_.create_entity();

    EXPECT_THROW(actions_->import_texture_asset(), std::invalid_argument);
    context_.select(target);
    actions_->import_texture_asset();
    EXPECT_TRUE(target.get_component<vulcal::scene::SpriteRendererComponent>().texture_asset_id_.is_nil());
}

TEST_F(EditorActionsTest, ImportMaterialRefreshesSelectedEntityMaterialCache) {
    const auto target = context_.create_entity();
    target.add_component<vulcal::scene::MeshRendererComponent>();
    open_path_ = context_.assets_directory() / "surface.vmat";
    {
        std::ofstream output(*open_path_);
        output << "format_version: 1\ndiffuse: [0.2, 0.3, 0.4]\nspecular: [0.05, 0.06, 0.07]\nshininess: 16\n";
    }

    actions_->import_material_asset();
    const auto id = target.get_component<vulcal::scene::MeshRendererComponent>().material_asset_id_;
    EXPECT_EQ(last_filter_, "vmat");
    EXPECT_EQ(last_directory_, context_.assets_directory());
    EXPECT_EQ(context_.selected_entity(), target);
    const auto cpu = assets_.load_material(id);
    ASSERT_TRUE(cpu);
    EXPECT_FLOAT_EQ(cpu->shininess_, 16.0f);
    const auto mesh = cache_->resolve_mesh(assets_.add_mesh(triangle()));
    const auto gpu  = cache_->resolve_material(id);
    EXPECT_NO_THROW(draw(mesh, gpu));

    {
        std::ofstream output(*open_path_);
        output << "format_version: 1\ndiffuse: [0.8, 0.7, 0.6]\nspecular: [0.05, 0.06, 0.07]\nshininess: 32\n";
    }
    actions_->import_material_asset();
    EXPECT_EQ(target.get_component<vulcal::scene::MeshRendererComponent>().material_asset_id_, id);
    const auto reloaded = assets_.load_material(id);
    ASSERT_TRUE(reloaded);
    EXPECT_NE(reloaded, cpu);
    EXPECT_FLOAT_EQ(reloaded->shininess_, 32.0f);
    EXPECT_THROW(draw(mesh, gpu), std::invalid_argument);
    const auto replacement = cache_->resolve_material(id);
    EXPECT_NE(replacement, gpu);
    EXPECT_NO_THROW(draw(mesh, replacement));
}

TEST_F(EditorActionsTest, AssignsProjectRelativeScriptPathFromAssetsPicker) {
    const auto target = context_.create_entity();
    target.add_component<vulcal::scene::ScriptComponent>();
    open_path_ = context_.assets_directory() / "Scripts" / "player.lua";
    std::filesystem::create_directories(open_path_->parent_path());
    std::ofstream(*open_path_) << "return {}\n";

    actions_->assign_script();

    EXPECT_EQ(last_filter_, "lua");
    EXPECT_EQ(last_directory_, context_.assets_directory());
    EXPECT_EQ(target.get_component<vulcal::scene::ScriptComponent>().script_path_, "assets/Scripts/player.lua");
}

TEST_F(EditorActionsTest, RejectsScriptSelectionOutsideAssets) {
    const auto target                                                    = context_.create_entity();
    target.add_component<vulcal::scene::ScriptComponent>().script_path_ = "Assets/Scripts/old.lua";
    open_path_                                                           = directory_ / "outside.lua";
    std::ofstream(*open_path_) << "return {}\n";

    EXPECT_THROW(actions_->assign_script(), std::invalid_argument);
    EXPECT_EQ(target.get_component<vulcal::scene::ScriptComponent>().script_path_, "Assets/Scripts/old.lua");
}

TEST_F(EditorActionsTest, RejectsScriptSymlinkThatResolvesOutsideAssets) {
    const auto target = context_.create_entity();
    target.add_component<vulcal::scene::ScriptComponent>();
    const auto outside = directory_ / "outside.lua";
    std::ofstream(outside) << "return {}\n";
    const auto alias = context_.assets_directory() / "escape.lua";
    std::error_code error;
    std::filesystem::create_symlink(outside, alias, error);
    if (error) GTEST_SKIP() << "symlink creation unavailable: " << error.message();
    open_path_ = alias;

    EXPECT_THROW(actions_->assign_script(), std::invalid_argument);
    EXPECT_TRUE(target.get_component<vulcal::scene::ScriptComponent>().script_path_.empty());
}

TEST_F(EditorActionsTest, ImportRequiresSelectedMeshRenderer) {
    const auto target = context_.create_entity();
    target.add_component<vulcal::scene::MeshRendererComponent>();
    context_.create_entity();

    EXPECT_THROW(actions_->import_mesh_asset(), std::invalid_argument);
    EXPECT_THROW(actions_->import_material_asset(), std::invalid_argument);
    EXPECT_THROW(actions_->import_texture_asset(), std::invalid_argument);
    EXPECT_TRUE(target.get_component<vulcal::scene::MeshRendererComponent>().mesh_asset_id_.is_nil());

    context_.select(vulcal::scene::Entity{});
    EXPECT_THROW(actions_->import_mesh_asset(), std::invalid_argument);
    EXPECT_THROW(actions_->import_material_asset(), std::invalid_argument);
    EXPECT_THROW(actions_->import_texture_asset(), std::invalid_argument);
}

TEST_F(EditorActionsTest, CancelledImportPreservesSelectedMeshAndCaches) {
    const auto target = context_.create_entity();
    target.add_component<vulcal::scene::MeshRendererComponent>();
    const auto id                                                               = assets_.add_mesh(triangle());
    target.get_component<vulcal::scene::MeshRendererComponent>().mesh_asset_id_ = id;
    const auto cpu                                                              = assets_.load_mesh(id);
    const auto gpu                                                              = cache_->resolve_mesh(id);

    actions_->import_mesh_asset();
    EXPECT_EQ(context_.selected_entity(), target);
    EXPECT_EQ(target.get_component<vulcal::scene::MeshRendererComponent>().mesh_asset_id_, id);
    EXPECT_EQ(assets_.load_mesh(id), cpu);
    EXPECT_EQ(cache_->resolve_mesh(id), gpu);
}

TEST_F(EditorActionsTest, FailedImportPreservesSelectedMeshAndCaches) {
    const auto target = context_.create_entity();
    target.add_component<vulcal::scene::MeshRendererComponent>();
    const auto id                                                               = assets_.add_mesh(triangle());
    target.get_component<vulcal::scene::MeshRendererComponent>().mesh_asset_id_ = id;
    const auto cpu                                                              = assets_.load_mesh(id);
    const auto gpu                                                              = cache_->resolve_mesh(id);
    open_path_                                                                  = context_.assets_directory() / "missing.obj";

    EXPECT_THROW(actions_->import_mesh_asset(), std::invalid_argument);
    EXPECT_EQ(context_.selected_entity(), target);
    EXPECT_EQ(target.get_component<vulcal::scene::MeshRendererComponent>().mesh_asset_id_, id);
    EXPECT_EQ(assets_.load_mesh(id), cpu);
    EXPECT_EQ(cache_->resolve_mesh(id), gpu);
}

TEST_F(EditorActionsTest, GuardedOperationRecordsTheLatestError) {
    actions_->run_guarded([] { throw std::runtime_error("first"); });
    actions_->run_guarded([] { throw std::runtime_error("second"); });
    const auto error = actions_->take_error();
    ASSERT_TRUE(error);
    EXPECT_EQ(*error, "second");
    EXPECT_FALSE(actions_->take_error());
}

} // namespace
