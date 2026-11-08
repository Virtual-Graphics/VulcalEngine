#include "asset_import/asset_importer.h"

#include "asset_import/importers/material_asset_importer.h"
#include "asset_import/importers/obj_asset_importer.h"
#include "asset_import/importers/texture_asset_importer.h"

#include <algorithm>
#include <cctype>
#include <stdexcept>
#include <string>

namespace apps::asset_import {

vulcal::asset::AssetID AssetImporter::import(const std::filesystem::path& source_file,
                                             const std::filesystem::path& assets_directory,
                                             const std::filesystem::path& library_directory) {
    auto extension = source_file.extension().string();
    std::transform(extension.begin(), extension.end(), extension.begin(), [](unsigned char character) {
        return static_cast<char>(std::tolower(character));
    });

    if (extension == ".obj") {
        return ObjAssetImporter::import(source_file, assets_directory, library_directory);
    }
    if (extension == ".vmat") {
        return MaterialAssetImporter::import(source_file, assets_directory, library_directory);
    }
    if (extension == ".png" || extension == ".jpg" || extension == ".jpeg" || extension == ".bmp" || extension == ".tga") {
        return TextureAssetImporter::import(source_file, assets_directory, library_directory);
    }
    throw std::invalid_argument("Unsupported asset source extension: " + extension);
}

} // namespace apps::asset_import
