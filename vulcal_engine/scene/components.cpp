#include "scene/components.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

namespace vulcal::scene {

glm::mat4 TransformComponent::matrix() const {
    glm::mat4 transform = glm::translate(glm::mat4(1.0f), translation_);
    transform *= glm::mat4_cast(glm::quat(rotation_));
    return glm::scale(transform, scale_);
}

} // namespace vulcal::scene
