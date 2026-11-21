#pragma once

#include "render/camera.h"

namespace editor {

// Independent of scene cameras and viewport/window capture policy.
class EditorCamera {
public:
    struct Input {
        glm::vec3 movement{ 0.0f }; // right, world up, forward
        glm::vec2 look_delta{ 0.0f };
        float wheel = 0.0f;
    };

    void update(const Input& input, float seconds);
    // Called by the viewport only while camera control is active.
    void update_from_input(float seconds, bool discard_mouse_delta = false);
    vulcal::render::CameraData data(float aspect) const;
    float speed() const { return speed_; }

private:
    glm::vec3 forward() const;

    glm::vec3 position_{ 0.0f, 4.0f, 8.0f };
    float yaw_   = -90.0f;
    float pitch_ = -29.357754f;
    float speed_ = 5.0f;
};

} // namespace editor
