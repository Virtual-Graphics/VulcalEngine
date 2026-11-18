#pragma once

#include "asset/asset_id.h"
#include "render/resources/material.h"
#include "render/resources/mesh_data.h"
#include "render/resources/texture_data.h"

#include <filesystem>

namespace vulcal::asset {

// Reads cooked runtime assets from one library directory. Owns the on-disk
// naming convention; AssetManager adds caching on top of it.
class AssetLibrary {
public:
    explicit AssetLibrary(std::filesystem::path directory);

    render::MeshData load_mesh(MeshAssetID id) const;
    render::Material load_material(MaterialAssetID id) const;
    render::TextureData load_texture(TextureAssetID id) const;
    std::filesystem::path mesh_path(MeshAssetID id) const;
    std::filesystem::path material_path(MaterialAssetID id) const;
    std::filesystem::path texture_path(TextureAssetID id) const;

    const std::filesystem::path& directory() const { return directory_; }

private:
    std::filesystem::path directory_;
};

} // namespace vulcal::asset
