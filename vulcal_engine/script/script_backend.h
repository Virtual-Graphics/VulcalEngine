#pragma once

#include "scene/entity.h"

#include <filesystem>
#include <memory>
#include <string_view>

namespace vulcal::scene {
class Scene;
}

namespace vulcal::script {

struct ScriptContext {
    scene::Scene& scene;
    scene::Entity entity;
};

class ScriptInstance {
public:
    virtual ~ScriptInstance()                = default;
    virtual void on_create()                 = 0;
    virtual void on_update(float delta_time) = 0;
    virtual void on_destroy()                = 0;
};

class ScriptBackend {
public:
    virtual ~ScriptBackend()                                    = default;
    virtual std::string_view extension() const                  = 0;
    virtual std::unique_ptr<ScriptInstance> create(const ScriptContext& context,
            const std::filesystem::path& script_path) = 0;
};

} // namespace vulcal::script
