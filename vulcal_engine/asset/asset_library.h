#pragma once

#include "asset/asset_id.h"
#include "render/resources/material.h"
#include "render/resources/mesh_data.h"

#include <filesystem>

namespace vulcal::asset {

// Reads cooked runtime assets from one library directory. Owns the on-disk
// naming convention; AssetManager adds caching on top of it.
class AssetLibrary {
public:
    explicit AssetLibrary(std::filesystem::path directory);

    render::MeshData load_mesh(AssetID id) const;
    render::Material load_material(AssetID id) const;
    std::filesystem::path mesh_path(AssetID id) const;
    std::filesystem::path material_path(AssetID id) const;

    const std::filesystem::path& directory() const { return directory_; }

private:
    std::filesystem::path directory_;
};

} // namespace vulcal::asset
