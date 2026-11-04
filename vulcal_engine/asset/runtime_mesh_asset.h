#pragma once

#include "render/resources/mesh_data.h"

#include <filesystem>

namespace vulcal::asset {

class RuntimeMeshAsset {
public:
    static MeshData read(const std::filesystem::path& path);
    static void write(const std::filesystem::path& path, const MeshData& mesh);
};

} // namespace vulcal::asset
