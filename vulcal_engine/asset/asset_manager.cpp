#include "asset/asset_manager.h"

#include "asset/runtime_asset_library.h"
#include "base/logger.h"

#include <exception>
#include <utility>

namespace vulcal::asset {

AssetManager::AssetManager(std::filesystem::path directory)
        : directory_(std::move(directory)),
          default_material_(std::make_shared<const render::Material>()) {
    if (!directory_.empty()) base::log::info("AssetManager: runtime directory is {}", directory_.string());
}

std::shared_ptr<const MeshData> AssetManager::load_mesh(AssetID id) {
    if (id.is_nil()) return {};
    if (const auto it = meshes_.find(id); it != meshes_.end()) return it->second;
    if (directory_.empty() || failed_meshes_.contains(id)) return {};

    try {
        auto data = std::make_shared<const MeshData>(RuntimeAssetLibrary{ directory_ }.load_mesh(id));
        meshes_.emplace(id, data);
        base::log::debug("AssetManager: loaded mesh {} from {} ({} vertices, {} indices)", id.to_string(), directory_.string(),
                         data->vertices_.size(), data->indices_.size());
        return data;
    } catch (const std::exception& error) {
        failed_meshes_.insert(id);
        base::log::error("AssetManager: could not load mesh {} from {}: {} (failure cached until invalidation)", id.to_string(),
                         directory_.string(), error.what());
        return {};
    }
}

std::shared_ptr<const render::Material> AssetManager::load_material(AssetID id) const {
    if (id.is_nil()) return default_material_;
    if (const auto it = materials_.find(id); it != materials_.end()) return it->second;
    return {};
}

AssetID AssetManager::add_mesh(MeshData mesh) {
    const auto id = AssetID::generate();
    auto data     = std::make_shared<const MeshData>(std::move(mesh));
    meshes_.emplace(id, data);
    base::log::debug("AssetManager: registered procedural mesh {} ({} vertices, {} indices)", id.to_string(), data->vertices_.size(),
                     data->indices_.size());
    return id;
}

AssetID AssetManager::add_material(render::Material material) {
    const auto id = AssetID::generate();
    materials_.emplace(id, std::make_shared<const render::Material>(std::move(material)));
    base::log::debug("AssetManager: registered material {}", id.to_string());
    return id;
}

void AssetManager::invalidate(AssetID id) {
    const auto meshes    = meshes_.erase(id);
    const auto materials = materials_.erase(id);
    const auto failures  = failed_meshes_.erase(id);
    if (meshes || materials || failures) {
        base::log::debug("AssetManager: invalidated asset {} ({} meshes, {} materials, {} cached failures)", id.to_string(), meshes,
                         materials, failures);
    }
}

void AssetManager::clear() {
    if (!meshes_.empty() || !materials_.empty() || !failed_meshes_.empty()) {
        base::log::debug("AssetManager: clearing CPU cache ({} meshes, {} materials, {} cached failures)", meshes_.size(),
                         materials_.size(), failed_meshes_.size());
    }
    meshes_.clear();
    materials_.clear();
    failed_meshes_.clear();
}

void AssetManager::set_directory(std::filesystem::path directory) {
    clear();
    directory_ = std::move(directory);
    base::log::info("AssetManager: runtime directory set to {}", directory_.empty() ? "<memory only>" : directory_.string());
}

} // namespace vulcal::asset
