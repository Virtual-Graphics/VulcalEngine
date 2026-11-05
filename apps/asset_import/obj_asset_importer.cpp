#include "asset_import/obj_asset_importer.h"

#include "asset/mesh_asset.h"
#include "asset_import/meta_data.h"
#include "asset_import/obj_loader.h"
#include "base/logger.h"

#include <stdexcept>

namespace apps::asset_import {

vulcal::asset::AssetID ObjAssetImporter::import(const std::filesystem::path& source_file, const std::filesystem::path& assets_directory,
                                                const std::filesystem::path& library_directory) {

    base::log::debug("ObjAssetImporter: importing {} (assets: {}, runtime: {})", source_file.string(), assets_directory.string(),
                     library_directory.string());
    // 预处理 begin
    validate_source_file(source_file, assets_directory, ".obj", "OBJ");

    const auto mesh = OBJImporter::load_obj(source_file);
    if (mesh.vertices_.empty() || mesh.indices_.empty()) {
        throw std::runtime_error("OBJ file contains no renderable mesh data");
    }
    // 预处理 end

    const auto meta         = metadata_path(source_file);
    const auto has_metadata = std::filesystem::exists(meta);
    const auto metadata     = metadata_for_source(source_file, vulcal::asset::AssetType::Mesh);
    const auto id           = metadata.id;

    std::filesystem::create_directories(library_directory);
    const auto runtime_file = library_directory / (id.to_string() + ".vmesh");
    vulcal::asset::MeshAsset::write(runtime_file, mesh);
    if (!has_metadata) {
        write_metadata(meta, metadata);
    }

    base::log::info("ObjAssetImporter: {} {} as asset {} -> {} ({} vertices, {} indices)", has_metadata ? "reimported" : "imported",
                    source_file.string(), id.to_string(), runtime_file.string(), mesh.vertices_.size(), mesh.indices_.size());
    return id;
}

} // namespace apps::asset_import
