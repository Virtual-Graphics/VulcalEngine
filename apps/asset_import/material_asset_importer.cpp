#include "asset_import/material_asset_importer.h"

#include "asset/material_asset.h"
#include "asset_import/meta_data.h"
#include "base/logger.h"
#include "base/yaml.h"

#include <stdexcept>
#include <string>
#include <yaml-cpp/yaml.h>

namespace apps::asset_import {
namespace {

constexpr std::uint32_t source_format_version = 1;

vulcal::render::Material read_source(const std::filesystem::path& path) {
    try {
        const auto root = YAML::LoadFile(path.string());
        if (!root.IsMap() || root["format_version"].as<std::uint32_t>() != source_format_version) {
            throw std::runtime_error("Unsupported material source file: " + path.string());
        }
        vulcal::render::Material material;
        material.diffuse_   = base::yaml::read_vec<3>(root, "diffuse");
        material.specular_  = base::yaml::read_vec<3>(root, "specular");
        material.shininess_ = root["shininess"].as<float>();
        return material;
    } catch (const YAML::Exception& error) {
        throw std::runtime_error("Invalid material source " + path.string() + ": " + error.what());
    }
}

} // namespace

vulcal::asset::AssetID MaterialAssetImporter::import(const std::filesystem::path& source_file,
                                                     const std::filesystem::path& assets_directory,
                                                     const std::filesystem::path& library_directory) {
    base::log::debug("MaterialAssetImporter: importing {} (assets: {}, runtime: {})", source_file.string(), assets_directory.string(),
                     library_directory.string());
    validate_source_file(source_file, assets_directory, ".vmat", "material");

    const auto material     = read_source(source_file);
    const auto meta         = metadata_path(source_file);
    const auto has_metadata = std::filesystem::exists(meta);
    const auto metadata     = metadata_for_source(source_file, vulcal::asset::AssetType::Material);

    const auto id           = metadata.id;
    std::filesystem::create_directories(library_directory);
    const auto runtime_file = library_directory / (id.to_string() + ".vmatc");
    vulcal::asset::MaterialAsset::write(runtime_file, material);
    if (!has_metadata) write_metadata(meta, metadata);

    base::log::info("MaterialAssetImporter: {} {} as asset {} -> {}", has_metadata ? "reimported" : "imported", source_file.string(),
                    id.to_string(), runtime_file.string());
    return id;
}

} // namespace apps::asset_import
