#include "base/input.h"
#include "base/window.h"
#include "editor/panels/viewport_panel.h"
#include "gfx/context.h"

#include <SDL3/SDL.h>
#include <glad/glad.h>
#include <gtest/gtest.h>
#include <imgui.h>
#include <memory>

namespace {

class ViewportTest : public testing::Test {
protected:
    void SetUp() override {
        vulcal::gfx::Context::prepare_attributes();
        window_   = std::make_unique<base::Window>(base::WindowProps{ .title = "Viewport test", .width = 640, .height = 480 });
        graphics_ = std::make_unique<vulcal::gfx::Context>(window_->handle());
        ImGui::CreateContext();
        auto& io                        = ImGui::GetIO();
        io.IniFilename                  = nullptr;
        io.DisplaySize                  = { 640.0f, 480.0f };
        io.DeltaTime                    = 1.0f / 60.0f;
        io.ConfigInputTrickleEventQueue = false;
        unsigned char* pixels           = nullptr;
        int width = 0, height = 0;
        io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);
        io.Fonts->SetTexID(ImTextureID{ 1 });
        SDL_RaiseWindow(window_->handle());
        SDL_SyncWindow(window_->handle());
        window_->poll_events();
        SDL_Event focus{};
        focus.type            = SDL_EVENT_WINDOW_FOCUS_GAINED;
        focus.window.windowID = SDL_GetWindowID(window_->handle());
        SDL_PushEvent(&focus);
        ImGui::GetIO().AddMousePosEvent(100.0f, 100.0f);
        frame();
        frame();
    }

    void TearDown() override {
        viewport_.release_resources();
        ImGui::DestroyContext();
        graphics_.reset();
        window_.reset();
    }

    void frame(bool popup = false) {
        window_->poll_events();
        ImGui::NewFrame();
        ImGui::SetNextWindowPos({ 0.0f, 0.0f });
        ImGui::SetNextWindowSize({ 600.0f, 440.0f });
        viewport_.draw(open_, graphics_->limits());
        if (popup) {
            ImGui::OpenPopup("Test popup");
            if (ImGui::BeginPopupModal("Test popup")) ImGui::EndPopup();
        }
        viewport_.update_camera(1.0f / 60.0f, window_->handle());
        ImGui::Render();
    }

    void right_button(bool down) {
        SDL_Event event{};
        event.type            = down ? SDL_EVENT_MOUSE_BUTTON_DOWN : SDL_EVENT_MOUSE_BUTTON_UP;
        event.button.windowID = SDL_GetWindowID(window_->handle());
        event.button.button   = SDL_BUTTON_RIGHT;
        event.button.down     = down;
        SDL_PushEvent(&event);
        ImGui::GetIO().AddMouseButtonEvent(ImGuiMouseButton_Right, down);
    }

    void key(SDL_Scancode code, bool down) {
        SDL_Event event{};
        event.type         = down ? SDL_EVENT_KEY_DOWN : SDL_EVENT_KEY_UP;
        event.key.windowID = SDL_GetWindowID(window_->handle());
        event.key.scancode = code;
        event.key.down     = down;
        SDL_PushEvent(&event);
    }

    void capture() {
        ASSERT_NE(SDL_GetWindowFlags(window_->handle()) & SDL_WINDOW_INPUT_FOCUS, 0u);
        right_button(true);
        frame();
        ASSERT_TRUE(SDL_GetWindowRelativeMouseMode(window_->handle()));
        EXPECT_NE(ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_NoMouseCursorChange, 0);
    }

    std::unique_ptr<base::Window> window_;
    std::unique_ptr<vulcal::gfx::Context> graphics_;
    editor::ViewportPanel viewport_;
    bool open_ = true;
};

TEST_F(ViewportTest, CameraUsesRenderTargetAspectAndMovesOnlyWhileCaptured) {
    ASSERT_TRUE(viewport_.visible());
    const auto camera  = viewport_.camera_data();
    const auto& target = viewport_.target();
    EXPECT_NEAR(camera.projection_[1][1] / camera.projection_[0][0],
                static_cast<float>(target.width()) / static_cast<float>(target.height()), 1e-5f);
    key(SDL_SCANCODE_W, true);
    frame();
    EXPECT_EQ(viewport_.camera_data().position_, camera.position_);
    capture();
    EXPECT_GT(glm::length(viewport_.camera_data().position_ - camera.position_), 0.01f);
    right_button(false);
    frame();
    EXPECT_FALSE(SDL_GetWindowRelativeMouseMode(window_->handle()));
    const auto stopped = viewport_.camera_data().position_;
    frame();
    EXPECT_EQ(viewport_.camera_data().position_, stopped);
}

TEST_F(ViewportTest, EscapeReleasesMouseAndRestoresImGuiCursorPolicy) {
    capture();
    key(SDL_SCANCODE_ESCAPE, true);
    frame();
    EXPECT_FALSE(SDL_GetWindowRelativeMouseMode(window_->handle()));
    EXPECT_EQ(ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_NoMouseCursorChange, 0);
}

TEST_F(ViewportTest, PopupReleasesMouse) {
    capture();
    frame(true);
    EXPECT_FALSE(SDL_GetWindowRelativeMouseMode(window_->handle()));
}

TEST_F(ViewportTest, ClosingViewportReleasesMouse) {
    capture();
    open_ = false;
    frame();
    EXPECT_FALSE(viewport_.visible());
    EXPECT_FALSE(SDL_GetWindowRelativeMouseMode(window_->handle()));
}

TEST_F(ViewportTest, LosingWindowFocusReleasesMouse) {
    capture();
    SDL_HideWindow(window_->handle());
    SDL_SyncWindow(window_->handle());
    frame();
    EXPECT_FALSE(SDL_GetWindowRelativeMouseMode(window_->handle()));
}

TEST_F(ViewportTest, ReleaseResourcesDropsRenderTargetBeforeGraphicsShutdown) {
    capture();
    const auto texture = viewport_.target().color()->image().id();
    ASSERT_TRUE(glIsTexture(texture));
    viewport_.release_resources();
    EXPECT_FALSE(SDL_GetWindowRelativeMouseMode(window_->handle()));
    EXPECT_FALSE(viewport_.visible());
    EXPECT_FALSE(glIsTexture(texture));
    EXPECT_NO_THROW(viewport_.release_resources());
}

} // namespace
