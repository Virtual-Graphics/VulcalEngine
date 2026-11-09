#pragma once

#include "asset/asset_id.h"

#include <filesystem>

namespace apps::asset_import {

class MaterialAssetImporter {
public:
    static vulcal::asset::AssetID import(const std::filesystem::path& source_file, const std::filesystem::path& project_directory,
                                         const std::filesystem::path& library_directory);
};

} // namespace apps::asset_import
