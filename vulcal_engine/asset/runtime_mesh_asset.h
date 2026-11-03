#pragma once

#include "render/resources/mesh_data.h"

#include <filesystem>

namespace vulcal::asset {

class RuntimeMeshAsset {
public:
    static MeshData read(const std::filesystem::path& path);
};

} // namespace vulcal::asset
