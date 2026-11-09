#include "asset/asset_manager.h"
#include "asset/render_asset_cache.h"
#include "base/window.h"
#include "editor/project/scene/scene_serializer.h"
#include "gfx/context.h"
#include "gfx/render_target.h"
#include "render/renderer.h"
#include "scene/components.h"
#include "scene/scene.h"
#include "scene/scene_extractor.h"
#include "script/script_runtime.h"

#include <SDL3/SDL.h>
#include <array>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <glad/glad.h>
#include <gtest/gtest.h>
#include <memory>
#include <string>
#include <vector>

namespace {

class ArenaGraphics : public testing::Test {
protected:
    void SetUp() override {
        vulcal::gfx::Context::prepare_attributes();
        window_   = std::make_unique<base::Window>(base::WindowProps{ .title = "Arena graphics test", .width = 960, .height = 720 });
        graphics_ = std::make_unique<vulcal::gfx::Context>(window_->handle());
        renderer_ = std::make_unique<vulcal::render::Renderer>();
        cache_    = std::make_unique<vulcal::asset::RenderAssetCache>(assets_, *renderer_);
        target_   = std::make_unique<vulcal::gfx::RenderTarget>(vulcal::gfx::RenderTarget::Spec{ .width = 960, .height = 720 });
        const auto project = std::filesystem::path{ VULCAL_TEST_SOURCE_DIR } / "projects" / "arena";
        scene_             = editor::SceneSerializer::deserialize(project / "assets" / "scenes" / "arena.vscene");
        ASSERT_TRUE(scene_);
        SDL_RaiseWindow(window_->handle());
        SDL_SyncWindow(window_->handle());
        window_->poll_events();
        SDL_Event focus{};
        focus.type            = SDL_EVENT_WINDOW_FOCUS_GAINED;
        focus.window.windowID = SDL_GetWindowID(window_->handle());
        ASSERT_TRUE(SDL_PushEvent(&focus));
        window_->poll_events();
        runtime_.start(*scene_, project);
    }

    void TearDown() override {
        runtime_.stop();
        scene_.reset();
        target_.reset();
        cache_.reset();
        renderer_.reset();
        graphics_.reset();
        window_.reset();
    }

    vulcal::scene::Entity find(const std::string& name) {
        for (const auto handle: scene_->registry().view<const vulcal::scene::TagComponent>()) {
            if (scene_->registry().get<const vulcal::scene::TagComponent>(handle).name_ == name) {
                return vulcal::scene::Entity{ handle, *scene_ };
            }
        }
        return {};
    }

    void key(SDL_Scancode code, bool down) {
        SDL_Event event{};
        event.type         = down ? SDL_EVENT_KEY_DOWN : SDL_EVENT_KEY_UP;
        event.key.windowID = SDL_GetWindowID(window_->handle());
        event.key.scancode = code;
        event.key.down     = down;
        ASSERT_TRUE(SDL_PushEvent(&event));
    }

    void frame(float dt = 1.0f / 60.0f) {
        window_->poll_events();
        runtime_.update(dt);
    }

    void start() {
        key(SDL_SCANCODE_RETURN, true);
        frame(0);
        key(SDL_SCANCODE_RETURN, false);
        frame(0);
    }

    void render(const char* name) {
        vulcal::scene::SceneExtractor extractor{ *cache_ };
        renderer_->render(extractor.extract(*scene_, 4.0f / 3.0f), target_.get());
        target_->bind();
        std::vector<unsigned char> pixels(960 * 720 * 4);
        glReadPixels(0, 0, 960, 720, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
        EXPECT_EQ(glGetError(), static_cast<GLenum>(GL_NO_ERROR));
        // A life block outside the field proves the HUD is rendered and not clipped.
        const int x      = static_cast<int>((-15.2f + 26.0f * (4.0f / 3.0f) / 2.0f) / (26.0f * (4.0f / 3.0f)) * 960);
        const int y      = static_cast<int>((5.5f + 13.0f) / 26.0f * 720);
        const auto pixel = static_cast<std::size_t>((y * 960 + x) * 4);
        EXPECT_GT(pixels[pixel + 1], pixels[pixel]);
        EXPECT_GT(pixels[pixel + 2], pixels[pixel]);
        vulcal::gfx::RenderTarget::unbind();
        if (const auto* directory = std::getenv("VULCAL_ARENA_CAPTURE_DIR")) {
            std::filesystem::create_directories(directory);
            std::ofstream output(std::filesystem::path{ directory } / (std::string{ name } + ".ppm"), std::ios::binary);
            output << "P6\n960 720\n255\n";
            for (int row = 719; row >= 0; --row) {
                for (int column = 0; column < 960; ++column) {
                    output.write(reinterpret_cast<const char*>(&pixels[(row * 960 + column) * 4]), 3);
                }
            }
            ASSERT_TRUE(output.good());
        }
    }

    vulcal::asset::AssetManager assets_;
    std::unique_ptr<base::Window> window_;
    std::unique_ptr<vulcal::gfx::Context> graphics_;
    std::unique_ptr<vulcal::render::Renderer> renderer_;
    std::unique_ptr<vulcal::asset::RenderAssetCache> cache_;
    std::unique_ptr<vulcal::gfx::RenderTarget> target_;
    std::unique_ptr<vulcal::scene::Scene> scene_;
    vulcal::script::ScriptRuntime runtime_;
};

TEST_F(ArenaGraphics, RealInputRenderingCombatLossAndRestart) {
    const auto initial_count = scene_->registry().view<vulcal::scene::TagComponent>().size();
    ASSERT_TRUE(find("Player"));
    render("waiting");
    start();
    EXPECT_FALSE(find("PromptPanel").get_component<vulcal::scene::SpriteRendererComponent>().visible_);
    key(SDL_SCANCODE_D, true);
    key(SDL_SCANCODE_SPACE, true);
    frame();
    key(SDL_SCANCODE_SPACE, false);
    for (int i = 0; i < 90; ++i) frame();
    EXPECT_GT(find("Player").get_component<vulcal::scene::TransformComponent>().translation_.x, 5.0f);
    EXPECT_TRUE(find("Enemy"));
    key(SDL_SCANCODE_UP, true);
    frame();
    EXPECT_TRUE(find("Bullet"));
    render("playing");
    key(SDL_SCANCODE_D, false);
    key(SDL_SCANCODE_UP, false);
    for (int i = 0; i < 60 * 20; ++i) frame();
    EXPECT_TRUE(find("PromptPanel").get_component<vulcal::scene::SpriteRendererComponent>().visible_);
    const auto tint = find("Player").get_component<vulcal::scene::SpriteRendererComponent>().color_;
    EXPECT_GT(tint.r, tint.g);
    render("lost");
    start();
    EXPECT_FALSE(find("Enemy"));
    EXPECT_FALSE(find("Bullet"));
    EXPECT_FALSE(find("Particle"));
    EXPECT_EQ(scene_->registry().view<vulcal::scene::TagComponent>().size(), initial_count);
    EXPECT_EQ(find("Player").get_component<vulcal::scene::TransformComponent>().translation_, glm::vec3(0.0f));
    render("restarted");
}

} // namespace
