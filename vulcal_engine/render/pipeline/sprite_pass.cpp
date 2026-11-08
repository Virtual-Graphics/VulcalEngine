#include "render/pipeline/sprite_pass.h"

#include "gfx/command.h"
#include "gfx/render_target.h"
#include "gfx/texture.h"
#include "render/render_data.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <glad/glad.h>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>

namespace vulcal::render {
namespace {
struct alignas(16) CameraUBO {
    glm::mat4 view_proj_;
    glm::vec4 position_;
};

struct SpriteVertex {
    glm::vec2 position_;
    glm::vec2 uv_;
};

constexpr std::uint32_t kTextureUnit = 0;
} // namespace

SpritePass::SpritePass()
        : Pass({ .depth_test  = true,
                 .depth_write = false,
                 .cull        = false,
                 .blend       = true,
                 .src_rgb     = gfx::PipelineState::BlendFactor::SrcAlpha,
                 .dst_rgb     = gfx::PipelineState::BlendFactor::OneMinusSrcAlpha,
                 .src_alpha   = gfx::PipelineState::BlendFactor::One,
                 .dst_alpha   = gfx::PipelineState::BlendFactor::OneMinusSrcAlpha }) {
    load_shaders("sprite.vert", "sprite.frag");
    program_.bind_uniform_block("CameraBlock", 0);
    camera_ubo_.set_data(CameraUBO{ glm::mat4(1.0f), glm::vec4() }, 0);

    const std::vector<SpriteVertex> vertices{
        { { -0.5f, -0.5f }, { 0.0f, 0.0f } },
        {  { 0.5f, -0.5f }, { 1.0f, 0.0f } },
        {   { 0.5f, 0.5f }, { 1.0f, 1.0f } },
        {  { -0.5f, 0.5f }, { 0.0f, 1.0f } },
    };
    const std::vector<std::uint32_t> indices{ 0, 1, 2, 2, 3, 0 };

    quad_vertices_.set_data(vertices);
    quad_indices_.set_data(indices, gfx::IndexBuffer::Type::Uint32);

    const gfx::VertexLayout layout{
        {
            { 0, 2, GL_FLOAT, GL_FALSE, offsetof(SpriteVertex, position_) },
            { 1, 2, GL_FLOAT, GL_FALSE, offsetof(SpriteVertex, uv_) },
        },
        sizeof(SpriteVertex)
    };
    quad_vertex_array_.set_vertex_buffer(quad_vertices_, layout);
    quad_vertex_array_.set_index_buffer(quad_indices_);

    gfx::Image white_image(gfx::Image::Format::RGBA8, 1, 1, 1);
    const std::uint8_t white[4]{ 255, 255, 255, 255 };
    white_image.set_data(white);
    white_texture_ = std::make_unique<gfx::Texture>(std::move(white_image), gfx::Sampler(gfx::Sampler::Spec::clamp_to_edge()));
}

bool SpritePass::enabled(const RenderData& data) const {
    return std::any_of(data.items_.begin(), data.items_.end(), [](const RenderItem& item) { return item.kind_ == RenderItemKind::Sprite; });
}

void SpritePass::prepare(PassContext& ctx) {
    if (ctx.target_ != nullptr) {
        ctx.target_->bind();
    } else {
        gfx::RenderTarget::unbind();
        gfx::Command::set_viewport(static_cast<std::uint32_t>(ctx.viewport_.z), static_cast<std::uint32_t>(ctx.viewport_.w));
    }

    bind_pipeline();

    const CameraUBO camera{ ctx.data_.camera_.projection_ * ctx.data_.camera_.view_, glm::vec4(ctx.data_.camera_.position_, 1.0f) };
    camera_ubo_.set_sub_data(camera);
    camera_ubo_.bind_base(0);

    program_.set("u_texture", static_cast<int>(kTextureUnit));
    quad_vertex_array_.bind();
}

void SpritePass::draw(PassContext& ctx, const DrawItem& item) {
    if (item.kind_ != RenderItemKind::Sprite) return;

    const Sprite& sprite         = item.sprite_;
    const glm::vec2 pivot_offset = sprite.pivot_ - glm::vec2(0.5f);
    const glm::mat4 model = item.world_transform_ * glm::translate(glm::mat4(1.0f), glm::vec3(-pivot_offset.x, -pivot_offset.y, 0.0f));

    const gfx::Texture& texture = sprite.texture_ != nullptr ? *sprite.texture_ : *white_texture_;
    texture.bind(kTextureUnit);

    program_.set("u_model", model);
    program_.set("u_color", sprite.color_);
    program_.set("u_uv_rect", sprite.uv_rect_);
    program_.set("u_flip", glm::vec2(sprite.flip_x_ ? 1.0f : 0.0f, sprite.flip_y_ ? 1.0f : 0.0f));

    gfx::Command::draw_indexed(quad_vertex_array_);
}

void SpritePass::finish(PassContext& ctx) {
    gfx::Program::unuse();
    gfx::VertexArray::unbind();
    gfx::RenderTarget::unbind();
}

} // namespace vulcal::render
