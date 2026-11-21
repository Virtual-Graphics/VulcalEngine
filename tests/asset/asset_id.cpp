#include "asset/asset_id.h"
#include "asset/asset_library.h"
#include "asset/asset_manager.h"
#include "asset/gpu_resource_registry.h"
#include "scene/components.h"

#include <concepts>
#include <gtest/gtest.h>
#include <limits>
#include <type_traits>
#include <unordered_map>

namespace {

using vulcal::asset::AssetLibrary;
using vulcal::asset::AssetManager;
using vulcal::asset::MaterialAssetID;
using vulcal::asset::MeshAssetID;
using vulcal::asset::GPUResourceRegistry;
using vulcal::asset::TextureAssetID;

template<typename ID, typename Other>
constexpr bool rejects_other_id =
        !std::is_constructible_v<ID, Other> && !std::is_assignable_v<ID&, Other> && !requires(ID id, Other other) { id == other; };

static_assert(rejects_other_id<MeshAssetID, MaterialAssetID> && rejects_other_id<MaterialAssetID, MeshAssetID>);
static_assert(rejects_other_id<MeshAssetID, TextureAssetID> && rejects_other_id<TextureAssetID, MeshAssetID>);
static_assert(rejects_other_id<MaterialAssetID, TextureAssetID> && rejects_other_id<TextureAssetID, MaterialAssetID>);

template<typename ID>
constexpr bool explicit_storage_conversions =
        std::is_constructible_v<ID, base::UUID> && std::is_constructible_v<ID, std::uint64_t> && !std::is_convertible_v<base::UUID, ID> &&
        !std::is_convertible_v<std::uint64_t, ID> && !std::is_convertible_v<ID, base::UUID> && !std::is_convertible_v<ID, std::uint64_t> &&
        !std::is_assignable_v<ID&, base::UUID> && !std::is_assignable_v<ID&, std::uint64_t>;

static_assert(explicit_storage_conversions<MeshAssetID>);
static_assert(explicit_storage_conversions<MaterialAssetID>);
static_assert(explicit_storage_conversions<TextureAssetID>);

template<typename ID>
concept MeshInput = requires(AssetManager& assets, const AssetLibrary& library, GPUResourceRegistry& registry, ID id) {
    assets.load_mesh(id);
    library.load_mesh(id);
    library.mesh_path(id);
    registry.resolve(id);
};

template<typename ID>
concept MaterialInput = requires(AssetManager& assets, const AssetLibrary& library, GPUResourceRegistry& registry, ID id) {
    assets.load_material(id);
    library.load_material(id);
    library.material_path(id);
    registry.resolve(id);
};

template<typename ID>
concept TextureInput = requires(AssetManager& assets, const AssetLibrary& library, GPUResourceRegistry& registry, ID id) {
    assets.load_texture(id);
    library.load_texture(id);
    library.texture_path(id);
    registry.resolve(id);
};

static_assert(MeshInput<MeshAssetID> && !MeshInput<MaterialAssetID> && !MeshInput<TextureAssetID>);
static_assert(MaterialInput<MaterialAssetID> && !MaterialInput<MeshAssetID> && !MaterialInput<TextureAssetID>);
static_assert(TextureInput<TextureAssetID> && !TextureInput<MeshAssetID> && !TextureInput<MaterialAssetID>);
static_assert(!MeshInput<base::UUID> && !MaterialInput<base::UUID> && !TextureInput<base::UUID>);
static_assert(!MeshInput<std::uint64_t> && !MaterialInput<std::uint64_t> && !TextureInput<std::uint64_t>);

static_assert(std::same_as<decltype(std::declval<AssetManager&>().add_mesh({})), MeshAssetID>);
static_assert(std::same_as<decltype(std::declval<AssetManager&>().add_material({})), MaterialAssetID>);
static_assert(std::same_as<decltype(std::declval<AssetManager&>().add_texture({})), TextureAssetID>);
static_assert(std::same_as<decltype(vulcal::scene::MeshRendererComponent::mesh_asset_id_), MeshAssetID>);
static_assert(std::same_as<decltype(vulcal::scene::MeshRendererComponent::material_asset_id_), MaterialAssetID>);
static_assert(std::same_as<decltype(vulcal::scene::SpriteRendererComponent::texture_asset_id_), TextureAssetID>);

using AssetIDTypes = testing::Types<MeshAssetID, MaterialAssetID, TextureAssetID>;
template<typename ID>
class AssetIDTest : public testing::Test {};
TYPED_TEST_SUITE(AssetIDTest, AssetIDTypes);

TYPED_TEST(AssetIDTest, DefaultsToNilAndGeneratesNonNilIdentity) {
    const TypeParam nil;
    EXPECT_TRUE(nil.is_nil());
    EXPECT_EQ(nil.value(), 0u);
    EXPECT_EQ(nil.to_string(), "0");
    EXPECT_EQ(nil, TypeParam{});

    const auto first  = TypeParam::generate();
    const auto second = TypeParam::generate();
    EXPECT_FALSE(first.is_nil());
    EXPECT_FALSE(second.is_nil());
    EXPECT_NE(first, second);
}

TYPED_TEST(AssetIDTest, PreservesStoredUuidAndDecimalRepresentation) {
    const auto value = std::numeric_limits<std::uint64_t>::max();
    const TypeParam from_integer{ value };
    const TypeParam from_uuid{ base::UUID{ value } };
    EXPECT_EQ(from_integer, from_uuid);
    EXPECT_EQ(from_uuid.value(), value);
    EXPECT_EQ(from_uuid.to_string(), "18446744073709551615");
}

TYPED_TEST(AssetIDTest, HashesByIdentity) {
    std::unordered_map<TypeParam, int> values;
    values.emplace(TypeParam{ 42 }, 1);
    values.insert_or_assign(TypeParam{ base::UUID{ 42 } }, 2);
    values.emplace(TypeParam{ 43 }, 3);
    EXPECT_EQ(values.size(), 2u);
    EXPECT_EQ(values.at(TypeParam{ 42 }), 2);
    EXPECT_EQ(values.at(TypeParam{ 43 }), 3);
}

} // namespace
