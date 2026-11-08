#include "scene/scene_extractor.h"

#include "asset/render_asset_cache.h"
#include "base/logger.h"
#include "scene/components.h"
#include "scene/scene.h"

#include <cmath>
#include <stdexcept>

namespace vulcal::scene {
namespace {

struct CameraSelection {
    const CameraComponent* camera_ = nullptr;
    glm::mat4 world_transform_{ 1.0f };
};

CameraSelection find_camera(const Scene& scene, SceneExtractor::CameraPolicy policy) {
    const auto view = scene.registry().view<const TransformComponent, const CameraComponent>();
    CameraSelection result;

    for (const auto entity: view) {
        const auto& camera    = view.get<const CameraComponent>(entity);
        const auto& transform = view.get<const TransformComponent>(entity);

        if (result.camera_ == nullptr) {
            result.camera_          = &camera;
            result.world_transform_ = transform.matrix();
        }

        if (policy == SceneExtractor::CameraPolicy::Primary && camera.primary_) {
            return CameraSelection{ &camera, transform.matrix() };
        }
    }

    return result;
}

} // namespace

SceneExtractor::SceneExtractor(asset::RenderAssetCache& resources) : resources_(&resources) {}

render::RenderData SceneExtractor::extract(const Scene& scene, float aspect) const {
    Options options;
    options.aspect_ = aspect;
    return extract(scene, options);
}

render::RenderData SceneExtractor::extract(const Scene& scene, const Options& options) const {
    render::RenderData data;

    if (options.camera_override_) {
        data.camera_ = *options.camera_override_;
    } else {
        if (!std::isfinite(options.aspect_) || options.aspect_ <= 0.0f) {
            throw std::invalid_argument("SceneExtractor: camera aspect must be positive and finite");
        }
        const auto camera = find_camera(scene, options.camera_policy_);
        if (camera.camera_ == nullptr) {
            base::log::error("SceneExtractor: scene has no camera");
            throw std::runtime_error("SceneExtractor: scene has no camera");
        }
        data.camera_ = make_camera_data(*camera.camera_, camera.world_transform_, options.aspect_);
    }

    const auto environment_view = scene.registry().view<const EnvironmentComponent>();
    if (!environment_view.empty()) {
        data.environment_ = make_environment(environment_view.get<const EnvironmentComponent>(environment_view.front()));
    }

    const auto light_view = scene.registry().view<const TransformComponent, const LightComponent>();
    for (const auto entity: light_view) {
        const auto& light = light_view.get<const LightComponent>(entity);
        switch (light.type_) {
            case LightType::Directional:
                data.directional_light_ = make_directional_light(light, light_view.get<const TransformComponent>(entity));
                break;
        }
        if (data.directional_light_) break;
    }

    const auto render_view = scene.registry().view<const TransformComponent, const MeshRendererComponent>();
    for (const auto entity: render_view) {
        const auto& transform = render_view.get<const TransformComponent>(entity);
        const auto& renderer  = render_view.get<const MeshRendererComponent>(entity);
        if (!renderer.visible_ || renderer.mesh_asset_id_.is_nil() || !resources_) {
            continue;
        }

        const auto mesh = resources_->resolve_mesh(renderer.mesh_asset_id_);
        if (mesh.value_ == 0) continue;
        const auto material = resources_->resolve_material(renderer.material_asset_id_);
        if (material.value_ == 0) continue;
        data.items_.push_back(render::RenderItem{ mesh, material, transform.matrix() });
    }

    const auto sprite_view = scene.registry().view<const TransformComponent, const SpriteRendererComponent>();
    for (const auto entity: sprite_view) {
        const auto& transform = sprite_view.get<const TransformComponent>(entity);
        const auto& sprite    = sprite_view.get<const SpriteRendererComponent>(entity);
        if (!sprite.visible_ || !resources_) {
            continue;
        }

        render::TextureHandle texture{};
        if (!sprite.texture_asset_id_.is_nil()) {
            texture = resources_->resolve_texture(sprite.texture_asset_id_);
            if (texture.value_ == 0) continue;
        }

        data.items_.push_back(render::RenderItem{
            .world_transform_ = transform.matrix(),
            .kind_            = render::RenderItemKind::Sprite,
            .sprite_          = render::SpriteData{
                .texture_       = texture,
                .color_         = sprite.color_,
                .uv_rect_       = sprite.uv_rect_,
                .pivot_         = sprite.pivot_,
                .flip_x_        = sprite.flip_x_,
                .flip_y_        = sprite.flip_y_,
                .sorting_order_ = sprite.sorting_order_,
            },
        });
    }

    return data;
}

} // namespace vulcal::scene
