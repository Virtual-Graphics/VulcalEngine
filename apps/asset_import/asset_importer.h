#pragma once

#include "asset/asset_id.h"

#include <filesystem>
#include <variant>

namespace apps::asset_import {

using ImportedAssetID = std::variant<vulcal::asset::MeshAssetID, vulcal::asset::MaterialAssetID, vulcal::asset::TextureAssetID>;

class AssetImporter {
public:
    static ImportedAssetID import(const std::filesystem::path& source_file, const std::filesystem::path& project_directory,
                                  const std::filesystem::path& library_directory);
};

} // namespace apps::asset_import
