#include "asset/asset_library.h"

#include "asset/res/material_asset.h"
#include "asset/res/mesh_asset.h"
#include "asset/res/texture_asset.h"
#include "base/logger.h"

#include <stdexcept>
#include <utility>

namespace vulcal::asset {

AssetLibrary::AssetLibrary(std::filesystem::path directory)
        : directory_(std::move(directory)) {}

std::filesystem::path AssetLibrary::mesh_path(AssetID id) const { return directory_ / (id.to_string() + ".vmesh"); }
std::filesystem::path AssetLibrary::material_path(AssetID id) const { return directory_ / (id.to_string() + ".vmatc"); }
std::filesystem::path AssetLibrary::texture_path(AssetID id) const { return directory_ / (id.to_string() + ".vtex"); }

render::MeshData AssetLibrary::load_mesh(AssetID id) const {
    if (id.is_nil()) {
        throw std::invalid_argument("Cannot load a mesh with an empty asset ID");
    }
    const auto path = mesh_path(id);
    base::log::debug("AssetLibrary: loading mesh {} from {}", id.to_string(), path.string());
    return MeshAsset::read(path);
}

render::Material AssetLibrary::load_material(AssetID id) const {
    if (id.is_nil()) throw std::invalid_argument("Cannot load a material with an empty asset ID");
    const auto path = material_path(id);
    base::log::debug("AssetLibrary: loading material {} from {}", id.to_string(), path.string());
    return MaterialAsset::read(path);
}

render::TextureData AssetLibrary::load_texture(AssetID id) const {
    if (id.is_nil()) throw std::invalid_argument("Cannot load a texture with an empty asset ID");
    const auto path = texture_path(id);
    base::log::debug("AssetLibrary: loading texture {} from {}", id.to_string(), path.string());
    return TextureAsset::read(path);
}

} // namespace vulcal::asset
