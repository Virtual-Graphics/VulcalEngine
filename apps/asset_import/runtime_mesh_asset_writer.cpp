#include "asset_import/runtime_mesh_asset_writer.h"

#include "base/binary_io.h"
#include "base/logger.h"

#include <array>
#include <cstdint>
#include <fstream>
#include <stdexcept>

namespace apps::asset_import {
namespace {

constexpr std::array<char, 8> magic{ 'V', 'U', 'L', 'M', 'E', 'S', 'H', '\0' };
constexpr std::uint32_t version = 1;

} // namespace

void RuntimeMeshAssetWriter::write(const std::filesystem::path& path, const vulcal::MeshData& mesh) {

    base::log::debug("RuntimeMeshAssetWriter: writing {} (version {}, {} vertices, {} indices)", path.string(), version,
                     mesh.vertices_.size(), mesh.indices_.size());

    // 预处理 begin
    for (const auto index: mesh.indices_) {
        if (index >= mesh.vertices_.size()) throw std::invalid_argument("Mesh index is outside the vertex array");
    }

    if (!path.parent_path().empty()) {
        std::filesystem::create_directories(path.parent_path());
    }
    // 预处理 end

    std::ofstream output(path, std::ios::binary | std::ios::trunc);
    if (!output) {
        throw std::runtime_error("Could not create runtime mesh file: " + path.string());
    }

    output.write(magic.data(), static_cast<std::streamsize>(magic.size()));
    base::binary::write_u32_le(output, version);
    base::binary::write_u64_le(output, mesh.vertices_.size());
    base::binary::write_u64_le(output, mesh.indices_.size());
    for (const auto& vertex: mesh.vertices_) {
        base::binary::write_float_le(output, vertex.position.x);
        base::binary::write_float_le(output, vertex.position.y);
        base::binary::write_float_le(output, vertex.position.z);
        base::binary::write_float_le(output, vertex.normal.x);
        base::binary::write_float_le(output, vertex.normal.y);
        base::binary::write_float_le(output, vertex.normal.z);
    }
    for (const auto index: mesh.indices_) {
        base::binary::write_u32_le(output, index);
    }

    if (!output) {
        throw std::runtime_error("Could not write runtime mesh file: " + path.string());
    }
}

} // namespace apps::asset_import
