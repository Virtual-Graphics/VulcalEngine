#include "script/lua/lua_bindings.h"

#include "script/lua/lua_components.h"
#include "script/lua/lua_entity.h"
#include "script/lua/lua_input.h"
#include "script/lua/lua_log.h"
#include "script/lua/lua_scene.h"

#include <cstddef>
#include <stdexcept>
#include <vector>

namespace vulcal::script::lua {
namespace {

std::vector<float> read_floats(const sol::table& table) {
    std::vector<float> values;
    values.reserve(table.size());
    for (std::size_t index = 1; index <= table.size(); ++index) {
        const sol::object value = table[index];
        if (value.get_type() != sol::type::number) throw std::invalid_argument("expected an array of numbers");
        values.push_back(value.as<float>());
    }
    return values;
}

} // namespace

// clang-format off
void bind_all(LuaBindingContext& context) {
    auto& lua = context.lua;

    lua.new_usertype<BoundSprite>(
            "SpriteRenderer", 
            sol::no_constructor, 

            "visible", 
            sol::property(&BoundSprite::visible, &BoundSprite::set_visible), 
            "color", 
            sol::property(&BoundSprite::color, [](BoundSprite& sprite, const sol::table& value) { sprite.set_color(read_floats(value)); }),
            "uv_rect", 
            sol::property(&BoundSprite::uv_rect,[](BoundSprite& sprite, const sol::table& value) { sprite.set_uv_rect(read_floats(value)); }),
            "pivot", 
            sol::property(&BoundSprite::pivot, [](BoundSprite& sprite, const sol::table& value) { sprite.set_pivot(read_floats(value)); }),
            "flip_x", 
            sol::property(&BoundSprite::flip_x, &BoundSprite::set_flip_x), 
            "flip_y", 
            sol::property(&BoundSprite::flip_y, &BoundSprite::set_flip_y), 
            "sorting_order", 
            sol::property(&BoundSprite::sorting_order, &BoundSprite::set_sorting_order), 
            "texture_id", 
            sol::readonly_property(&BoundSprite::texture_id));

    lua.new_usertype<BoundCamera>(
            "Camera", 
            sol::no_constructor, 

            "projection", 
            sol::property(&BoundCamera::projection, &BoundCamera::set_projection), 
            "primary", 
            sol::property(&BoundCamera::primary, &BoundCamera::set_primary), 
            "fov", 
            sol::property(&BoundCamera::fov, &BoundCamera::set_fov),
            "near", 
            sol::property(&BoundCamera::near_plane, &BoundCamera::set_near_plane), 
            "far", 
            sol::property(&BoundCamera::far_plane, &BoundCamera::set_far_plane), 
            "left", 
            sol::property(&BoundCamera::ortho_left, &BoundCamera::set_ortho_left), 
            "right", 
            sol::property(&BoundCamera::ortho_right, &BoundCamera::set_ortho_right), 
            "bottom", 
            sol::property(&BoundCamera::ortho_bottom, &BoundCamera::set_ortho_bottom), 
            "top", 
            sol::property(&BoundCamera::ortho_top, &BoundCamera::set_ortho_top));

    lua.new_usertype<BoundLight>(
            "Light", 
            sol::no_constructor, 

            "color", 
            sol::property(&BoundLight::color, [](BoundLight& light, const sol::table& value) { light.set_color(read_floats(value)); }),
            "intensity", 
            sol::property(&BoundLight::intensity, &BoundLight::set_intensity), 
            "shadow_enabled", 
            sol::property(&BoundLight::shadow_enabled, &BoundLight::set_shadow_enabled));

    lua.new_usertype<BoundMesh>(
            "MeshRenderer", 
            sol::no_constructor,

            "visible", 
            sol::property(&BoundMesh::visible, &BoundMesh::set_visible),
            "mesh_id", 
            sol::readonly_property(&BoundMesh::mesh_id), 
            "material_id",
            sol::readonly_property(&BoundMesh::material_id));

    lua.new_usertype<BoundEnvironment>(
            "Environment", 
            sol::no_constructor, 

            "ambient_color",
            sol::property(&BoundEnvironment::ambient_color,
                [](BoundEnvironment& environment, const sol::table& value) { environment.set_ambient_color(read_floats(value)); }),

            "clear_color", 
            sol::property(&BoundEnvironment::clear_color, 
                [](BoundEnvironment& environment, const sol::table& value) {environment.set_clear_color(read_floats(value));}));

    auto entity_type = lua.new_usertype<BoundEntity>(
            "Entity", 
            sol::no_constructor, 

            "name",
            sol::readonly_property(&BoundEntity::name), 
            "id", 
            sol::readonly_property(&BoundEntity::id), 
            "valid", 
            sol::readonly_property(&BoundEntity::valid),

            "get_translation",
            &BoundEntity::get_translation, 
            "set_translation", 
            &BoundEntity::set_translation, 
            "get_rotation", 
            &BoundEntity::get_rotation,
            "set_rotation", 
            &BoundEntity::set_rotation, 
            "get_scale", 
            &BoundEntity::get_scale, 
            "set_scale", 
            &BoundEntity::set_scale,

            "sprite", 
            sol::property(&BoundEntity::sprite), 
            "camera", 
            sol::property(&BoundEntity::camera), 
            "light",
            sol::property(&BoundEntity::light), 
            "mesh", 
            sol::property(&BoundEntity::mesh), 
            "environment",
            sol::property(&BoundEntity::environment), 

            "destroy", 
            &BoundEntity::destroy, 
            "add_camera", 
            &BoundEntity::add_camera, 
            "add_light",
            &BoundEntity::add_light, 
            "add_mesh", 
            &BoundEntity::add_mesh, 
            "remove_sprite", 
            &BoundEntity::remove_sprite, 
            "remove_camera",
            &BoundEntity::remove_camera, 
            "remove_light", 
            &BoundEntity::remove_light, 
            "remove_mesh", 
            &BoundEntity::remove_mesh);

    entity_type.set_function(
        "add_sprite",
        sol::overload(static_cast<BoundSprite (BoundEntity::*)()>(&BoundEntity::add_sprite),
                        static_cast<BoundSprite (BoundEntity::*)(const std::string&)>(&BoundEntity::add_sprite)));

    lua.new_usertype<BoundScene>(
        "Scene", sol::no_constructor,

        "find", 
        &BoundScene::find, 
        "find_by_id", 
        &BoundScene::find_by_id,
        "create_entity", 
        &BoundScene::create_entity, 
        "destroy", 
        &BoundScene::destroy, 
        "entities",
        &BoundScene::entities, 

        "environment", 
        sol::readonly_property(&BoundScene::environment));

    lua.new_usertype<BoundTime>(
        "Time", sol::no_constructor, 

        "delta", 
        sol::readonly_property(&BoundTime::delta), 
        "elapsed", 
        sol::readonly_property(&BoundTime::elapsed), 
        "frame", 
        sol::readonly_property(&BoundTime::frame));

    lua["entity"] = BoundEntity{ context.entity };
    lua["scene"]  = BoundScene{ context.scene };
    lua["Time"]   = BoundTime{ context.time };

    bind_input(lua);
    bind_log(lua);
}
// clang-format on
} // namespace vulcal::script::lua
