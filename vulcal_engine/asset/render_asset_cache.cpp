#include "asset/render_asset_cache.h"

#include "asset/asset_manager.h"
#include "base/logger.h"
#include "render/renderer.h"

#include <utility>

namespace vulcal::asset {

RenderAssetCache::RenderAssetCache(AssetManager& assets, render::Renderer& renderer)
        : assets_(assets),
          renderer_(renderer) {}

RenderAssetCache::~RenderAssetCache() { clear(); }

render::MeshHandle RenderAssetCache::resolve_mesh(AssetID id) {
    auto source = assets_.load_mesh(id);
    auto it     = meshes_.find(id);
    if (it != meshes_.end()) {
        if (source == it->second.source_) return it->second.handle_;
        base::log::debug("RenderAssetCache: releasing stale mesh {} (handle {})", id.to_string(), it->second.handle_.value_);
        renderer_.destroy_mesh(it->second.handle_);
        meshes_.erase(it);
    }
    if (!source) return {};

    const auto handle = renderer_.create_mesh(*source);
    try {
        meshes_.emplace(id, MeshEntry{ std::move(source), handle });
    } catch (...) {
        renderer_.destroy_mesh(handle);
        throw;
    }
    base::log::debug("RenderAssetCache: cached mesh {} as handle {}", id.to_string(), handle.value_);
    return handle;
}

render::MaterialHandle RenderAssetCache::resolve_material(AssetID id) {
    auto source = assets_.load_material(id);
    auto it     = materials_.find(id);
    if (it != materials_.end()) {
        if (source == it->second.source_) return it->second.handle_;
        base::log::debug("RenderAssetCache: releasing stale material {} (handle {})", id.to_string(), it->second.handle_.value_);
        renderer_.destroy_material(it->second.handle_);
        materials_.erase(it);
    }
    if (!source) return {};

    const auto handle = renderer_.create_material(*source);
    try {
        materials_.emplace(id, MaterialEntry{ std::move(source), handle });
    } catch (...) {
        renderer_.destroy_material(handle);
        throw;
    }
    base::log::debug("RenderAssetCache: cached material {} as handle {}", id.to_string(), handle.value_);
    return handle;
}

void RenderAssetCache::invalidate(AssetID id) {
    if (const auto it = meshes_.find(id); it != meshes_.end()) {
        base::log::debug("RenderAssetCache: invalidating mesh {} (handle {})", id.to_string(), it->second.handle_.value_);
        renderer_.destroy_mesh(it->second.handle_);
        meshes_.erase(it);
    }
    if (const auto it = materials_.find(id); it != materials_.end()) {
        base::log::debug("RenderAssetCache: invalidating material {} (handle {})", id.to_string(), it->second.handle_.value_);
        renderer_.destroy_material(it->second.handle_);
        materials_.erase(it);
    }
}

void RenderAssetCache::clear() {
    if (!meshes_.empty() || !materials_.empty()) {
        base::log::debug("RenderAssetCache: releasing GPU cache ({} meshes, {} materials)", meshes_.size(), materials_.size());
    }
    for (const auto& [id, entry]: meshes_) renderer_.destroy_mesh(entry.handle_);
    for (const auto& [id, entry]: materials_) renderer_.destroy_material(entry.handle_);
    meshes_.clear();
    materials_.clear();
}

} // namespace vulcal::asset
