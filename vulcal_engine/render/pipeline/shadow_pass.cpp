#include "render/pipeline/shadow_pass.h"

#include "base/logger.h"
#include "gfx/command.h"
#include "gfx/debug.h"
#include "gfx/render_target.h"
#include "render/render_data.h"
#include "render/resources/mesh_gpu.h"
#include <glm/gtc/matrix_transform.hpp>

namespace vulcal::render {
namespace {

glm::mat4 light_view_projection(const DirectionalLight& light) {
    const float distance = light.shadow_.far_ * 0.5f;
    const glm::vec3 direction = glm::normalize(light.direction_);
    const glm::vec3 center = light.shadow_.center_;
    const glm::vec3 eye = center - direction * distance;

    const glm::vec3 up = std::abs(glm::dot(direction, glm::vec3{ 0.0f, 1.0f, 0.0f })) > 0.99f
                                 ? glm::vec3{ 0.0f, 0.0f, 1.0f }
                                 : glm::vec3{ 0.0f, 1.0f, 0.0f };

    const glm::mat4 view = glm::lookAt(eye, center, up);
    const float extent = light.shadow_.extent_;
    const glm::mat4 projection = glm::ortho(-extent, extent, -extent, extent, light.shadow_.near_, light.shadow_.far_);
    return projection * view;
}

} // namespace

ShadowPass::ShadowPass()
        : Pass({ .depth_test = true,
              .depth_write = true,
              .cull = false,
              .polygon_offset = true,
              .polygon_offset_factor = 2.0f,
              .polygon_offset_units = 4.0f }) {
    load_shaders("shadow.vert", "shadow.frag");
}

ShadowPass::~ShadowPass() = default;

bool ShadowPass::enabled(const RenderData& data) const {
    return data.directional_light_ && data.directional_light_->shadow_.enabled_;
}

void ShadowPass::ensure_target(std::uint32_t map_size) {
    if (target_ && map_size_ == map_size) {
        return;
    }

    map_size_ = map_size;

    gfx::RenderTarget::Spec spec;
    spec.width = map_size;
    spec.height = map_size;
    spec.color_format = std::nullopt;
    spec.depth_format = gfx::Image::Format::Depth32F;

    gfx::Sampler::Spec depth_sampler = gfx::Sampler::Spec::clamp_to_edge();
    depth_sampler.compare_func = gfx::CompareFunc::LessEqual;
    spec.depth_sampler = depth_sampler;

    target_ = std::make_unique<gfx::RenderTarget>(spec);
    gfx::set_object_label(GL_TEXTURE, target_->depth()->image().id(), "shadow map");
    base::log::debug("Shadow map created: {}x{}", map_size, map_size);
}

void ShadowPass::prepare(PassContext& ctx) {
    const auto& light = *ctx.data_.directional_light_;

    ensure_target(light.shadow_.map_size_);
    ctx.light_view_proj_ = light_view_projection(light);
    ctx.shadow_map_ = target_->depth();

    target_->bind();
    target_->clear_depth();

    bind_pipeline();
    program_.set("u_light_view_proj", ctx.light_view_proj_);
}

void ShadowPass::draw(PassContext& ctx, const DrawItem& item) {
    if (item.kind_ != RenderItemKind::Mesh) return;

    program_.set("u_model", item.world_transform_);
    gfx::Command::draw_indexed(item.mesh_->vertex_array());
}

void ShadowPass::finish(PassContext& ctx) {}

} // namespace vulcal::render
