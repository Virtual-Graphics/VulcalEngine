#pragma once

#include "asset/asset_id.h"

#include <cstdint>
#include <filesystem>

namespace apps::asset_import {
constexpr std::uint32_t metadata_format_version = 1;
struct Metadata {
    std::uint32_t format_version = metadata_format_version;
    vulcal::asset::AssetID id;
};

std::filesystem::path metadata_path(const std::filesystem::path& source_file);
Metadata read_metadata(const std::filesystem::path& path);
void write_metadata(const std::filesystem::path& path, const Metadata& metadata);

} // namespace apps::asset_import
