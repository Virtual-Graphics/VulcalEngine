#pragma once

#include "render/resources/mesh_data.h"

#include <filesystem>

namespace vulcal::asset {

class MeshAsset {
public:
    static render::MeshData read(const std::filesystem::path& path);
    static void write(const std::filesystem::path& path, const render::MeshData& mesh);
};

} // namespace vulcal::asset
