#include "asset/asset_manager.h"

#include "base/profiler.h"

#include "base/logger.h"

#include <exception>
#include <string_view>
#include <utility>

namespace vulcal::asset {
namespace {

template<typename T, typename ID, typename Loader>
std::shared_ptr<const T> load_cooked_asset(AssetCache<T, ID>& cache,
                                           const std::filesystem::path& directory,
                                           ID id, std::string_view type,
                                           bool has_provider, Loader&& loader) {
    if (id.is_nil()) {
        return {};
    }
    if (auto data = cache.find(id)) {
        return data;
    }
    if (!has_provider || cache.failed(id)) {
        return {};
    }

    VULCAL_PROFILE_SCOPE_NAMED("Load asset from disk");
    VULCAL_PROFILE_TEXT(type.data(), type.size());
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

AssetManager::AssetManager(std::filesystem::path assets_lib_dir)
        : library_(std::move(assets_lib_dir)),
          default_material_(std::make_shared<const render::Material>()) {
    if (library_.has_provider()) base::log::info("AssetManager: assets cooked library directory is {}", library_.directory().string());
}

AssetManager::AssetManager(std::shared_ptr<const resource::ResourceProvider> provider, std::string logical_dir_for_lib)
        : library_(std::move(provider), std::move(logical_dir_for_lib)),
          default_material_(std::make_shared<const render::Material>()) {}

std::shared_ptr<const render::MeshData> AssetManager::load_mesh(MeshAssetID id) {
    return load_cooked_asset<render::MeshData>(meshes_, library_.directory(), id, "mesh", library_.has_provider(),
                                                [&] { return library_.load_mesh(id); });
}

std::shared_ptr<const render::Material> AssetManager::load_material(MaterialAssetID id) {
    if (id.is_nil()) return default_material_;
    return load_cooked_asset<render::Material>(materials_, library_.directory(), id, "material", library_.has_provider(),
                                                [&] { return library_.load_material(id); });
}

std::shared_ptr<const render::TextureData> AssetManager::load_texture(TextureAssetID id) {
    return load_cooked_asset<render::TextureData>(textures_, library_.directory(), id, "texture", library_.has_provider(),
                                                   [&] { return library_.load_texture(id); });
}

MeshAssetID AssetManager::add_mesh(render::MeshData mesh) {
    const auto id = MeshAssetID::generate();
    auto data = std::make_shared<const render::MeshData>(std::move(mesh));
    meshes_.store(id, data);
    base::log::debug("AssetManager: registered procedural mesh {} ({} vertices, {} indices)", id.to_string(), data->vertices_.size(),
                     data->indices_.size());
    return id;
}

MaterialAssetID AssetManager::add_material(render::Material material) {
    const auto id = MaterialAssetID::generate();
    materials_.store(id, std::make_shared<const render::Material>(std::move(material)));
    base::log::debug("AssetManager: registered material {}", id.to_string());
    return id;
}

TextureAssetID AssetManager::add_texture(render::TextureData texture) {
    const auto id = TextureAssetID::generate();
    auto data     = std::make_shared<const render::TextureData>(std::move(texture));
    textures_.store(id, data);
    base::log::debug("AssetManager: registered procedural texture {} ({}x{})", id.to_string(), data->width_, data->height_);
    return id;
}

void AssetManager::invalidate(MeshAssetID id) {
    if (meshes_.erase(id)) {
        base::log::debug("AssetManager: invalidated mesh {}", id.to_string());
    }
}

void AssetManager::invalidate(MaterialAssetID id) {
    if (materials_.erase(id)) {
        base::log::debug("AssetManager: invalidated material {}", id.to_string());
    }
}

void AssetManager::invalidate(TextureAssetID id) {
    if (textures_.erase(id)) {
        base::log::debug("AssetManager: invalidated texture {}", id.to_string());
    }
}

void AssetManager::clear() {
    if (!meshes_.empty() || !materials_.empty() || !textures_.empty()) {
        base::log::debug("AssetManager: clearing CPU cache ({} meshes, {} materials, {} textures, {} cached failures)", meshes_.size(),
                         materials_.size(), textures_.size(),
                         meshes_.failure_count() + materials_.failure_count() + textures_.failure_count());
    }
    meshes_.clear();
    materials_.clear();
    textures_.clear();
}

void AssetManager::set_library(std::filesystem::path assets_lib_dir) {
    clear();
    library_ = AssetLibrary{ std::move(assets_lib_dir) };

    base::log::info("AssetManager: assets cooked library directory set to {}",
                    library_.directory().empty() ? "<memory only>" : library_.directory().string());
}

void AssetManager::set_provider(std::shared_ptr<const resource::ResourceProvider> provider, std::string logical_dir_for_lib) {
    clear();
    library_ = AssetLibrary{ std::move(provider), std::move(logical_dir_for_lib) };

    base::log::info("AssetManager: assets cooked library directory set to {}",
                    library_.directory().empty() ? "<resource provider>" : library_.directory().string());
}

} // namespace vulcal::asset
