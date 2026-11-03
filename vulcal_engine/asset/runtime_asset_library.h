#pragma once

#include "asset/asset_id.h"
#include "render/resources/mesh_data.h"

#include <filesystem>

namespace vulcal::asset {

class RuntimeAssetLibrary {
public:
    explicit RuntimeAssetLibrary(std::filesystem::path directory);

    MeshData load_mesh(AssetID id) const;
    std::filesystem::path mesh_path(AssetID id) const;

private:
    std::filesystem::path directory_;
};

} // namespace vulcal::asset
