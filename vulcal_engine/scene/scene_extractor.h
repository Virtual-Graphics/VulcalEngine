#pragma once

#include "render/render_data.h"
#include "scene/components.h"

#include <glm/gtc/quaternion.hpp>
#include <cmath>
#include <optional>
#include <stdexcept>

namespace vulcal::asset {
class RenderAssetCache;
}

namespace vulcal::scene {

class Scene;

class SceneExtractor {
public:
    // Without a cache, extracts camera, light, and environment data only.
    SceneExtractor() = default;
    explicit SceneExtractor(asset::RenderAssetCache& resources);

    enum class CameraPolicy { Primary, First };

    struct Options {
        CameraPolicy camera_policy_ = CameraPolicy::Primary;
        std::optional<render::CameraData> camera_override_;
        std::optional<float> aspect_override_;
    };

    render::RenderData extract(const Scene& scene) const;
    render::RenderData extract(const Scene& scene, const Options& options) const;

private:
    asset::RenderAssetCache* resources_ = nullptr;

    static render::DirectionalLight make_directional_light(const LightComponent& light, const TransformComponent& transform) {
        return render::DirectionalLight{
            // Local -Z is the light's propagation direction. Position and scale do not affect it.
            .direction_ = glm::normalize(glm::quat(transform.rotation_) * glm::vec3{ 0.0f, 0.0f, -1.0f }),
            .color_     = light.color_,
            .intensity_ = light.intensity_,
            .shadow_    = render::DirectionalLightShadow{
                                                                                    .enabled_     = light.shadow_enabled_,
                                                                                    .map_size_    = light.shadow_map_size_,
                                                                                    .center_      = light.shadow_center_,
                                                                                    .extent_      = light.shadow_extent_,
                                                                                    .near_        = light.shadow_near_,
                                                                                    .far_         = light.shadow_far_,
                                                                                    .depth_bias_  = light.shadow_depth_bias_,
                                                                                    .normal_bias_ = light.shadow_normal_bias_,
                                                                                    },
        };
    }

    static render::Environment make_environment(const EnvironmentComponent& environment) {
        return render::Environment{
            .ambient_color_ = environment.ambient_color_,
            .clear_color_   = environment.clear_color_,
        };
    }

    static render::CameraData make_camera_data(const CameraComponent& camera, const glm::mat4& world_transform,
                                               std::optional<float> aspect_override) {
        if (camera.projection_type_ == CameraComponent::ProjectionType::Perspective) {
            const render::PerspectiveCamera::Spec spec{
                .fov_y_  = camera.perspective_fov_y_,
                .aspect_ = aspect_override.value_or(camera.perspective_aspect_),
                .near_   = camera.perspective_near_,
                .far_    = camera.perspective_far_,
            };
            return render::PerspectiveCamera{ spec }.data(world_transform);
        }

        float left          = camera.orthographic_left_;
        float right         = camera.orthographic_right_;
        const float bottom  = camera.orthographic_bottom_;
        const float top     = camera.orthographic_top_;
        if (aspect_override) {
            const float height = top - bottom;
            if (!std::isfinite(height) || height <= 0.0f) {
                throw std::invalid_argument("SceneExtractor: orthographic camera must have a positive finite height");
            }
            const float center = (left + right) * 0.5f;
            const float width  = height * *aspect_override;
            left               = center - width * 0.5f;
            right              = center + width * 0.5f;
        }

        const render::OrthographicCamera::Spec spec{
            .left_   = left,
            .right_  = right,
            .bottom_ = bottom,
            .top_    = top,
            .near_   = camera.orthographic_near_,
            .far_    = camera.orthographic_far_,
        };
        return render::OrthographicCamera{ spec }.data(world_transform);
    }
};

} // namespace vulcal::scene
