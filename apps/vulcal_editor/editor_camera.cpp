#include "vulcal_editor/editor_camera.h"
#include "base/input.h"

#include <algorithm>
#include <cmath>
#include <glm/gtc/matrix_transform.hpp>
#include <stdexcept>

namespace editor {

void EditorCamera::update_from_input(float seconds, bool discard_mouse_delta) {
    Input input;
    const auto down = [](base::Key key) { return base::Input::is_key_down(key) ? 1.0f : 0.0f; };
    input.movement = { down(base::Key::D) - down(base::Key::A), down(base::Key::E) - down(base::Key::Q),
        down(base::Key::W) - down(base::Key::S) };
    if (!discard_mouse_delta) {
        const auto [x, y] = base::Input::mouse_delta();
        input.look_delta = { x, y };
    }
    input.wheel = base::Input::mouse_wheel_y();
    update(input, seconds);
}

glm::vec3 EditorCamera::forward() const {
    const float yaw = glm::radians(yaw_);
    const float pitch = glm::radians(pitch_);
    return { std::cos(yaw) * std::cos(pitch), std::sin(pitch), std::sin(yaw) * std::cos(pitch) };
}

void EditorCamera::update(const Input& input, float seconds) {
    speed_ = std::clamp(speed_ * std::pow(1.2f, std::clamp(input.wheel, -100.0f, 100.0f)), 0.05f, 1000.0f);
    yaw_ = std::remainder(yaw_ + input.look_delta.x * 0.15f, 360.0f);
    pitch_ = std::clamp(pitch_ - input.look_delta.y * 0.15f, -89.0f, 89.0f);

    const auto ahead = forward();
    const auto right = glm::normalize(glm::cross(ahead, glm::vec3{ 0.0f, 1.0f, 0.0f }));
    auto direction = right * input.movement.x + glm::vec3{ 0.0f, input.movement.y, 0.0f } + ahead * input.movement.z;
    if (glm::dot(direction, direction) > 0.0f) {
        position_ += glm::normalize(direction) * speed_ * std::max(seconds, 0.0f);
    }
}

vulcal::render::CameraData EditorCamera::data(float aspect) const {
    if (!std::isfinite(aspect) || aspect <= 0.0f) throw std::invalid_argument("Camera aspect must be positive and finite");
    return {
        .view_ = glm::lookAt(position_, position_ + forward(), glm::vec3{ 0.0f, 1.0f, 0.0f }),
        .projection_ = glm::perspective(glm::radians(45.0f), aspect, 0.1f, 1000.0f),
        .position_ = position_,
    };
}

} // namespace editor
