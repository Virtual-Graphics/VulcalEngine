#pragma once
#include "glm/ext/matrix_float4x4.hpp"
#include "render/camera.h"
#include "render/render_item_kind.h"
#include "render/resources/resources_handle.h"

#include <cstdint>
#include <glm/glm.hpp>
#include <optional>
#include <vector>

namespace vulcal::render {

struct DirectionalLightShadow {
    bool enabled_ = true;
    std::uint32_t map_size_ = 2048;
    glm::vec3 center_{ 0.0f };
    float extent_ = 10.0f;
    float near_ = 0.1f;
    float far_ = 50.0f;
    float depth_bias_ = 0.0015f;
    float normal_bias_ = 0.02f;
};

struct DirectionalLight {
    glm::vec3 direction_{ 0.0f, -1.0f, 0.0f };
    glm::vec3 color_{ 1.0f };
    float intensity_ = 1.0f;
    DirectionalLightShadow shadow_;
};

struct Environment {
    glm::vec3 ambient_color_{ 0.1f };
    glm::vec4 clear_color_{ 0.1f, 0.1f, 0.1f, 1.0f };
};

struct SpriteData {
    TextureHandle texture_;
    glm::vec4 color_{ 1.0f };
    glm::vec4 uv_rect_{ 0.0f, 0.0f, 1.0f, 1.0f };
    glm::vec2 pivot_{ 0.5f, 0.5f };
    bool flip_x_                = false;
    bool flip_y_                = false;
    std::int32_t sorting_order_ = 0;
};

struct RenderItem {
    MeshHandle mesh_;
    MaterialHandle material_;
    glm::mat4 world_transform_{ 1.0f };

    RenderItemKind kind_ = RenderItemKind::Mesh;
    SpriteData sprite_;
};

struct RenderData {
    CameraData camera_;
    Environment environment_;
    std::optional<DirectionalLight> directional_light_;
    std::vector<RenderItem> items_;
};
} // namespace vulcal::render