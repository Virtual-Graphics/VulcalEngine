#pragma once

#include "scene/scene.h"
#include "script/script_backend.h"

#include <filesystem>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace vulcal::script {

class ScriptRuntime {
public:
    ScriptRuntime();
    ~ScriptRuntime();
    ScriptRuntime(const ScriptRuntime&)            = delete;
    ScriptRuntime& operator=(const ScriptRuntime&) = delete;

    void register_backend(std::unique_ptr<ScriptBackend> backend);
    void start(scene::Scene& scene, const std::filesystem::path& project_root, const std::filesystem::path& assets_root);
    void update(float delta_time);
    void stop();
    bool running() const { return scene_ != nullptr; }

private:
    struct Entry {
        scene::Entity entity;
        std::unique_ptr<ScriptInstance> instance;
        std::string entity_id;
        std::string script_path;
        bool active = true;
    };

    std::unordered_map<std::string, std::unique_ptr<ScriptBackend>> backends_;
    std::vector<Entry> entries_;
    scene::Scene* scene_ = nullptr;
};

} // namespace vulcal::script
