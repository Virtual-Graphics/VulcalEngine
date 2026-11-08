#pragma once
#include "gfx/index_buffer.h"
#include "gfx/texture.h"
#include "gfx/uniform_buffer.h"
#include "gfx/vertex_array.h"
#include "gfx/vertex_buffer.h"
#include "render/pipeline/pass.h"

#include <memory>

namespace vulcal::render {

class SpritePass : public Pass {
public:
    SpritePass();

    const char* name() const override { return "SpritePass"; }
    bool enabled(const RenderData& data) const override;

    void prepare(PassContext& ctx) override;
    void draw(PassContext& ctx, const DrawItem& item) override;
    void finish(PassContext& ctx) override;

private:
    gfx::UniformBuffer camera_ubo_;
    gfx::VertexBuffer quad_vertices_;
    gfx::IndexBuffer quad_indices_;
    gfx::VertexArray quad_vertex_array_;
    std::unique_ptr<gfx::Texture> white_texture_;
};

} // namespace vulcal::render
