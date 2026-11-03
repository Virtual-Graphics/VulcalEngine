#pragma once

#include "render/resources/mesh_data.h"

#include <filesystem>

namespace apps::asset_import {

class RuntimeMeshAssetWriter {
public:
    static void write(const std::filesystem::path& path, const vulcal::MeshData& mesh);
};

} // namespace apps::asset_import
