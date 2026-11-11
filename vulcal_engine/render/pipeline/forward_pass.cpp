#include "render/pipeline/forward_pass.h"

#include "gfx/command.h"
#include "gfx/render_target.h"
#include "gfx/texture.h"
#include "gfx/vertex_array.h"
#include "render/render_data.h"
#include "render/resources/mesh_gpu.h"
#include "render/shaders/embedded_shaders.h"

namespace vulcal::render {
namespace {
struct alignas(16) CameraUBO {
    glm::mat4 view_proj_;
    glm::vec4 position_;
};

constexpr std::uint32_t kShadowMapUnit = 0;
} // namespace

ForwardPass::ForwardPass() : Pass({ .depth_test = true }) {
    load_shaders(shaders::blinn_phong_vert, shaders::blinn_phong_frag);
    program_.bind_uniform_block("CameraBlock", 0);
    camera_ubo_.set_data(CameraUBO{ glm::mat4(1.0f), glm::vec4() }, 0);
}

void ForwardPass::prepare(PassContext& ctx) {
    if (ctx.target_ != nullptr) {
        ctx.target_->bind();
    } else {
        gfx::RenderTarget::unbind();
        gfx::Command::set_viewport(static_cast<std::uint32_t>(ctx.viewport_.z), static_cast<std::uint32_t>(ctx.viewport_.w));
    }

    bind_pipeline();

    if (ctx.target_ != nullptr) {
        ctx.target_->clear(ctx.data_.environment_.clear_color_, 1.0f);
    } else {
        gfx::Command::clear(ctx.data_.environment_.clear_color_);
    }

    const CameraUBO camera_ubo{ ctx.data_.camera_.projection_ * ctx.data_.camera_.view_, glm::vec4(ctx.data_.camera_.position_, 1.0f) };
    camera_ubo_.set_sub_data(camera_ubo);
    camera_ubo_.bind_base(0);

    program_.set("u_ambient_color", ctx.data_.environment_.ambient_color_);
    program_.set("u_light_view_proj", ctx.light_view_proj_);

    if (ctx.shadow_map_ != nullptr) {
        ctx.shadow_map_->bind(kShadowMapUnit);
        program_.set("u_shadow_map", static_cast<int>(kShadowMapUnit));
        program_.set("u_shadow_enabled", 1);
        if (ctx.data_.directional_light_) {
            const auto& shadow = ctx.data_.directional_light_->shadow_;
            program_.set("u_shadow_depth_bias", shadow.depth_bias_);
            program_.set("u_shadow_normal_bias", shadow.normal_bias_);
        }
    } else {
        program_.set("u_shadow_enabled", 0);
    }

    set_light(ctx.data_);
}

void ForwardPass::set_light(const RenderData& data) {
    if (data.directional_light_) {
        const auto& light = *data.directional_light_;
        program_.set("u_light_direction", glm::normalize(light.direction_));
        program_.set("u_light_color", light.color_ * light.intensity_);
    } else {
        program_.set("u_light_direction", glm::vec3(0.0f, -1.0f, 0.0f));
        program_.set("u_light_color", glm::vec3(0.0f));
    }
}

void ForwardPass::draw(PassContext& ctx, const DrawItem& item) {
    if (item.kind_ != RenderItemKind::Mesh) return;

    const Material& material = *item.material_;
    program_.set("u_model", item.world_transform_);
    program_.set("u_normal_matrix", glm::transpose(glm::inverse(glm::mat3(item.world_transform_))));
    program_.set("u_diffuse", material.diffuse_);
    program_.set("u_specular", material.specular_);
    program_.set("u_shininess", material.shininess_);
    gfx::Command::draw_indexed(item.mesh_->vertex_array());
}

void ForwardPass::finish(PassContext& ctx) {
    gfx::Program::unuse();
    gfx::VertexArray::unbind();
    gfx::RenderTarget::unbind();
}

} // namespace vulcal::render
