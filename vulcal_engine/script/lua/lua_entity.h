#pragma once

#include "scene/entity.h"
#include "script/lua/lua_components.h"

#include <optional>
#include <string>
#include <tuple>

namespace vulcal::script::lua {

class BoundEntity {
public:
    explicit BoundEntity(scene::Entity entity);

    std::string name() const;
    std::string id() const;
    bool valid() const;

    std::tuple<float, float, float> get_translation() const;
    void set_translation(float x, float y, float z);
    std::tuple<float, float, float> get_rotation() const;
    void set_rotation(float x, float y, float z);
    std::tuple<float, float, float> get_scale() const;
    void set_scale(float x, float y, float z);

    void destroy();

    std::optional<BoundSprite> sprite() const;
    std::optional<BoundCamera> camera() const;
    std::optional<BoundLight> light() const;
    std::optional<BoundMesh> mesh() const;
    std::optional<BoundEnvironment> environment() const;

    BoundSprite add_sprite();
    BoundSprite add_sprite(const std::string& texture_id);
    BoundCamera add_camera();
    BoundLight add_light();
    BoundMesh add_mesh(const std::string& mesh_id, const std::string& material_id);

    void remove_sprite();
    void remove_camera();
    void remove_light();
    void remove_mesh();

private:
    scene::Entity entity_;
};

} // namespace vulcal::script::lua
