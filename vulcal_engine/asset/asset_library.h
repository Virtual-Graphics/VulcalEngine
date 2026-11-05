#pragma once

#include "asset/asset_id.h"
#include "render/resources/material.h"
#include "render/resources/mesh_data.h"

#include <filesystem>

namespace vulcal::asset {

class AssetLibrary {
public:
    explicit AssetLibrary(std::filesystem::path directory);

    render::MeshData load_mesh(AssetID id) const;
    render::Material load_material(AssetID id) const;
    std::filesystem::path mesh_path(AssetID id) const;
    std::filesystem::path material_path(AssetID id) const;

private:
    std::filesystem::path directory_;
};

} // namespace vulcal::asset
