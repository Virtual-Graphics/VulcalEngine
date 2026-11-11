#pragma once
#include "gfx/pipeline_state.h"
#include "gfx/program.h"
#include "render/render_item_kind.h"
#include <cstdint>
#include <glm/glm.hpp>

namespace vulcal::gfx {
class RenderTarget;
class Texture;
} // namespace vulcal::gfx

namespace vulcal::render {

class MeshGPU;
struct Material;
struct RenderData;

struct Sprite {
    const gfx::Texture* texture_ = nullptr;
    glm::vec4 color_{ 1.0f };
    glm::vec4 uv_rect_{ 0.0f, 0.0f, 1.0f, 1.0f };
    glm::vec2 pivot_{ 0.5f, 0.5f };
    bool flip_x_                = false;
    bool flip_y_                = false;
    std::int32_t sorting_order_ = 0;
};

// One drawable after the renderer has resolved its resource handles.
struct DrawItem {
    RenderItemKind kind_ = RenderItemKind::Mesh;
    glm::mat4 world_transform_{ 1.0f };

    const MeshGPU* mesh_      = nullptr;
    const Material* material_ = nullptr;

    Sprite sprite_;
};

// Shared state threaded through every pass of a frame. Passes may write to it
// so that later passes can consume their results (e.g. shadow map).
struct PassContext {
    const RenderData& data_;
    gfx::RenderTarget* target_ = nullptr;
    glm::ivec4 viewport_{ 0, 0, 1, 1 };
    const gfx::Texture* shadow_map_ = nullptr;
    glm::mat4 light_view_proj_{ 1.0f };
};

class Pass {
public:
    virtual ~Pass() = default;

    Pass(const Pass&) = delete;
    Pass& operator=(const Pass&) = delete;

    virtual const char* name() const = 0;
    virtual bool enabled(const RenderData& data) const { return true; }

    virtual void prepare(PassContext& ctx) = 0;
    virtual void draw(PassContext& ctx, const DrawItem& item) = 0;
    virtual void finish(PassContext& ctx) = 0;

protected:
    explicit Pass(const gfx::PipelineState::Spec& pipeline_spec = {});

    void load_shaders(const char* vertex_source, const char* fragment_source);
    void bind_pipeline();

    gfx::Program program_;
    gfx::PipelineState pipeline_state_;
};

} // namespace vulcal::render
