#include "script/lua/lua_components.h"

#include "scene/components.h"
#include "scene/scene.h"
#include "script/lua/lua_value.h"

#include <glm/glm.hpp>
#include <stdexcept>

namespace vulcal::script::lua {
namespace {
// clang-format off
scene::SpriteRendererComponent& sprite_component(scene::Entity entity) {
    return require_component<scene::SpriteRendererComponent>(entity, "sprite");
}

scene::CameraComponent& camera_component(scene::Entity entity) { 
    return require_component<scene::CameraComponent>(entity, "camera"); 
}

scene::LightComponent& light_component(scene::Entity entity) { 
    return require_component<scene::LightComponent>(entity, "light"); 
}

scene::MeshRendererComponent& mesh_component(scene::Entity entity) {
    return require_component<scene::MeshRendererComponent>(entity, "mesh");
}

scene::EnvironmentComponent& environment_component(scene::Entity entity) {
    return require_component<scene::EnvironmentComponent>(entity, "environment");
}

} // namespace

#pragma region BoundSprite

BoundSprite::BoundSprite(scene::Entity entity): entity_(entity) {}

bool BoundSprite::visible() const { 
    return sprite_component(entity_).visible_; 
}
void BoundSprite::set_visible(bool value) { 
    sprite_component(entity_).visible_ = value; 
}

std::vector<float> BoundSprite::color() const {
    const auto& value = sprite_component(entity_).color_;
    return { value.x, value.y, value.z, value.w };
}

void BoundSprite::set_color(const std::vector<float>& value) {
    require_size_3_or_4(value, "color");
    sprite_component(entity_).color_ = { value[0], value[1], value[2], value.size() == 4 ? value[3] : 1.0f };
}

std::vector<float> BoundSprite::uv_rect() const {
    const auto& value = sprite_component(entity_).uv_rect_;
    return { value.x, value.y, value.z, value.w };
}

void BoundSprite::set_uv_rect(const std::vector<float>& value) {
    require_size(value, 4, "uv_rect");
    sprite_component(entity_).uv_rect_ = { value[0], value[1], value[2], value[3] };
}

std::vector<float> BoundSprite::pivot() const {
    const auto& value = sprite_component(entity_).pivot_;
    return { value.x, value.y };
}

void BoundSprite::set_pivot(const std::vector<float>& value) {
    require_size(value, 2, "pivot");
    sprite_component(entity_).pivot_ = { value[0], value[1] };
}

bool BoundSprite::flip_x() const { return sprite_component(entity_).flip_x_; }
void BoundSprite::set_flip_x(bool value) { sprite_component(entity_).flip_x_ = value; }

bool BoundSprite::flip_y() const { return sprite_component(entity_).flip_y_; }
void BoundSprite::set_flip_y(bool value) { sprite_component(entity_).flip_y_ = value; }

int BoundSprite::sorting_order() const { return sprite_component(entity_).sorting_order_; }
void BoundSprite::set_sorting_order(int value) { sprite_component(entity_).sorting_order_ = value; }

std::string BoundSprite::texture_id() const { return sprite_component(entity_).texture_asset_id_.to_string(); }

#pragma endregion

#pragma region BoundCamera
BoundCamera::BoundCamera(scene::Entity entity): entity_(entity) {}

std::string BoundCamera::projection() const {
    return camera_component(entity_).projection_type_ == scene::CameraComponent::ProjectionType::Perspective ? "perspective"
                                                                                                             : "orthographic";
}

void BoundCamera::set_projection(const std::string& value) {
    if (value == "perspective") {
        camera_component(entity_).projection_type_ = scene::CameraComponent::ProjectionType::Perspective;
    } else if (value == "orthographic") {
        camera_component(entity_).projection_type_ = scene::CameraComponent::ProjectionType::Orthographic;
    } else {
        throw std::invalid_argument("projection must be 'perspective' or 'orthographic'");
    }
}

bool BoundCamera::primary() const { return camera_component(entity_).primary_; }
void BoundCamera::set_primary(bool value) { camera_component(entity_).primary_ = value; }

float BoundCamera::fov() const { return glm::degrees(camera_component(entity_).perspective_fov_y_); }
void BoundCamera::set_fov(float degrees) { camera_component(entity_).perspective_fov_y_ = glm::radians(degrees); }

float BoundCamera::near_plane() const {
    const auto& camera = camera_component(entity_);
    return camera.projection_type_ == scene::CameraComponent::ProjectionType::Perspective ? camera.perspective_near_
                                                                                          : camera.orthographic_near_;
}

void BoundCamera::set_near_plane(float value) {
    auto& camera = camera_component(entity_);
    if (camera.projection_type_ == scene::CameraComponent::ProjectionType::Perspective) {
        camera.perspective_near_ = value;
    } else {
        camera.orthographic_near_ = value;
    }
}

float BoundCamera::far_plane() const {
    const auto& camera = camera_component(entity_);
    return camera.projection_type_ == scene::CameraComponent::ProjectionType::Perspective ? camera.perspective_far_
                                                                                          : camera.orthographic_far_;
}

void BoundCamera::set_far_plane(float value) {
    auto& camera = camera_component(entity_);
    if (camera.projection_type_ == scene::CameraComponent::ProjectionType::Perspective) {
        camera.perspective_far_ = value;
    } else {
        camera.orthographic_far_ = value;
    }
}

float BoundCamera::ortho_left() const { return camera_component(entity_).orthographic_left_; }
void BoundCamera::set_ortho_left(float value) { camera_component(entity_).orthographic_left_ = value; }

float BoundCamera::ortho_right() const { return camera_component(entity_).orthographic_right_; }
void BoundCamera::set_ortho_right(float value) { camera_component(entity_).orthographic_right_ = value; }

float BoundCamera::ortho_bottom() const { return camera_component(entity_).orthographic_bottom_; }
void BoundCamera::set_ortho_bottom(float value) { camera_component(entity_).orthographic_bottom_ = value; }

float BoundCamera::ortho_top() const { return camera_component(entity_).orthographic_top_; }
void BoundCamera::set_ortho_top(float value) { camera_component(entity_).orthographic_top_ = value; }
#pragma endregion

#pragma region BoundLight
BoundLight::BoundLight(scene::Entity entity)
        : entity_(entity) {}

std::vector<float> BoundLight::color() const {
    const auto& value = light_component(entity_).color_;
    return { value.x, value.y, value.z };
}

void BoundLight::set_color(const std::vector<float>& value) {
    require_size(value, 3, "color");
    light_component(entity_).color_ = { value[0], value[1], value[2] };
}

float BoundLight::intensity() const { return light_component(entity_).intensity_; }
void BoundLight::set_intensity(float value) { light_component(entity_).intensity_ = value; }

bool BoundLight::shadow_enabled() const { return light_component(entity_).shadow_enabled_; }
void BoundLight::set_shadow_enabled(bool value) { light_component(entity_).shadow_enabled_ = value; }
#pragma endregion

#pragma region BoundMesh
BoundMesh::BoundMesh(scene::Entity entity)
        : entity_(entity) {}

bool BoundMesh::visible() const { return mesh_component(entity_).visible_; }
void BoundMesh::set_visible(bool value) { mesh_component(entity_).visible_ = value; }

std::string BoundMesh::mesh_id() const { return mesh_component(entity_).mesh_asset_id_.to_string(); }
std::string BoundMesh::material_id() const { return mesh_component(entity_).material_asset_id_.to_string(); }
#pragma endregion


#pragma region BoundEnvironment
BoundEnvironment::BoundEnvironment(scene::Entity entity)
        : entity_(entity) {}

std::vector<float> BoundEnvironment::ambient_color() const {
    const auto& value = environment_component(entity_).ambient_color_;
    return { value.x, value.y, value.z };
}

void BoundEnvironment::set_ambient_color(const std::vector<float>& value) {
    require_size(value, 3, "ambient_color");
    environment_component(entity_).ambient_color_ = { value[0], value[1], value[2] };
}

std::vector<float> BoundEnvironment::clear_color() const {
    const auto& value = environment_component(entity_).clear_color_;
    return { value.x, value.y, value.z, value.w };
}

void BoundEnvironment::set_clear_color(const std::vector<float>& value) {
    require_size(value, 4, "clear_color");
    environment_component(entity_).clear_color_ = { value[0], value[1], value[2], value[3] };
}
#pragma endregion
// clang-format on
} // namespace vulcal::script::lua
