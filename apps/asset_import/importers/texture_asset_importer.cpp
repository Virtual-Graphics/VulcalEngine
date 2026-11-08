#include "asset_import/importers/texture_asset_importer.h"

#include "asset/asset_library.h"
#include "asset/texture_asset.h"
#include "asset_import/loaders/image_loader.h"
#include "asset_import/meta_data.h"
#include "base/logger.h"
#include "base/string_utils.h"

#include <stdexcept>
#include <string>

namespace apps::asset_import {

vulcal::asset::AssetID TextureAssetImporter::import(const std::filesystem::path& source_file, const std::filesystem::path& assets_directory,
                                                    const std::filesystem::path& library_directory) {
    base::log::debug("TextureAssetImporter: importing {} (assets: {}, runtime: {})", source_file.string(), assets_directory.string(),
                     library_directory.string());

    const auto extension = base::lowercase_extension(source_file);
    validate_source_file(source_file, assets_directory, extension, "texture");

    const auto image = load_image(source_file, 4);
    if (image.width == 0 || image.height == 0 || image.pixels.empty()) {
        throw std::runtime_error("Texture source contains no image data");
    }

    vulcal::render::TextureData texture;
    texture.width_          = image.width;
    texture.height_         = image.height;
    texture.format_         = vulcal::render::TextureFormat::RGBA8;
    texture.pixels_         = image.pixels;

    const auto meta         = metadata_path(source_file);
    const auto has_metadata = std::filesystem::exists(meta);
    const auto metadata     = metadata_for_source(source_file, vulcal::asset::AssetType::Texture);
    const auto id           = metadata.id;

    std::filesystem::create_directories(library_directory);
    const auto runtime_file = vulcal::asset::AssetLibrary{ library_directory }.texture_path(id);
    vulcal::asset::TextureAsset::write(runtime_file, texture);
    if (!has_metadata) write_metadata(meta, metadata);

    base::log::info("TextureAssetImporter: {} {} as asset {} -> {} ({}x{})", has_metadata ? "reimported" : "imported", source_file.string(),
                    id.to_string(), runtime_file.string(), texture.width_, texture.height_);
    return id;
}

} // namespace apps::asset_import
