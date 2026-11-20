#pragma once

#include "render/resources/texture_data.h"

#include <filesystem>
#include <span>

namespace vulcal::asset {

class TextureAsset {
public:
    static render::TextureData read(const std::filesystem::path& path);
    static render::TextureData read(std::span<const std::byte> data);
    static void write(const std::filesystem::path& path, const render::TextureData& texture);
};

} // namespace vulcal::asset
