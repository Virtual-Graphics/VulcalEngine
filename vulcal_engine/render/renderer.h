#pragma once

#include "render/render_data.h"
#include "render/resources/material.h"
#include "render/resources/mesh_data.h"
#include "render/resources/resources_handle.h"
#include "render/resources/texture_data.h"
#include <memory>

namespace vulcal::gfx{
class RenderTarget;
}

namespace vulcal::render {

class Renderer {
public:
    Renderer();
    ~Renderer();

    MeshHandle create_mesh(const MeshData& data);
    void destroy_mesh(MeshHandle mesh);

    MaterialHandle create_material(const Material& material);
    void update_material(MaterialHandle handle, const Material& material);
    void destroy_material(MaterialHandle material);

    TextureHandle create_texture(const TextureData& texture);
    void destroy_texture(TextureHandle texture);

    void render(const RenderData& data, gfx::RenderTarget* target = nullptr);
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
}