#include "asset/gpu_resource_registry.h"

#include "base/profiler.h"

#include "asset/asset_manager.h"
#include "base/logger.h"
#include "render/renderer.h"

#include <utility>

namespace vulcal::asset {

namespace {

template<typename ID, typename Map, typename Load, typename Create, typename MakeEntry, typename Destroy>
auto resolve_entry(Map& entries, ID id, const char* kind, const char* profile_scope,
                   Load&& load, Create&& create, MakeEntry&& make_entry, Destroy&& destroy) {
    auto source = load();
    auto it     = entries.find(id);
    if (it != entries.end()) {
        if (source == it->second.source_) {
            return it->second.handle_;
        }

        base::log::debug("GPUResourceRegistry: releasing stale {} {} (handle {})", kind, id.to_string(),
                         it->second.handle_.value_);
        destroy(it->second.handle_);
        entries.erase(it);
    }
    if (!source) {
        return decltype(create(*source)){};
    }

    VULCAL_PROFILE_SCOPE_DYNAMIC(profile_scope);
    const auto handle = create(*source);
    try {
        entries.emplace(id, make_entry(std::move(source), handle));
    } catch (...) {
        destroy(handle);
        throw;
    }
    base::log::debug("GPUResourceRegistry: cached {} {} as handle {}", kind, id.to_string(), handle.value_);
    return handle;
}

template<typename ID, typename Map, typename Destroy>
void invalidate_entry(Map& entries, ID id, const char* kind, Destroy&& destroy) {
    if (const auto it = entries.find(id); it != entries.end()) {
        base::log::debug("GPUResourceRegistry: invalidating {} {} (handle {})", kind, id.to_string(),
                         it->second.handle_.value_);
        destroy(it->second.handle_);
        entries.erase(it);
    }
}

template<typename Map, typename Destroy>
void release_all(Map& entries, Destroy&& destroy) {
    for (const auto& entry: entries) {
        destroy(entry.second.handle_);
    }
    entries.clear();
}

} // namespace

GPUResourceRegistry::GPUResourceRegistry(AssetManager& assets, render::Renderer& renderer)
        : assets_(assets),
          renderer_(renderer) {}

GPUResourceRegistry::~GPUResourceRegistry() { clear(); }

render::MeshHandle GPUResourceRegistry::resolve(MeshAssetID id) {
    return resolve_entry(
            meshes_, id, "mesh", "Upload mesh",
            [this, id] { return assets_.load_mesh(id); },
            [this](const auto& data) { return renderer_.create_mesh(data); },
            [](auto source, auto handle) { return MeshEntry{ std::move(source), handle }; },
            [this](auto handle) { renderer_.destroy_mesh(handle); });
}

render::MaterialHandle GPUResourceRegistry::resolve(MaterialAssetID id) {
    return resolve_entry(
            materials_, id, "material", "Upload material",
            [this, id] { return assets_.load_material(id); },
            [this](const auto& data) { return renderer_.create_material(data); },
            [](auto source, auto handle) { return MaterialEntry{ std::move(source), handle }; },
            [this](auto handle) { renderer_.destroy_material(handle); });
}

render::TextureHandle GPUResourceRegistry::resolve(TextureAssetID id) {
    return resolve_entry(
            textures_, id, "texture", "Upload texture",
            [this, id] { return assets_.load_texture(id); },
            [this](const auto& data) { return renderer_.create_texture(data); },
            [](auto source, auto handle) { return TextureEntry{ std::move(source), handle }; },
            [this](auto handle) { renderer_.destroy_texture(handle); });
}

void GPUResourceRegistry::invalidate(MeshAssetID id) {
    invalidate_entry(meshes_, id, "mesh", [this](auto handle) { renderer_.destroy_mesh(handle); });
}

void GPUResourceRegistry::invalidate(MaterialAssetID id) {
    invalidate_entry(materials_, id, "material", [this](auto handle) { renderer_.destroy_material(handle); });
}

void GPUResourceRegistry::invalidate(TextureAssetID id) {
    invalidate_entry(textures_, id, "texture", [this](auto handle) { renderer_.destroy_texture(handle); });
}

void GPUResourceRegistry::clear() {
    if (!meshes_.empty() || !materials_.empty() || !textures_.empty()) {
        base::log::debug("GPUResourceRegistry: releasing GPU cache ({} meshes, {} materials, {} textures)",
                          meshes_.size(), materials_.size(),textures_.size());
    }

    release_all(meshes_,    [this](auto handle) { renderer_.destroy_mesh(handle);     });
    release_all(textures_,  [this](auto handle) { renderer_.destroy_texture(handle);  });
    release_all(materials_, [this](auto handle) { renderer_.destroy_material(handle); });
}

} // namespace vulcal::asset
