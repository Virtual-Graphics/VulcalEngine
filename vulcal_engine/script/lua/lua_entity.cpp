#include "script/lua/lua_entity.h"

#include "scene/components.h"
#include "scene/scene.h"
#include "script/lua/lua_value.h"

#include <stdexcept>

namespace vulcal::script::lua {

BoundEntity::BoundEntity(scene::Entity entity)
        : entity_(entity) {}

std::string BoundEntity::name() const { return require_component<scene::TagComponent>(entity_, "tag").name_; }
std::string BoundEntity::id() const { return require_component<scene::IDComponent>(entity_, "id").id_.to_string(); }
bool BoundEntity::valid() const { return entity_.valid(); }

std::tuple<float, float, float> BoundEntity::get_translation() const {
    const auto& value = require_component<scene::TransformComponent>(entity_, "transform").translation_;
    return { value.x, value.y, value.z };
}

void BoundEntity::set_translation(float x, float y, float z) {
    require_component<scene::TransformComponent>(entity_, "transform").translation_ = { x, y, z };
}

std::tuple<float, float, float> BoundEntity::get_rotation() const {
    const auto& value = require_component<scene::TransformComponent>(entity_, "transform").rotation_;
    return { value.x, value.y, value.z };
}

void BoundEntity::set_rotation(float x, float y, float z) {
    require_component<scene::TransformComponent>(entity_, "transform").rotation_ = { x, y, z };
}

std::tuple<float, float, float> BoundEntity::get_scale() const {
    const auto& value = require_component<scene::TransformComponent>(entity_, "transform").scale_;
    return { value.x, value.y, value.z };
}

void BoundEntity::set_scale(float x, float y, float z) {
    require_component<scene::TransformComponent>(entity_, "transform").scale_ = { x, y, z };
}

void BoundEntity::destroy() {
    if (entity_.valid()) entity_.scene().destroy_entity(entity_);
}

std::optional<BoundSprite> BoundEntity::sprite() const {
    if (!entity_.valid() || !entity_.has_component<scene::SpriteRendererComponent>()) return std::nullopt;
    return BoundSprite{ entity_ };
}

std::optional<BoundCamera> BoundEntity::camera() const {
    if (!entity_.valid() || !entity_.has_component<scene::CameraComponent>()) return std::nullopt;
    return BoundCamera{ entity_ };
}

std::optional<BoundLight> BoundEntity::light() const {
    if (!entity_.valid() || !entity_.has_component<scene::LightComponent>()) return std::nullopt;
    return BoundLight{ entity_ };
}

std::optional<BoundMesh> BoundEntity::mesh() const {
    if (!entity_.valid() || !entity_.has_component<scene::MeshRendererComponent>()) return std::nullopt;
    return BoundMesh{ entity_ };
}

std::optional<BoundEnvironment> BoundEntity::environment() const {
    if (!entity_.valid() || !entity_.has_component<scene::EnvironmentComponent>()) return std::nullopt;
    return BoundEnvironment{ entity_ };
}

BoundSprite BoundEntity::add_sprite() { return add_sprite({}); }

BoundSprite BoundEntity::add_sprite(const std::string& texture_id) {
    const auto id            = texture_id.empty() ? base::UUID{} : parse_uuid(texture_id);
    auto& sprite             = entity_.add_component<scene::SpriteRendererComponent>();
    sprite.texture_asset_id_ = id;
    return BoundSprite{ entity_ };
}

BoundCamera BoundEntity::add_camera() {
    entity_.add_component<scene::CameraComponent>();
    return BoundCamera{ entity_ };
}

BoundLight BoundEntity::add_light() {
    entity_.add_component<scene::LightComponent>();
    return BoundLight{ entity_ };
}

BoundMesh BoundEntity::add_mesh(const std::string& mesh_id, const std::string& material_id) {
    const auto parsed_mesh     = parse_uuid(mesh_id);
    const auto parsed_material = parse_uuid(material_id);
    auto& mesh                 = entity_.add_component<scene::MeshRendererComponent>();
    mesh.mesh_asset_id_        = parsed_mesh;
    mesh.material_asset_id_    = parsed_material;
    return BoundMesh{ entity_ };
}

void BoundEntity::remove_sprite() {
    if (entity_.valid()) entity_.remove_component<scene::SpriteRendererComponent>();
}

void BoundEntity::remove_camera() {
    if (entity_.valid()) entity_.remove_component<scene::CameraComponent>();
}

void BoundEntity::remove_light() {
    if (entity_.valid()) entity_.remove_component<scene::LightComponent>();
}

void BoundEntity::remove_mesh() {
    if (entity_.valid()) entity_.remove_component<scene::MeshRendererComponent>();
}

} // namespace vulcal::script::lua
