#pragma once

#include "render/render_data.h"
#include "scene/components.h"

#include <glm/gtc/quaternion.hpp>
#include <optional>

namespace vulcal::scene {

class Scene;

class SceneExtractor {
public:
    enum class CameraPolicy { Primary, First };

    struct Options {
        CameraPolicy camera_policy_ = CameraPolicy::Primary;
        std::optional<render::CameraData> camera_override_;
    };

    render::RenderData extract(const Scene& scene) const;
    render::RenderData extract(const Scene& scene, const Options& options) const;

private:
    static render::DirectionalLight make_directional_light(const LightComponent& light, const TransformComponent& transform) {
        return render::DirectionalLight{
            // Local -Z is the light's propagation direction. Position and scale do not affect it.
            .direction_ = glm::normalize(glm::quat(transform.rotation_) * glm::vec3{ 0.0f, 0.0f, -1.0f }),
            .color_ = light.color_,
            .intensity_ = light.intensity_,
            .shadow_ =
                    render::DirectionalLightShadow{
                        .enabled_ = light.shadow_enabled_,
                        .map_size_ = light.shadow_map_size_,
                        .center_ = light.shadow_center_,
                        .extent_ = light.shadow_extent_,
                        .near_ = light.shadow_near_,
                        .far_ = light.shadow_far_,
                        .depth_bias_ = light.shadow_depth_bias_,
                        .normal_bias_ = light.shadow_normal_bias_,
                    },
        };
    }

    static render::Environment make_environment(const EnvironmentComponent& environment) {
        return render::Environment{
            .ambient_color_ = environment.ambient_color_,
            .clear_color_ = environment.clear_color_,
        };
    }

    static render::CameraData make_camera_data(const CameraComponent& camera, const glm::mat4& world_transform) {
        if (camera.projection_type_ == CameraComponent::ProjectionType::Perspective) {
            const render::PerspectiveCamera::Spec spec{
                .fov_y_ = camera.perspective_fov_y_,
                .aspect_ = camera.perspective_aspect_,
                .near_ = camera.perspective_near_,
                .far_ = camera.perspective_far_,
            };
            return render::PerspectiveCamera{ spec }.data(world_transform);
        }

        const render::OrthographicCamera::Spec spec{
            .left_ = camera.orthographic_left_,
            .right_ = camera.orthographic_right_,
            .bottom_ = camera.orthographic_bottom_,
            .top_ = camera.orthographic_top_,
            .near_ = camera.orthographic_near_,
            .far_ = camera.orthographic_far_,
        };
        return render::OrthographicCamera{ spec }.data(world_transform);
    }
};

} // namespace vulcal::scene
