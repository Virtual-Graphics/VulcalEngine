#pragma once

#include "script/lua/lua_entity.h"

#include <optional>
#include <string>
#include <vector>

namespace vulcal::scene {
class Scene;
}

namespace vulcal::script::lua {

class BoundScene {
public:
    explicit BoundScene(scene::Scene& scene);

    std::optional<BoundEntity> find(const std::string& name) const;
    std::optional<BoundEntity> find_by_id(const std::string& id) const;
    BoundEntity create_entity(const std::string& name);
    void destroy(BoundEntity entity) const;
    std::vector<BoundEntity> entities() const;
    std::optional<BoundEnvironment> environment() const;

private:
    scene::Scene& scene_;
};

} // namespace vulcal::script::lua
