#include "asset_import/meta_data.h"

#include "base/logger.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <stdexcept>
#include <string>
#include <yaml-cpp/yaml.h>

namespace apps::asset_import {
namespace {

const char* type_name(vulcal::asset::AssetType type) {
    switch (type) {
        case vulcal::asset::AssetType::Mesh:
            return "mesh";
        case vulcal::asset::AssetType::Material:
            return "material";
    }
    throw std::invalid_argument("Unsupported asset type");
}

vulcal::asset::AssetType parse_type(const std::string& type, const std::filesystem::path& path) {
    if (type == "mesh") return vulcal::asset::AssetType::Mesh;
    if (type == "material") return vulcal::asset::AssetType::Material;
    throw std::runtime_error("Unsupported asset type in metadata: " + path.string());
}

} // namespace

std::filesystem::path metadata_path(const std::filesystem::path& source_file) {
    auto path = source_file;
    path += ".meta";
    return path;
}

Metadata read_metadata(const std::filesystem::path& metadata_file_path) {
    base::log::debug("Metadata: reading {}", metadata_file_path.string());
    try {
        const auto metadata = YAML::LoadFile(metadata_file_path.string());
        if (!metadata.IsMap()) {
            throw std::runtime_error("Unsupported asset metadata: " + metadata_file_path.string());
        }

        const auto version = metadata["format_version"].as<std::uint32_t>();
        if (version != 1 && version != metadata_format_version) {
            throw std::runtime_error("Unsupported asset metadata: " + metadata_file_path.string());
        }

        const vulcal::asset::AssetID id{ metadata["id"].as<std::uint64_t>() };
        if (id.is_nil()) {
            throw std::runtime_error("Asset metadata has an empty ID: " + metadata_file_path.string());
        }

        const auto type = version == 1 ? vulcal::asset::AssetType::Mesh : parse_type(metadata["type"].as<std::string>(), metadata_file_path);
        base::log::debug("Metadata: read {} asset {} from {} (version {})", type_name(type), id.to_string(), metadata_file_path.string(), version);
        return Metadata{ version, id, type };
    } catch (const YAML::Exception& error) {
        throw std::runtime_error("Invalid asset metadata " + metadata_file_path.string() + ": " + error.what());
    }
}

void write_metadata(const std::filesystem::path& metadata_file_path, const Metadata& metadata) {
    if (metadata.format_version != metadata_format_version) {
        throw std::invalid_argument("Cannot write unsupported asset metadata version");
    }
    const auto type = type_name(metadata.type);
    base::log::debug("Metadata: writing {} asset {} to {} (version {})", type, metadata.id.to_string(), metadata_file_path.string(),
                     metadata.format_version);
    YAML::Emitter output;
    output << YAML::BeginMap;
    output << YAML::Key << "format_version" << YAML::Value << metadata.format_version;
    output << YAML::Key << "id" << YAML::Value << metadata.id.value();
    output << YAML::Key << "type" << YAML::Value << type;
    output << YAML::EndMap;

    std::ofstream file(metadata_file_path, std::ios::binary | std::ios::trunc);
    if (!file) {
        throw std::runtime_error("Could not create asset metadata: " + metadata_file_path.string());
    }

    file << output.c_str();
    if (!file) {
        throw std::runtime_error("Could not write asset metadata: " + metadata_file_path.string());
    }
}

Metadata metadata_for_source(const std::filesystem::path& source_file, vulcal::asset::AssetType expected_type) {
    const auto meta     = metadata_path(source_file);
    const auto metadata = std::filesystem::exists(meta) ? read_metadata(meta)
                                                        : Metadata{ .id = vulcal::asset::AssetID::generate(), .type = expected_type };
    if (metadata.type != expected_type) {
        throw std::invalid_argument("Asset metadata type does not match source file: " + meta.string());
    }
    return metadata;
}

void validate_source_file(const std::filesystem::path& source_file, const std::filesystem::path& assets_directory,
                          const std::string& extension, const std::string& asset_name) {
    std::error_code error;
    if (!std::filesystem::is_regular_file(source_file, error) || error) {
        throw std::invalid_argument(asset_name + " source file does not exist: " + source_file.string());
    }

    auto actual_extension = source_file.extension().string();
    std::transform(actual_extension.begin(), actual_extension.end(), actual_extension.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    if (actual_extension != extension) {
        throw std::invalid_argument("Only " + extension + " files can be imported as " + asset_name + " assets");
    }

    const auto source   = std::filesystem::weakly_canonical(source_file);
    const auto assets   = std::filesystem::weakly_canonical(assets_directory);
    const auto relative = source.lexically_relative(assets);
    if (relative.empty() || relative.is_absolute() || *relative.begin() == "..") {
        throw std::invalid_argument("Import source must be inside the project's Assets directory");
    }
}

} // namespace apps::asset_import
