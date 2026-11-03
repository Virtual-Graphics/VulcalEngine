#pragma once

#include "asset/asset_id.h"
#include "render/resources/material.h"
#include "render/resources/mesh_data.h"
#include "render/resources/resources_handle.h"

#include <memory>
#include <unordered_map>

namespace vulcal::render {
class Renderer;
}

namespace vulcal::asset {

class AssetManager;

// Owns handles created for one Renderer. Both dependencies and the graphics
// context must outlive this cache. Resolve and release on the render thread.
class RenderAssetCache {
public:
    RenderAssetCache(AssetManager& assets, render::Renderer& renderer);
    ~RenderAssetCache();
    RenderAssetCache(const RenderAssetCache&)            = delete;
    RenderAssetCache& operator=(const RenderAssetCache&) = delete;

    // Returned handles are borrowed; callers must not destroy them directly.
    render::MeshHandle resolve_mesh(AssetID id);
    render::MaterialHandle resolve_material(AssetID id);

    // Releases GPU handles; CPU data remains in AssetManager.
    void invalidate(AssetID id);
    void clear();

private:
    struct MeshEntry {
        std::shared_ptr<const MeshData> source_;
        render::MeshHandle handle_;
    };
    struct MaterialEntry {
        std::shared_ptr<const render::Material> source_;
        render::MaterialHandle handle_;
    };

    AssetManager& assets_;
    render::Renderer& renderer_;
    std::unordered_map<AssetID, MeshEntry> meshes_;
    std::unordered_map<AssetID, MaterialEntry> materials_;
};

} // namespace vulcal::asset
