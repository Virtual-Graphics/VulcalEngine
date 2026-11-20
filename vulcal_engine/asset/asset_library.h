#pragma once

#include "asset/asset_id.h"
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

// Reads cooked runtime assets from one library directory. Owns the on-disk
// naming convention; AssetManager adds caching on top of it.
class AssetLibrary {
public:
    explicit AssetLibrary(std::filesystem::path directory);
    explicit AssetLibrary(std::shared_ptr<const resource::ResourceProvider> provider, std::string logical_directory = {});

    render::MeshData load_mesh(MeshAssetID id) const;
    render::Material load_material(MaterialAssetID id) const;
    render::TextureData load_texture(TextureAssetID id) const;
    std::filesystem::path mesh_path(MeshAssetID id) const;
    std::filesystem::path material_path(MaterialAssetID id) const;
    std::filesystem::path texture_path(TextureAssetID id) const;

    const std::filesystem::path& directory() const { return directory_; }
    bool has_provider() const { return static_cast<bool>(provider_); }

private:
    std::filesystem::path directory_;
    std::shared_ptr<const resource::ResourceProvider> provider_;
    std::string logical_directory_;
};

} // namespace vulcal::asset
