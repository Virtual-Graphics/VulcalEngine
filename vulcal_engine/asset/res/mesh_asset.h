#pragma once

#include "render/resources/mesh_data.h"

#include <filesystem>
#include <span>

namespace vulcal::asset {

class MeshAsset {
public:
    static render::MeshData read(const std::filesystem::path& path);
    static render::MeshData read(std::span<const std::byte> data);
    static void write(const std::filesystem::path& path, const render::MeshData& mesh);
};

} // namespace vulcal::asset
