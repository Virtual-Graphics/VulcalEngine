#include "editor/panels/viewport_panel.h"
#include "base/input.h"
#include "base/logger.h"
#include "common/imgui/imgui_render_target.h"

#include <SDL3/SDL.h>
#include <algorithm>
#include <cmath>
#include <imgui.h>
#include <stdexcept>

namespace editor {

void ViewportPanel::draw(bool& open, const vulcal::gfx::Context::Limits& limits) {
    visible_          = false;
    focused_          = false;
    camera_requested_ = false;
    if (!open) {
        release_camera();
        return;
    }
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{ 0.0f, 0.0f });
    if (ImGui::Begin("Viewport", &open, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse)) {
        ImGui::Text("RMB: look | WASD: move | Q/E: down/up | Wheel: speed (%.2f)", camera_.speed());
        const auto size = ImGui::GetContentRegionAvail();
        if (size.x >= 1.0f && size.y >= 1.0f) {
            const auto scale  = ImGui::GetIO().DisplayFramebufferScale;
            const auto pixels = [](float value, std::uint32_t maximum) {
                return static_cast<std::uint32_t>(std::clamp(std::round(value), 1.0f, static_cast<float>(maximum)));
            };
            const auto width  = pixels(size.x * scale.x, limits.max_render_target_width);
            const auto height = pixels(size.y * scale.y, limits.max_render_target_height);
            if (!target_) {
                target_ = std::make_unique<vulcal::gfx::RenderTarget>(vulcal::gfx::RenderTarget::Spec{ .width = width, .height = height });
            } else {
                target_->resize(width, height);
            }
            // resize() recreates attachments, so obtain the ID only after resizing.
            apps::draw_render_target(*target_, size);
            const bool clicked_left  = ImGui::IsItemClicked(ImGuiMouseButton_Left);
            const bool clicked_right = ImGui::IsItemClicked(ImGuiMouseButton_Right);
            if (clicked_left || clicked_right) ImGui::SetWindowFocus();
            camera_requested_ = clicked_right;
            visible_          = true;
        }
        focused_ = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);
    }
    ImGui::End();
    ImGui::PopStyleVar();
    if (!visible_) release_camera();
}

void ViewportPanel::update_camera(float seconds, SDL_Window* window) {
    const bool focused    = (SDL_GetWindowFlags(window) & SDL_WINDOW_INPUT_FOCUS) != 0;
    const bool popup      = ImGui::IsPopupOpen(nullptr, ImGuiPopupFlags_AnyPopupId | ImGuiPopupFlags_AnyPopupLevel);
    const bool escape     = base::Input::is_key_down(base::Key::Escape);
    const bool right_down = base::Input::is_mouse_button_down(base::MouseButton::Right);
    if (controlling_camera() && (!right_down || escape || !focused || !visible_ || !focused_ || popup || captured_window_ != window)) {
        release_camera();
    }

    bool started = false;
    if (!controlling_camera() && camera_requested_ && right_down && focused && !escape && !popup) {
        const auto [x, y] = base::Input::mouse_position();
        saved_mouse_x_    = x;
        saved_mouse_y_    = y;
        if (SDL_SetWindowRelativeMouseMode(window, true)) {
            captured_window_ = window;
            started          = true;
            ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;
            SDL_HideCursor();
            base::log::debug("Editor camera control started");
        } else {
            base::log::warn("Could not capture editor camera mouse: {}", SDL_GetError());
        }
    }
    if (!controlling_camera()) return;
    camera_.update_from_input(seconds, started);
    ImGui::SetMouseCursor(ImGuiMouseCursor_None);
}

void ViewportPanel::release_camera() {
    if (!controlling_camera()) return;
    SDL_SetWindowRelativeMouseMode(captured_window_, false);
    if ((SDL_GetWindowFlags(captured_window_) & SDL_WINDOW_INPUT_FOCUS) != 0) {
        SDL_WarpMouseInWindow(captured_window_, saved_mouse_x_, saved_mouse_y_);
    }
    SDL_ShowCursor();
    ImGui::GetIO().ConfigFlags &= ~ImGuiConfigFlags_NoMouseCursorChange;
    captured_window_ = nullptr;
    base::log::debug("Editor camera control released");
}

void ViewportPanel::release_resources() {
    release_camera();
    target_.reset();
    visible_ = focused_ = camera_requested_ = false;
}

vulcal::render::CameraData ViewportPanel::camera_data() const {
    if (!target_) throw std::logic_error("Viewport has no render target");
    return camera_.data(static_cast<float>(target_->width()) / static_cast<float>(target_->height()));
}

} // namespace editor
