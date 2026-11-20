#include "script/script_runtime.h"

#include "base/profiler.h"

#include "base/logger.h"
#include "base/path_utils.h"
#include "base/string_utils.h"
#include "resource/directory_provider.h"
#include "resource/resource_provider.h"
#include "scene/components.h"
#include "script/lua/lua_script_backend.h"

#include <stdexcept>

namespace vulcal::script {
namespace {

std::string extension_key(std::string_view extension) {
    auto key = base::to_lower(extension);
    if (!key.empty() && key.front() != '.') key.insert(key.begin(), '.');
    return key;
}

} // namespace

ScriptRuntime::ScriptRuntime() { register_backend(std::make_unique<lua::LuaScriptBackend>()); }

ScriptRuntime::~ScriptRuntime() { stop(); }

void ScriptRuntime::register_backend(std::unique_ptr<ScriptBackend> backend) {
    if (!backend) throw std::invalid_argument("Script backend cannot be null");
    auto extension = extension_key(backend->extension());
    if (extension.empty() || extension == ".") throw std::invalid_argument("Backend extension is empty");
    backends_.insert_or_assign(std::move(extension), std::move(backend));
}

void ScriptRuntime::start(scene::Scene& scene, const std::filesystem::path& project_root) {
    if (scene.registry().view<scene::ScriptComponent>().empty()) {
        stop();
        scene_ = &scene;
        return;
    }
    const resource::DirectoryProvider resources(project_root);
    start(scene, resources);
}

void ScriptRuntime::start(scene::Scene& scene, const resource::ResourceProvider& resources) {
    VULCAL_PROFILE_SCOPE();
    stop();
    scene_ = &scene;
    scene.each<scene::ScriptComponent>([&](scene::Entity entity, scene::ScriptComponent& component) {
        const auto entity_id      = entity.get_component<scene::IDComponent>().id_.to_string();
        const auto requested_path = component.script_path_;
        try {
            const auto logical_path = base::path_from_utf8(requested_path);
            if (requested_path.empty() || logical_path.is_absolute() || logical_path.has_root_name() || logical_path.has_root_directory() ||
                requested_path.find('\\') != std::string::npos || requested_path.find(':') != std::string::npos) {
                throw std::runtime_error("Script path must be project-relative");
            }
            const auto backend = backends_.find(extension_key(logical_path.extension().string()));
            if (backend == backends_.end()) {
                throw std::runtime_error("No backend for script extension");
            }

            if (!resources.contains(requested_path)) throw std::runtime_error("Script file does not exist");
            const auto source = resources.read_text(requested_path);

            VULCAL_PROFILE_SCOPE_NAMED("Script create");
            VULCAL_PROFILE_TEXT(requested_path.data(), requested_path.size());
            auto instance = backend->second->create_source(ScriptContext{ scene, entity }, source, requested_path);
            if (!instance) {
                throw std::runtime_error("Backend returned an empty script instance");
            }

            entries_.push_back(Entry{ entity, std::move(instance), entity_id, requested_path, true });
            try {
                entries_.back().instance->on_create();
            } catch (const std::exception& error) {
                entries_.back().active = false;
                base::log::error("Script on_create failed for entity {} at '{}': {}", entity_id, requested_path, error.what());
            } catch (...) {
                entries_.back().active = false;
                base::log::error("Script on_create failed for entity {} at '{}'", entity_id, requested_path);
            }
        } catch (const std::exception& error) {
            base::log::error("Could not start script for entity {} at '{}': {}", entity_id, requested_path, error.what());
        } catch (...) {
            base::log::error("Could not start script for entity {} at '{}'", entity_id, requested_path);
        }
    });
}

void ScriptRuntime::update(float delta_time) {
    VULCAL_PROFILE_SCOPE();
    VULCAL_PROFILE_PLOT("Script/Instances", entries_.size());
    auto entry = entries_.begin();
    while (entry != entries_.end()) {
        if (!entry->entity.valid() || !entry->entity.has_component<scene::ScriptComponent>()) {
            try {
                entry->instance->on_destroy();
            } catch (const std::exception& error) {
                base::log::error("Script on_destroy failed for entity {} at '{}': {}", entry->entity_id, entry->script_path, error.what());
            } catch (...) {
                base::log::error("Script on_destroy failed for entity {} at '{}'", entry->entity_id, entry->script_path);
            }
            entry = entries_.erase(entry);
            continue;
        }
        if (entry->active) {
            try {
                VULCAL_PROFILE_SCOPE_NAMED("Script on_update");
                VULCAL_PROFILE_TEXT(entry->script_path.data(), entry->script_path.size());
                entry->instance->on_update(delta_time);
            } catch (const std::exception& error) {
                entry->active = false;
                base::log::error("Script on_update failed for entity {} at '{}': {}", entry->entity_id, entry->script_path, error.what());
            } catch (...) {
                entry->active = false;
                base::log::error("Script on_update failed for entity {} at '{}'", entry->entity_id, entry->script_path);
            }
        }
        ++entry;
    }
}

void ScriptRuntime::stop() {
    VULCAL_PROFILE_SCOPE();
    for (auto& entry: entries_) {
        try {
            entry.instance->on_destroy();
        } catch (const std::exception& error) {
            base::log::error("Script on_destroy failed for entity {} at '{}': {}", entry.entity_id, entry.script_path, error.what());
        } catch (...) {
            base::log::error("Script on_destroy failed for entity {} at '{}'", entry.entity_id, entry.script_path);
        }
    }
    entries_.clear();
    scene_ = nullptr;
}

} // namespace vulcal::script
