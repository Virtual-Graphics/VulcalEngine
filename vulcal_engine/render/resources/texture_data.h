#pragma once
#include <cstdint>
#include <vector>

namespace vulcal::render {

enum class TextureFormat : std::uint32_t {
    RGBA8 = 1,
};

struct TextureData {
    std::uint32_t width_  = 0;
    std::uint32_t height_ = 0;
    TextureFormat format_ = TextureFormat::RGBA8;
    std::vector<std::uint8_t> pixels_;
};

} // namespace vulcal::render
