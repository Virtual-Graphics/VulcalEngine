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

// Assets 与 Renderer 的适配层，拥有 AssetID 到 Renderer Resource Handle 的映射
// Renderer 缓存了创建过的 GPU 资源，通过 Resource Handle 索引，而在 Scene 中我们使用 AssetID 索引 Assets
// 所以我们需要一张 AssetID -> GPU Handle 的登记表。
class GPUResourceRegistry {
public:
    GPUResourceRegistry(AssetManager& assets, render::Renderer& renderer);
    ~GPUResourceRegistry();
    GPUResourceRegistry(const GPUResourceRegistry&)            = delete;
    GPUResourceRegistry& operator=(const GPUResourceRegistry&) = delete;

    // Returned handles are borrowed; callers must not destroy them directly.
    render::MeshHandle resolve(MeshAssetID id);
    render::MaterialHandle resolve(MaterialAssetID id);
    render::TextureHandle resolve(TextureAssetID id);

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
