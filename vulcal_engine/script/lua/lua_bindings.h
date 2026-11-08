#pragma once

#include "scene/entity.h"
#include "script/lua/lua_time.h"

#include <sol/sol.hpp>

namespace vulcal::scene {
class Scene;
}

namespace vulcal::script::lua {

struct LuaBindingContext {
    sol::state& lua;
    scene::Scene& scene;
    scene::Entity entity;
    TimeState& time;
};

void bind_all(LuaBindingContext& context);

} // namespace vulcal::script::lua
