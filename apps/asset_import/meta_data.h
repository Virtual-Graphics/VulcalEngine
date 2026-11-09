#pragma once

#include "asset/asset_id.h"
#include "asset/asset_type.h"

#include <cstdint>
#include <filesystem>
#include <string>

namespace apps::asset_import {
constexpr std::uint32_t metadata_format_version = 1;

struct Metadata {
    std::uint32_t format_version = metadata_format_version;
    vulcal::asset::AssetID id;
    vulcal::asset::AssetType type = vulcal::asset::AssetType::Mesh;
};

std::filesystem::path metadata_path(const std::filesystem::path& source_file);

Metadata read_metadata(const std::filesystem::path& metadata_file_path);
void write_metadata(const std::filesystem::path& metadata_file_path, const Metadata& metadata);

Metadata metadata_for_source(const std::filesystem::path& source_file, vulcal::asset::AssetType expected_type);

void validate_source_file(const std::filesystem::path& source_file, const std::filesystem::path& project_directory,
                          const std::string& extension, const std::string& asset_name);

} // namespace apps::asset_import
