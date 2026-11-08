#pragma once
#include <cstdint>

namespace vulcal::render {

enum class RenderItemKind : std::uint8_t {
    Mesh,
    Sprite,
};

} // namespace vulcal::render
