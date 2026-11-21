#include "asset/res/texture_asset.h"

#include "base/binary_io.h"
#include "base/logger.h"
#include "base/path_utils.h"
#include "resource/directory_provider.h"

#include <array>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <limits>
#include <sstream>
#include <stdexcept>

namespace vulcal::asset {
namespace {

constexpr std::array<char, 8> magic{ 'V', 'U', 'L', 'T', 'E', 'X', '\0', '\0' };
constexpr std::uint32_t version     = 1;
constexpr std::uint64_t header_size = magic.size() + sizeof(std::uint32_t) + 3 * sizeof(std::uint32_t) + sizeof(std::uint64_t);

std::uint64_t expected_data_size(std::uint32_t width, std::uint32_t height) {
    if (width == 0 || height == 0) {
        throw std::invalid_argument("Texture dimensions must be non-zero");
    }

    constexpr auto max = std::numeric_limits<std::uint64_t>::max();
    const auto pixels  = static_cast<std::uint64_t>(width) * static_cast<std::uint64_t>(height);
    if (pixels > max / 4) {
        throw std::runtime_error("Texture is too large");
    }
    return pixels * 4;
}

void validate(const render::TextureData& texture) {
    if (texture.format_ != render::TextureFormat::RGBA8) {
        throw std::invalid_argument("Unsupported texture format");
    }
    const auto expected = expected_data_size(texture.width_, texture.height_);
    if (texture.pixels_.size() != expected) {
        throw std::invalid_argument("Texture pixel data has an invalid size");
    }
}

} // namespace

render::TextureData TextureAsset::read(const std::filesystem::path& path) {
    base::log::debug("TextureAsset: reading {}", path.string());
    const auto absolute = std::filesystem::absolute(path);
    const resource::DirectoryProvider resources(absolute.parent_path());
    return read(resources.read_binary(base::path_to_utf8(absolute.filename())));
}

render::TextureData TextureAsset::read(std::span<const std::byte> data) {
    const std::string bytes(reinterpret_cast<const char*>(data.data()), data.size());
    std::istringstream input(bytes, std::ios::binary);
    if (!input) {
        throw std::runtime_error("Could not read runtime texture data");
    }

    std::array<char, magic.size()> file_magic{};
    input.read(file_magic.data(), static_cast<std::streamsize>(file_magic.size()));
    if (!input || file_magic != magic) {
        throw std::runtime_error("Invalid runtime texture data");
    }
    if (base::binary::read_u32_le(input) != version) {
        throw std::runtime_error("Unsupported runtime texture version");
    }

    const auto width     = base::binary::read_u32_le(input);
    const auto height    = base::binary::read_u32_le(input);
    const auto format    = base::binary::read_u32_le(input);
    const auto data_size = base::binary::read_u64_le(input);

    if (format != static_cast<std::uint32_t>(render::TextureFormat::RGBA8)) {
        throw std::runtime_error("Unsupported runtime texture format");
    }

    std::uint64_t expected_size = 0;
    try {
        expected_size = expected_data_size(width, height);
    } catch (const std::exception& error) {
        throw std::runtime_error("Invalid runtime texture dimensions: " + std::string(error.what()));
    }
    if (data_size != expected_size) {
        throw std::runtime_error("Invalid runtime texture data size");
    }

    if (data.size() < header_size || data.size() - header_size != data_size || data_size > std::numeric_limits<std::size_t>::max()) {
        throw std::runtime_error("Invalid runtime texture file size");
    }

    render::TextureData texture;
    texture.width_  = width;
    texture.height_ = height;
    texture.format_ = render::TextureFormat::RGBA8;
    texture.pixels_.resize(static_cast<std::size_t>(data_size));
    input.read(reinterpret_cast<char*>(texture.pixels_.data()), static_cast<std::streamsize>(data_size));
    if (!input) {
        throw std::runtime_error("Could not read runtime texture pixels");
    }

    base::log::debug("TextureAsset: read runtime data (version {}, {}x{}, {} bytes)", version, texture.width_, texture.height_,
                     data_size);
    return texture;
}

void TextureAsset::write(const std::filesystem::path& path, const render::TextureData& texture) {
    validate(texture);
    base::log::debug("TextureAsset: writing {} (version {}, {}x{})", path.string(), version, texture.width_, texture.height_);

    if (!path.parent_path().empty()) {
        std::filesystem::create_directories(path.parent_path());
    }

    std::ofstream output(path, std::ios::binary | std::ios::trunc);
    if (!output) {
        throw std::runtime_error("Could not create runtime texture file: " + path.string());
    }

    output.write(magic.data(), static_cast<std::streamsize>(magic.size()));
    base::binary::write_u32_le(output, version);
    base::binary::write_u32_le(output, texture.width_);
    base::binary::write_u32_le(output, texture.height_);
    base::binary::write_u32_le(output, static_cast<std::uint32_t>(texture.format_));
    base::binary::write_u64_le(output, texture.pixels_.size());
    output.write(reinterpret_cast<const char*>(texture.pixels_.data()), static_cast<std::streamsize>(texture.pixels_.size()));
    if (!output) {
        throw std::runtime_error("Could not write runtime texture file: " + path.string());
    }
}

} // namespace vulcal::asset
