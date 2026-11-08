#pragma once
#include <cstdint>

namespace vulcal::render {

// 0 is invalid handle
struct MeshHandle {
    std::uint64_t value_ = 0;
    bool operator==(const MeshHandle& other) const { return value_ == other.value_; }
};

struct MaterialHandle {
    std::uint64_t value_ = 0;
    bool operator==(const MaterialHandle& other) const { return value_ == other.value_; }
};

struct TextureHandle {
    std::uint64_t value_ = 0;
    bool operator==(const TextureHandle& other) const { return value_ == other.value_; }
};

}
