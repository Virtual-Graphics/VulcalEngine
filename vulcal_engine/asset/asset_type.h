#pragma once

#include <cstdint>

namespace vulcal::asset {

enum class AssetType : std::uint8_t {
    Mesh     = 1,
    Material = 2,
    Texture  = 3,
};

} // namespace vulcal::asset
