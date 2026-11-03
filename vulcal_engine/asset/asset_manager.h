#pragma once

#include "asset/asset_id.h"
#include "render/resources/material.h"
#include "render/resources/mesh_data.h"

#include <filesystem>
#include <memory>
#include <unordered_map>
#include <unordered_set>

namespace vulcal::asset {

// Caches immutable CPU data; does not require a Renderer or graphics context.
class AssetManager {
public:
    explicit AssetManager(std::filesystem::path directory = {});
    AssetManager(const AssetManager&)            = delete;
    AssetManager& operator=(const AssetManager&) = delete;

    std::shared_ptr<const MeshData> load_mesh(AssetID id);
    // Nil selects the shared default. Other IDs must have been registered.
    std::shared_ptr<const render::Material> load_material(AssetID id) const;

    // Procedural IDs are valid until invalidation, clear, or directory change.
    AssetID add_mesh(MeshData mesh);
    AssetID add_material(render::Material material);

    void invalidate(AssetID id);
    void clear();
    void set_directory(std::filesystem::path directory);

private:
    std::filesystem::path directory_;
    std::unordered_map<AssetID, std::shared_ptr<const MeshData>> meshes_;
    std::unordered_map<AssetID, std::shared_ptr<const render::Material>> materials_;
    std::unordered_set<AssetID> failed_meshes_;
    std::shared_ptr<const render::Material> default_material_;
};

} // namespace vulcal::asset
