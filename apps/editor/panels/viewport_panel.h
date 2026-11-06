#pragma once

#include "editor/editor_camera.h"
#include "gfx/context.h"
#include "gfx/render_target.h"

#include <memory>

namespace editor {

class ViewportPanel {
public:
    void draw(bool& open, const vulcal::gfx::Context::Limits& limits);
    void update_camera(float seconds, SDL_Window* window);
    void release_camera();
    // Call before destroying the window, ImGui or graphics context.
    void release_resources();
    float aspect() const;
    vulcal::render::CameraData camera_data() const;
    bool visible() const { return visible_; }
    bool controlling_camera() const { return captured_window_ != nullptr; }
    vulcal::gfx::RenderTarget& target() { return *target_; }

private:
    std::unique_ptr<vulcal::gfx::RenderTarget> target_;
    EditorCamera camera_;
    SDL_Window* captured_window_ = nullptr;
    float saved_mouse_x_         = 0.0f;
    float saved_mouse_y_         = 0.0f;
    bool visible_                = false;
    bool focused_                = false;
    bool camera_requested_       = false;
};

} // namespace editor
