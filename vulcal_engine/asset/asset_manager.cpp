#include "asset/asset_manager.h"

#include "base/logger.h"

#include <exception>
#include <string_view>
#include <utility>

namespace vulcal::asset {
namespace {

template<typename T, typename Loader>
std::shared_ptr<const T> load_runtime_asset(AssetCache<T>& cache, const std::filesystem::path& directory, AssetID id, std::string_view type,
                                            Loader&& loader) {
    if (id.is_nil()) return {};
    if (auto data = cache.find(id)) return data;
    if (directory.empty() || cache.failed(id)) return {};

    try {
        auto data = std::make_shared<const T>(std::forward<Loader>(loader)());
        cache.store(id, data);
        base::log::debug("AssetManager: loaded {} {} from {}", type, id.to_string(), directory.string());
        return data;
    } catch (const std::exception& error) {
        cache.mark_failed(id);
        base::log::error("AssetManager: could not load {} {} from {}: {} (failure cached until invalidation)", type, id.to_string(),
                         directory.string(), error.what());
        return {};
    }
}

} // namespace

AssetManager::AssetManager(std::filesystem::path directory)
        : library_(std::move(directory)),
          default_material_(std::make_shared<const render::Material>()) {
    if (!library_.directory().empty()) base::log::info("AssetManager: runtime directory is {}", library_.directory().string());
}

std::shared_ptr<const render::MeshData> AssetManager::load_mesh(AssetID id) {
    return load_runtime_asset<render::MeshData>(meshes_, library_.directory(), id, "mesh", [&] { return library_.load_mesh(id); });
}

std::shared_ptr<const render::Material> AssetManager::load_material(AssetID id) {
    if (id.is_nil()) return default_material_;
    return load_runtime_asset<render::Material>(materials_, library_.directory(), id, "material",
                                                [&] { return library_.load_material(id); });
}

AssetID AssetManager::add_mesh(render::MeshData mesh) {
    const auto id = AssetID::generate();
    auto data     = std::make_shared<const render::MeshData>(std::move(mesh));
    meshes_.store(id, data);
    base::log::debug("AssetManager: registered procedural mesh {} ({} vertices, {} indices)", id.to_string(), data->vertices_.size(),
                     data->indices_.size());
    return id;
}

AssetID AssetManager::add_material(render::Material material) {
    const auto id = AssetID::generate();
    materials_.store(id, std::make_shared<const render::Material>(std::move(material)));
    base::log::debug("AssetManager: registered material {}", id.to_string());
    return id;
}

void AssetManager::invalidate(AssetID id) {
    const bool mesh     = meshes_.erase(id);
    const bool material = materials_.erase(id);
    if (mesh || material) base::log::debug("AssetManager: invalidated asset {} (mesh {}, material {})", id.to_string(), mesh, material);
}

void AssetManager::clear() {
    if (!meshes_.empty() || !materials_.empty()) {
        base::log::debug("AssetManager: clearing CPU cache ({} meshes, {} materials, {} cached failures)", meshes_.size(),
                         materials_.size(), meshes_.failure_count() + materials_.failure_count());
    }
    meshes_.clear();
    materials_.clear();
}

void AssetManager::set_directory(std::filesystem::path directory) {
    clear();
    library_ = AssetLibrary{ std::move(directory) };
    base::log::info("AssetManager: runtime directory set to {}",
                    library_.directory().empty() ? "<memory only>" : library_.directory().string());
}

} // namespace vulcal::asset
