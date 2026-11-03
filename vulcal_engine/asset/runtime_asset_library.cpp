#include "asset/runtime_asset_library.h"

#include "asset/runtime_mesh_asset.h"
#include "base/logger.h"

#include <stdexcept>
#include <utility>

namespace vulcal::asset {

RuntimeAssetLibrary::RuntimeAssetLibrary(std::filesystem::path directory)
        : directory_(std::move(directory)) {}

std::filesystem::path RuntimeAssetLibrary::mesh_path(AssetID id) const { return directory_ / (id.to_string() + ".vmesh"); }

MeshData RuntimeAssetLibrary::load_mesh(AssetID id) const {
    if (id.is_nil()) {
        throw std::invalid_argument("Cannot load a mesh with an empty asset ID");
    }
    const auto path = mesh_path(id);
    base::log::debug("RuntimeAssetLibrary: loading mesh {} from {}", id.to_string(), path.string());
    return RuntimeMeshAsset::read(path);
}

} // namespace vulcal::asset
