#include "editor/project/scene/default_scene.h"

#include "scene/scene.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

namespace editor {
namespace {

void add_sun(vulcal::scene::Scene& scene) {
    auto entity = scene.create_entity("Sun");
    auto& light = entity.add_component<vulcal::scene::LightComponent>();
    entity.get_component<vulcal::scene::TransformComponent>().rotation_ =
            glm::eulerAngles(glm::quatLookAt(glm::normalize(glm::vec3{ -1.0f }), glm::vec3{ 0.0f, 1.0f, 0.0f }));
    light.shadow_center_ = { 0.0f, -0.5f, 0.0f };
    light.shadow_extent_ = 8.0f;
    light.shadow_far_ = 40.0f;
}

void add_camera(vulcal::scene::Scene& scene) {
    auto entity = scene.create_entity("Camera");
    auto& transform = entity.get_component<vulcal::scene::TransformComponent>();
    const auto world =
            glm::inverse(glm::lookAt(glm::vec3{ 0.0f, 4.0f, 8.0f }, glm::vec3{ 0.0f, -0.5f, 0.0f }, glm::vec3{ 0.0f, 1.0f, 0.0f }));
    transform.translation_ = glm::vec3{ world[3] };
    transform.rotation_ = glm::eulerAngles(glm::quat_cast(world));
    entity.add_component<vulcal::scene::CameraComponent>();
}

void add_environment(vulcal::scene::Scene& scene) {
    auto entity = scene.create_entity("Environment");
    auto& environment = entity.add_component<vulcal::scene::EnvironmentComponent>();
    environment.ambient_color_ = glm::vec3{ 0.15f };
    environment.clear_color_ = { 0.08f, 0.09f, 0.11f, 1.0f };
}

} // namespace

std::unique_ptr<vulcal::scene::Scene> create_default_scene() {
    auto scene = std::make_unique<vulcal::scene::Scene>();
    add_sun(*scene);
    add_camera(*scene);
    add_environment(*scene);
    return scene;
}

} // namespace editor
