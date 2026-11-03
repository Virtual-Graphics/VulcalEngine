#pragma once
#include "base/logger.h"
#include <cstdint>
#include <filesystem>
#include <stdexcept>
#include <stb_image.h>
#include <vector>

namespace vulcal::asset {

struct ImageData {
    std::uint32_t width = 0;
    std::uint32_t height = 0;
    std::uint32_t channels = 0;
    std::vector<std::uint8_t> pixels;
};

// Desired channels: 0 keeps the file's own channel count, otherwise forces
// e.g. 3 (RGB) or 4 (RGBA). Vertically flips the image to match GL's UV origin.
inline ImageData load_image(const std::filesystem::path& path, int desired_channels = 0) {
    stbi_set_flip_vertically_on_load(1);

    int width = 0;
    int height = 0;
    int channels = 0;
    stbi_uc* data = stbi_load(path.string().c_str(), &width, &height, &channels, desired_channels);
    if (!data) {
        const char* reason = stbi_failure_reason();
        base::log::error("failed to load image: {} ({})", path.string(), reason ? reason : "unknown");
        throw std::runtime_error("Image load failed: " + path.string());
    }

    if (desired_channels != 0) {
        channels = desired_channels;
    }

    const std::size_t size = static_cast<std::size_t>(width) * static_cast<std::size_t>(height)
                           * static_cast<std::size_t>(channels);

    ImageData image;
    image.width = static_cast<std::uint32_t>(width);
    image.height = static_cast<std::uint32_t>(height);
    image.channels = static_cast<std::uint32_t>(channels);
    image.pixels.assign(data, data + size);

    stbi_image_free(data);

    base::log::info("loaded image: {} ({}x{}, {} channels)", path.string(), width, height, channels);
    return image;
}

}
