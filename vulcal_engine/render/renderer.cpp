#include "render/renderer.h"

#include "base/logger.h"
#include "gfx/debug.h"
#include "gfx/render_target.h"
#include "gfx/texture.h"
#include "render/pipeline/pipeline.h"
#include "render/resources/mesh_gpu.h"
#include "resources/material.h"
#include <algorithm>
#include <atomic>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <unordered_map>

namespace {
std::uint64_t next_resource_id() {
    static std::uint64_t next{ 1 };
    return next++;
}
} // namespace
namespace vulcal::render {
struct Renderer::Impl {
    std::unordered_map<std::uint64_t, std::unique_ptr<MeshGPU>> meshes_;
    std::unordered_map<std::uint64_t, Material> materials_;
    std::unordered_map<std::uint64_t, std::unique_ptr<gfx::Texture>> textures_;
    Pipeline pipeline_;
};

Renderer::Renderer()
        : impl_(std::make_unique<Impl>()) {
    base::log::debug("Renderer: initialized");
}

Renderer::~Renderer() {
    base::log::debug("Renderer: shutting down ({} meshes, {} materials, {} textures remaining)", impl_->meshes_.size(),
                     impl_->materials_.size(), impl_->textures_.size());
}

MeshHandle Renderer::create_mesh(const MeshData& data) {
    auto mesh = std::make_unique<MeshGPU>(data);
    MeshHandle handle{ next_resource_id() };
    impl_->meshes_.emplace(handle.value_, std::move(mesh));
    base::log::debug("Created mesh {} ({} vertices, {} indices)", handle.value_, data.vertices_.size(), data.indices_.size());
    return handle;
}

void Renderer::destroy_mesh(MeshHandle mesh) {
    if (impl_->meshes_.erase(mesh.value_) == 0) {
        base::log::error("Invalid mesh handle: {}", mesh.value_);
        throw std::invalid_argument("Invalid mesh handle");
    }
    base::log::debug("Destroyed mesh {}", mesh.value_);
}

MaterialHandle Renderer::create_material(const Material& material) {
    MaterialHandle handle{ next_resource_id() };
    impl_->materials_.emplace(handle.value_, material);
    base::log::debug("Created material {}", handle.value_);
    return handle;
}

void Renderer::update_material(MaterialHandle handle, const Material& material) {
    auto it = impl_->materials_.find(handle.value_);
    if (it == impl_->materials_.end()) {
        base::log::error("Invalid material handle: {}", handle.value_);
        throw std::invalid_argument("Invalid material handle");
    }
    it->second = material;
    base::log::trace("Updated material {}", handle.value_);
}

void Renderer::destroy_material(MaterialHandle material) {
    if (impl_->materials_.erase(material.value_) == 0) {
        base::log::error("Invalid material handle: {}", material.value_);
        throw std::invalid_argument("Invalid material handle");
    }
    base::log::debug("Destroyed material {}", material.value_);
}

TextureHandle Renderer::create_texture(const TextureData& texture) {
    if (texture.width_ == 0 || texture.height_ == 0) {
        throw std::invalid_argument("Texture dimensions must be non-zero");
    }
    if (texture.format_ != TextureFormat::RGBA8) {
        throw std::invalid_argument("Unsupported texture format");
    }

    constexpr auto max     = std::numeric_limits<std::uint64_t>::max();
    const auto pixel_count = static_cast<std::uint64_t>(texture.width_) * static_cast<std::uint64_t>(texture.height_);
    if (pixel_count > max / 4) {
        throw std::invalid_argument("Texture is too large");
    }
    const std::uint64_t expected = pixel_count * 4;
    if (texture.pixels_.size() != expected) {
        throw std::invalid_argument("Texture pixel data has an invalid size");
    }

    gfx::Image image(gfx::Image::Format::RGBA8, texture.width_, texture.height_, 1);
    image.set_data(texture.pixels_.data());
    gfx::Sampler sampler(gfx::Sampler::Spec::clamp_to_edge());

    TextureHandle handle{ next_resource_id() };
    impl_->textures_.emplace(handle.value_, std::make_unique<gfx::Texture>(std::move(image), std::move(sampler)));
    base::log::debug("Created texture {} ({}x{})", handle.value_, texture.width_, texture.height_);
    return handle;
}

void Renderer::destroy_texture(TextureHandle texture) {
    if (impl_->textures_.erase(texture.value_) == 0) {
        base::log::error("Invalid texture handle: {}", texture.value_);
        throw std::invalid_argument("Invalid texture handle");
    }
    base::log::debug("Destroyed texture {}", texture.value_);
}

void Renderer::render(const RenderData& data, gfx::RenderTarget* target) {

    gfx::DebugScope scope("Renderer::render");

    std::vector<DrawItem> items;
    items.reserve(data.items_.size());
    for (const auto& item: data.items_) {
        if (item.kind_ == RenderItemKind::Sprite) {
            const gfx::Texture* texture = nullptr;
            if (item.sprite_.texture_.value_ != 0) {
                const auto found = impl_->textures_.find(item.sprite_.texture_.value_);
                if (found == impl_->textures_.end()) {
                    base::log::error("Sprite render item references an invalid texture: {}", item.sprite_.texture_.value_);
                    throw std::invalid_argument("Sprite render item references an invalid texture");
                }
                texture = found->second.get();
            }

            items.push_back(DrawItem{
                .kind_            = RenderItemKind::Sprite,
                .world_transform_ = item.world_transform_,
                .sprite_          = Sprite{ .texture_       = texture,
                                           .color_         = item.sprite_.color_,
                                           .uv_rect_       = item.sprite_.uv_rect_,
                                           .pivot_         = item.sprite_.pivot_,
                                           .flip_x_        = item.sprite_.flip_x_,
                                           .flip_y_        = item.sprite_.flip_y_,
                                           .sorting_order_ = item.sprite_.sorting_order_ }
            });
            continue;
        }

        const auto mesh     = impl_->meshes_.find(item.mesh_.value_);
        const auto material = impl_->materials_.find(item.material_.value_);
        if (mesh == impl_->meshes_.end() || material == impl_->materials_.end()) {
            base::log::error("Render item references an invalid resource (mesh: {}, material: {})", item.mesh_.value_,
                             item.material_.value_);
            throw std::invalid_argument("Render item references an invalid resource");
        }
        items.push_back(DrawItem{ .kind_            = RenderItemKind::Mesh,
                                  .world_transform_ = item.world_transform_,
                                  .mesh_            = mesh->second.get(),
                                  .material_        = &material->second });
    }

    std::stable_sort(items.begin(), items.end(), [](const DrawItem& a, const DrawItem& b) {
        const bool a_sprite = a.kind_ == RenderItemKind::Sprite;
        const bool b_sprite = b.kind_ == RenderItemKind::Sprite;
        if (a_sprite != b_sprite) return !a_sprite;
        return a_sprite && a.sprite_.sorting_order_ < b.sprite_.sorting_order_;
    });

    impl_->pipeline_.render(data, target, items);
}

} // namespace vulcal::render
