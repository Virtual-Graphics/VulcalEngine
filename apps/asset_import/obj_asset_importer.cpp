#include "asset_import/obj_asset_importer.h"

#include "asset_import/meta_data.h"
#include "asset_import/obj_loader.h"
#include "asset_import/runtime_mesh_asset_writer.h"
#include "base/logger.h"

#include <algorithm>
#include <cctype>
#include <stdexcept>
#include <string>

namespace apps::asset_import {
namespace {

void require_source_in_project(const std::filesystem::path& source_file, const std::filesystem::path& assets_directory) {
    const auto source   = std::filesystem::weakly_canonical(source_file);
    const auto assets   = std::filesystem::weakly_canonical(assets_directory);

    const auto relative = source.lexically_relative(assets);
    if (relative.empty() || relative.is_absolute() || *relative.begin() == "..") {
        throw std::invalid_argument("Import source must be inside the project's Assets directory");
    }
}

} // namespace

vulcal::asset::AssetID ObjAssetImporter::import(const std::filesystem::path& source_file, const std::filesystem::path& assets_directory,
                                                const std::filesystem::path& library_directory) {

    base::log::debug("ObjAssetImporter: importing {} (assets: {}, runtime: {})", source_file.string(), assets_directory.string(),
                     library_directory.string());
    // 预处理 begin
    std::error_code error;
    if (!std::filesystem::is_regular_file(source_file, error) || error) {
        throw std::invalid_argument("OBJ source file does not exist: " + source_file.string());
    }

    auto extension = source_file.extension().string();
    std::transform(extension.begin(), extension.end(), extension.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    if (extension != ".obj") {
        throw std::invalid_argument("Only OBJ files can be imported by this importer");
    }
    require_source_in_project(source_file, assets_directory);

    const auto mesh = OBJImporter::load_obj(source_file);
    if (mesh.vertices_.empty() || mesh.indices_.empty()) {
        throw std::runtime_error("OBJ file contains no renderable mesh data");
    }
    // 预处理 end

    const auto meta         = metadata_path(source_file);
    const auto has_metadata = std::filesystem::exists(meta);
    const auto metadata     = has_metadata ? read_metadata(meta) : Metadata{ .id = vulcal::asset::AssetID::generate() };
    const auto id           = metadata.id;

    std::filesystem::create_directories(library_directory);
    const auto runtime_file = library_directory / (id.to_string() + ".vmesh");
    RuntimeMeshAssetWriter::write(runtime_file, mesh);
    if (!has_metadata) {
        write_metadata(meta, metadata);
    }

    base::log::info("ObjAssetImporter: {} {} as asset {} -> {} ({} vertices, {} indices)", has_metadata ? "reimported" : "imported",
                    source_file.string(), id.to_string(), runtime_file.string(), mesh.vertices_.size(), mesh.indices_.size());
    return id;
}

} // namespace apps::asset_import
