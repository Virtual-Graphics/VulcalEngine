#include "asset_import/meta_data.h"

#include "base/logger.h"

#include <fstream>
#include <stdexcept>
#include <yaml-cpp/yaml.h>

namespace apps::asset_import {

std::filesystem::path metadata_path(const std::filesystem::path& source_file) {
    auto path = source_file;
    path += ".meta";
    return path;
}

Metadata read_metadata(const std::filesystem::path& path) {
    base::log::debug("Metadata: reading {}", path.string());
    try {
        const auto metadata = YAML::LoadFile(path.string());
        if (!metadata.IsMap()) {
            throw std::runtime_error("Unsupported asset metadata: " + path.string());
        }

        const auto version = metadata["format_version"].as<std::uint32_t>();
        if (version != metadata_format_version) {
            throw std::runtime_error("Unsupported asset metadata: " + path.string());
        }

        const vulcal::asset::AssetID id{ metadata["id"].as<std::uint64_t>() };
        if (id.is_nil()) {
            throw std::runtime_error("Asset metadata has an empty ID: " + path.string());
        }

        base::log::debug("Metadata: read asset {} from {} (version {})", id.to_string(), path.string(), version);
        return Metadata{ version, id };
    } catch (const YAML::Exception& error) {
        throw std::runtime_error("Invalid asset metadata " + path.string() + ": " + error.what());
    }
}

void write_metadata(const std::filesystem::path& path, const Metadata& metadata) {
    base::log::debug("Metadata: writing asset {} to {} (version {})", metadata.id.to_string(), path.string(), metadata.format_version);
    YAML::Emitter output;
    output << YAML::BeginMap;
    output << YAML::Key << "format_version" << YAML::Value << metadata.format_version;
    output << YAML::Key << "id" << YAML::Value << metadata.id.value();
    output << YAML::EndMap;

    std::ofstream file(path, std::ios::binary | std::ios::trunc);
    if (!file) {
        throw std::runtime_error("Could not create asset metadata: " + path.string());
    }

    file << output.c_str();
    if (!file) {
        throw std::runtime_error("Could not write asset metadata: " + path.string());
    }
}

} // namespace apps::asset_import
