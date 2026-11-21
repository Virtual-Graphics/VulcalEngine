#include "asset/asset_library.h"

#include "asset/res/material_asset.h"
#include "asset/res/mesh_asset.h"
#include "asset/res/texture_asset.h"
#include "base/logger.h"
#include "resource/directory_provider.h"
#include "resource/resource_provider.h"

#include <memory>
#include <stdexcept>
#include <utility>

namespace vulcal::asset {

AssetLibrary::AssetLibrary(std::filesystem::path directory)
        : directory_(std::move(directory)) {
    std::error_code error;
    if (!directory_.empty() && std::filesystem::is_directory(directory_, error) && !error) {
        provider_ = std::make_shared<resource::DirectoryProvider>(directory_);
    }
}

AssetLibrary::AssetLibrary(std::shared_ptr<const resource::ResourceProvider> provider, std::string logical_directory)
        : provider_(std::move(provider)),
          logical_directory_(std::move(logical_directory)) {
    while (!logical_directory_.empty() && logical_directory_.back() == '/') {
        logical_directory_.pop_back();
    }
}

std::filesystem::path AssetLibrary::mesh_path(MeshAssetID id) const { return directory_ / (id.to_string() + ".vmesh"); }
std::filesystem::path AssetLibrary::material_path(MaterialAssetID id) const { return directory_ / (id.to_string() + ".vmatc"); }
std::filesystem::path AssetLibrary::texture_path(TextureAssetID id) const { return directory_ / (id.to_string() + ".vtex"); }

render::MeshData AssetLibrary::load_mesh(MeshAssetID id) const {
    if (id.is_nil()) {
        throw std::invalid_argument("Cannot load a mesh with an empty asset ID");
    }
    if (!provider_) {
        throw std::runtime_error("No resource provider is configured for runtime assets");
    }

    const auto logical_path = logical_directory_.empty() ? id.to_string() + ".vmesh" : logical_directory_ + "/" + id.to_string() + ".vmesh";
    base::log::debug("AssetLibrary: loading mesh {} from {}", id.to_string(), logical_path);
    return MeshAsset::read(provider_->read_binary(logical_path));
}

render::Material AssetLibrary::load_material(MaterialAssetID id) const {
    if (id.is_nil()) {
        throw std::invalid_argument("Cannot load a material with an empty asset ID");
    }
    if (!provider_) {
        throw std::runtime_error("No resource provider is configured for runtime assets");
    }

    const auto logical_path = logical_directory_.empty() ? id.to_string() + ".vmatc" : logical_directory_ + "/" + id.to_string() + ".vmatc";
    base::log::debug("AssetLibrary: loading material {} from {}", id.to_string(), logical_path);
    return MaterialAsset::read(provider_->read_binary(logical_path));
}

render::TextureData AssetLibrary::load_texture(TextureAssetID id) const {
    if (id.is_nil()) {
        throw std::invalid_argument("Cannot load a texture with an empty asset ID");
    }
    if (!provider_) {
        throw std::runtime_error("No resource provider is configured for runtime assets");
    }

    const auto logical_path = logical_directory_.empty() ? id.to_string() + ".vtex" : logical_directory_ + "/" + id.to_string() + ".vtex";
    base::log::debug("AssetLibrary: loading texture {} from {}", id.to_string(), logical_path);
    return TextureAsset::read(provider_->read_binary(logical_path));
}

} // namespace vulcal::asset
