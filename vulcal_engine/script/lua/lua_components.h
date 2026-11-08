#pragma once

#include "scene/entity.h"

#include <string>
#include <vector>

namespace vulcal::script::lua {

class BoundSprite {
public:
    explicit BoundSprite(scene::Entity entity);

    bool visible() const;
    void set_visible(bool value);

    std::vector<float> color() const;
    void set_color(const std::vector<float>& value);

    std::vector<float> uv_rect() const;
    void set_uv_rect(const std::vector<float>& value);

    std::vector<float> pivot() const;
    void set_pivot(const std::vector<float>& value);

    bool flip_x() const;
    void set_flip_x(bool value);

    bool flip_y() const;
    void set_flip_y(bool value);

    int sorting_order() const;
    void set_sorting_order(int value);

    std::string texture_id() const;

private:
    scene::Entity entity_;
};

class BoundCamera {
public:
    explicit BoundCamera(scene::Entity entity);

    std::string projection() const;
    void set_projection(const std::string& value);

    bool primary() const;
    void set_primary(bool value);

    float fov() const;
    void set_fov(float degrees);

    float near_plane() const;
    void set_near_plane(float value);

    float far_plane() const;
    void set_far_plane(float value);

    float ortho_left() const;
    void set_ortho_left(float value);

    float ortho_right() const;
    void set_ortho_right(float value);

    float ortho_bottom() const;
    void set_ortho_bottom(float value);

    float ortho_top() const;
    void set_ortho_top(float value);

private:
    scene::Entity entity_;
};

class BoundLight {
public:
    explicit BoundLight(scene::Entity entity);

    std::vector<float> color() const;
    void set_color(const std::vector<float>& value);

    float intensity() const;
    void set_intensity(float value);

    bool shadow_enabled() const;
    void set_shadow_enabled(bool value);

private:
    scene::Entity entity_;
};

class BoundMesh {
public:
    explicit BoundMesh(scene::Entity entity);

    bool visible() const;
    void set_visible(bool value);

    std::string mesh_id() const;
    std::string material_id() const;

private:
    scene::Entity entity_;
};

class BoundEnvironment {
public:
    explicit BoundEnvironment(scene::Entity entity);

    std::vector<float> ambient_color() const;
    void set_ambient_color(const std::vector<float>& value);

    std::vector<float> clear_color() const;
    void set_clear_color(const std::vector<float>& value);

private:
    scene::Entity entity_;
};

} // namespace vulcal::script::lua
