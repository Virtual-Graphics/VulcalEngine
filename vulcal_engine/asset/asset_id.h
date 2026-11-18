#pragma once

#include "base/uuid.h"

namespace vulcal::asset {

// Persistent asset identity. Different asset categories cannot be mixed.
template<typename Tag>
class AssetID {
public:
    AssetID() = default;
    explicit AssetID(base::UUID value)
            : value_(value) {}
    explicit AssetID(std::uint64_t value)
            : value_(value) {}

    static AssetID generate() { return AssetID{ base::UUID::generate() }; }

    std::uint64_t value() const { return value_.value(); }
    bool is_nil() const { return value_.is_nil(); }
    std::string to_string() const { return value_.to_string(); }

    bool operator==(const AssetID&) const = default;

private:
    base::UUID value_;
};

struct MeshAssetTag;
struct MaterialAssetTag;
struct TextureAssetTag;

using MeshAssetID     = AssetID<MeshAssetTag>;
using MaterialAssetID = AssetID<MaterialAssetTag>;
using TextureAssetID  = AssetID<TextureAssetTag>;

} // namespace vulcal::asset

namespace std {
template<typename Tag>
struct hash<vulcal::asset::AssetID<Tag>> {
    std::size_t operator()(const vulcal::asset::AssetID<Tag>& id) const noexcept { return std::hash<std::uint64_t>{}(id.value()); }
};
} // namespace std
