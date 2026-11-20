#include "asset/res/mesh_asset.h"

#include "base/binary_io.h"
#include "base/logger.h"
#include "base/path_utils.h"
#include "resource/directory_provider.h"

#include <array>
#include <cstdint>
#include <fstream>
#include <sstream>
#include <limits>
#include <stdexcept>
#include <string>

namespace vulcal::asset {
namespace {

constexpr std::array<char, 8> magic{ 'V', 'U', 'L', 'M', 'E', 'S', 'H', '\0' };
constexpr std::uint32_t version     = 1;
constexpr std::uint64_t header_size = 28;
constexpr std::uint64_t vertex_size = 24;
constexpr std::uint64_t index_size  = 4;

std::uint64_t expected_file_size(std::uint64_t vertex_count, std::uint64_t index_count) {

    constexpr auto max = std::numeric_limits<std::uint64_t>::max();
    if (vertex_count > (max - header_size) / vertex_size) {
        throw std::runtime_error("Runtime mesh is too large");
    }

    const auto vertices_size = vertex_count * vertex_size;
    if (index_count > (max - header_size - vertices_size) / index_size) {
        throw std::runtime_error("Runtime mesh is too large");
    }

    return header_size + vertices_size + index_count * index_size;
}

} // namespace

render::MeshData MeshAsset::read(const std::filesystem::path& path) {
    base::log::debug("MeshAsset: reading {}", path.string());
    const auto absolute = std::filesystem::absolute(path);
    const resource::DirectoryProvider resources(absolute.parent_path());
    return read(resources.read_binary(base::path_to_utf8(absolute.filename())));
}

render::MeshData MeshAsset::read(std::span<const std::byte> data) {
    const std::string bytes(reinterpret_cast<const char*>(data.data()), data.size());
    std::istringstream input(bytes, std::ios::binary);
    std::array<char, magic.size()> file_magic{};
    if (!input) {
        throw std::runtime_error("Could not read runtime mesh data");
    }

    input.read(file_magic.data(), static_cast<std::streamsize>(file_magic.size()));
    if (!input || file_magic != magic) {
        throw std::runtime_error("Invalid runtime mesh data");
    }

    if (base::binary::read_u32_le(input) != version) {
        throw std::runtime_error("Unsupported runtime mesh version");
    }

    const auto vertex_count  = base::binary::read_u64_le(input);
    const auto index_count   = base::binary::read_u64_le(input);
    const auto expected_size = expected_file_size(vertex_count, index_count);

    if (data.size() != expected_size || vertex_count > std::numeric_limits<std::size_t>::max() ||
        index_count > std::numeric_limits<std::size_t>::max()) {
        throw std::runtime_error("Invalid runtime mesh size");
    }

    render::MeshData mesh;
    mesh.vertices_.reserve(static_cast<std::size_t>(vertex_count));
    mesh.indices_.reserve(static_cast<std::size_t>(index_count));
    for (std::uint64_t i = 0; i < vertex_count; ++i) {
        mesh.vertices_.push_back({
            { base::binary::read_float_le(input), base::binary::read_float_le(input), base::binary::read_float_le(input) },
            { base::binary::read_float_le(input), base::binary::read_float_le(input), base::binary::read_float_le(input) },
        });
    }

    for (std::uint64_t i = 0; i < index_count; ++i) {
        const auto index = base::binary::read_u32_le(input);
        if (index >= vertex_count) {
            throw std::runtime_error("Runtime mesh index is outside the vertex array");
        }

        mesh.indices_.push_back(index);
    }
    base::log::debug("MeshAsset: read runtime data (version {}, {} bytes, {} vertices, {} indices)", version, expected_size,
                     mesh.vertices_.size(), mesh.indices_.size());
    return mesh;
}

void MeshAsset::write(const std::filesystem::path& path, const render::MeshData& mesh) {

    base::log::debug("MeshAsset: writing {} (version {}, {} vertices, {} indices)", path.string(), version, mesh.vertices_.size(),
                     mesh.indices_.size());

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

} // namespace vulcal::asset
