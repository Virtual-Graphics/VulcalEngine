#pragma once

#include "asset/asset_id.h"
#include "render/resources/material.h"
#include "render/resources/mesh_data.h"
#include "render/resources/resources_handle.h"
#include "render/resources/texture_data.h"

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
    render::MeshHandle resolve_mesh(MeshAssetID id);
    render::MaterialHandle resolve_material(MaterialAssetID id);
    render::TextureHandle resolve_texture(TextureAssetID id);

    // Releases GPU handles; CPU data remains in AssetManager.
    void invalidate(MeshAssetID id);
    void invalidate(MaterialAssetID id);
    void invalidate(TextureAssetID id);
    void clear();

private:
    struct MeshEntry {
        std::shared_ptr<const render::MeshData> source_;
        render::MeshHandle handle_;
    };
    struct MaterialEntry {
        std::shared_ptr<const render::Material> source_;
        render::MaterialHandle handle_;
    };
    struct TextureEntry {
        std::shared_ptr<const render::TextureData> source_;
        render::TextureHandle handle_;
    };

    AssetManager& assets_;
    render::Renderer& renderer_;
    std::unordered_map<MeshAssetID, MeshEntry> meshes_;
    std::unordered_map<MaterialAssetID, MaterialEntry> materials_;
    std::unordered_map<TextureAssetID, TextureEntry> textures_;
};

} // namespace vulcal::asset
