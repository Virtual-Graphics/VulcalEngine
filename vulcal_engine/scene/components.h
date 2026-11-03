#pragma once

#include "base/uuid.h"
#include "render/resources/resources_handle.h"

#include <cstdint>
#include <glm/glm.hpp>
#include <string>

namespace vulcal::scene {

struct IDComponent {
    base::UUID id_ = base::UUID::generate();
};

struct TagComponent {
    std::string name_ = "Entity";
};

struct TransformComponent {
    glm::vec3 translation_{ 0.0f };
    glm::vec3 rotation_{ 0.0f };
    glm::vec3 scale_{ 1.0f };

    glm::mat4 matrix() const;
};

struct MeshRendererComponent {
    render::MeshHandle mesh_;
    render::MaterialHandle material_;
    bool visible_ = true;
};

struct CameraComponent {
    enum class ProjectionType { Perspective, Orthographic };

    ProjectionType projection_type_ = ProjectionType::Perspective;
    bool primary_ = true;

    float perspective_fov_y_ = glm::radians(45.0f);
    float perspective_aspect_ = 1.0f;
    float perspective_near_ = 0.1f;
    float perspective_far_ = 100.0f;

    float orthographic_left_ = -1.0f;
    float orthographic_right_ = 1.0f;
    float orthographic_bottom_ = -1.0f;
    float orthographic_top_ = 1.0f;
    float orthographic_near_ = 0.1f;
    float orthographic_far_ = 100.0f;

    void set_aspect(float aspect);
};

enum class LightType { Directional };

struct LightComponent {
    LightType type_ = LightType::Directional;
    glm::vec3 color_{ 1.0f };
    float intensity_ = 1.0f;

    bool shadow_enabled_ = true;
    std::uint32_t shadow_map_size_ = 2048;
    glm::vec3 shadow_center_{ 0.0f };
    float shadow_extent_ = 10.0f;
    float shadow_near_ = 0.1f;
    float shadow_far_ = 50.0f;
    float shadow_depth_bias_ = 0.0015f;
    float shadow_normal_bias_ = 0.02f;
};

struct EnvironmentComponent {
    glm::vec3 ambient_color_{ 0.1f };
    glm::vec4 clear_color_{ 0.1f, 0.1f, 0.1f, 1.0f };
};

} // namespace vulcal::scene
