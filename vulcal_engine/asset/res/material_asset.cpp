#include "asset/res/material_asset.h"

#include "base/binary_io.h"
#include "base/logger.h"
#include "base/path_utils.h"
#include "resource/directory_provider.h"

#include <array>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <stdexcept>

namespace vulcal::asset {
namespace {

constexpr std::array<char, 8> magic{ 'V', 'U', 'L', 'M', 'A', 'T', 'C', '\0' };
constexpr std::uint32_t version   = 1;
constexpr std::uint64_t file_size = magic.size() + sizeof(std::uint32_t) + 7 * sizeof(float);

void validate(const render::Material& material) {
    for (const float value: { material.diffuse_.x, material.diffuse_.y, material.diffuse_.z, material.specular_.x, material.specular_.y,
                              material.specular_.z, material.shininess_ }) {
        if (!std::isfinite(value)) throw std::invalid_argument("Material values must be finite");
    }
    if (material.shininess_ < 0.0f) throw std::invalid_argument("Material shininess cannot be negative");
}

} // namespace

render::Material MaterialAsset::read(const std::filesystem::path& path) {
    base::log::debug("MaterialAsset: reading {}", path.string());
    const auto absolute = std::filesystem::absolute(path);
    const resource::DirectoryProvider resources(absolute.parent_path());
    return read(resources.read_binary(base::path_to_utf8(absolute.filename())));
}

render::Material MaterialAsset::read(std::span<const std::byte> data) {
    const std::string bytes(reinterpret_cast<const char*>(data.data()), data.size());
    std::istringstream input(bytes, std::ios::binary);
    if (!input) throw std::runtime_error("Could not read runtime material data");

    std::array<char, magic.size()> file_magic{};
    input.read(file_magic.data(), static_cast<std::streamsize>(file_magic.size()));
    if (!input || file_magic != magic) throw std::runtime_error("Invalid runtime material data");
    if (base::binary::read_u32_le(input) != version) throw std::runtime_error("Unsupported runtime material version");

    render::Material material;
    material.diffuse_   = { base::binary::read_float_le(input), base::binary::read_float_le(input), base::binary::read_float_le(input) };
    material.specular_  = { base::binary::read_float_le(input), base::binary::read_float_le(input), base::binary::read_float_le(input) };
    material.shininess_ = base::binary::read_float_le(input);
    if (input.peek() != std::char_traits<char>::eof() || data.size() != file_size)
        throw std::runtime_error("Invalid runtime material size");

    try {
        validate(material);
    } catch (const std::invalid_argument& error) {
        throw std::runtime_error("Invalid runtime material values: " + std::string(error.what()));
    }
    base::log::debug("MaterialAsset: read runtime data (version {}, {} bytes)", version, file_size);
    return material;
}

void MaterialAsset::write(const std::filesystem::path& path, const render::Material& material) {
    validate(material);
    base::log::debug("MaterialAsset: writing {} (version {})", path.string(), version);
    if (!path.parent_path().empty()) std::filesystem::create_directories(path.parent_path());

    std::ofstream output(path, std::ios::binary | std::ios::trunc);
    if (!output) throw std::runtime_error("Could not create runtime material file: " + path.string());
    output.write(magic.data(), static_cast<std::streamsize>(magic.size()));
    base::binary::write_u32_le(output, version);
    for (const float value: { material.diffuse_.x, material.diffuse_.y, material.diffuse_.z, material.specular_.x, material.specular_.y,
                              material.specular_.z, material.shininess_ }) {
        base::binary::write_float_le(output, value);
    }
    if (!output) throw std::runtime_error("Could not write runtime material file: " + path.string());
}

} // namespace vulcal::asset
