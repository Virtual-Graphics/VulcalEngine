#pragma once

#include "asset/asset_cache.h"
#include "asset/asset_id.h"
#include "asset/asset_library.h"
#include "render/resources/material.h"
#include "render/resources/mesh_data.h"
#include "render/resources/texture_data.h"

#include <filesystem>
#include <memory>
#include <string>

namespace vulcal::resource {
class ResourceProvider;
}

namespace vulcal::asset {

// Caches immutable CPU data; does not require a Renderer or graphics context.
class AssetManager {
public:
    explicit AssetManager(std::filesystem::path assets_lib_dir = {});
    explicit AssetManager(std::shared_ptr<const resource::ResourceProvider> provider, std::string logical_dir_for_lib = {});
    AssetManager(const AssetManager&)            = delete;
    AssetManager& operator=(const AssetManager&) = delete;

    // Nil selects the shared default. Other IDs resolve from memory or the runtime library.
    std::shared_ptr<const render::MeshData> load_mesh(MeshAssetID id);
    std::shared_ptr<const render::Material> load_material(MaterialAssetID id);
    std::shared_ptr<const render::TextureData> load_texture(TextureAssetID id);

    // Procedural IDs are valid until invalidation, clear, or directory change.
    MeshAssetID add_mesh(render::MeshData mesh);
    MaterialAssetID add_material(render::Material material);
    TextureAssetID add_texture(render::TextureData texture);

    void invalidate(MeshAssetID id);
    void invalidate(MaterialAssetID id);
    void invalidate(TextureAssetID id);
    void clear();
    void set_library(std::filesystem::path assets_lib_dir);
    void set_provider(std::shared_ptr<const resource::ResourceProvider> provider, std::string logical_dir_for_lib = {});

private:
    AssetLibrary library_;
    AssetCache<render::MeshData, MeshAssetID> meshes_;
    AssetCache<render::Material, MaterialAssetID> materials_;
    AssetCache<render::TextureData, TextureAssetID> textures_;
    std::shared_ptr<const render::Material> default_material_;
};

} // namespace vulcal::asset
