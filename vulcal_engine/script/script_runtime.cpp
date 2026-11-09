#include "script/script_runtime.h"

#include "base/logger.h"
#include "base/path_utils.h"
#include "base/string_utils.h"
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

std::filesystem::path checked_script_path(const std::string& script_path, const std::filesystem::path& project_root) {
    const std::filesystem::path relative{ script_path };
    if (relative.empty() || relative.is_absolute() || relative.has_root_name() || relative.has_root_directory()) {
        throw std::runtime_error("Script path must be project-relative");
    }

    std::error_code error;
    const auto project = std::filesystem::canonical(project_root, error);
    if (error || !std::filesystem::is_directory(project)) throw std::runtime_error("Project root does not exist");
    const auto script = std::filesystem::canonical(project / relative, error);
    if (error || !std::filesystem::is_regular_file(script)) throw std::runtime_error("Script file does not exist");
    if (!base::is_within(script, project)) throw std::runtime_error("Script resolves outside the project root");
    return script;
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
    stop();
    scene_ = &scene;
    scene.each<scene::ScriptComponent>([&](scene::Entity entity, scene::ScriptComponent& component) {
        const auto entity_id      = entity.get_component<scene::IDComponent>().id_.to_string();
        const auto requested_path = component.script_path_;
        try {
            const auto path    = checked_script_path(requested_path, project_root);
            const auto backend = backends_.find(extension_key(path.extension().string()));
            if (backend == backends_.end()) {
                throw std::runtime_error("No backend for script extension");
            }

            auto instance = backend->second->create(ScriptContext{ scene, entity }, path);
            if (!instance) {
                throw std::runtime_error("Backend returned an empty script instance");
            }

            entries_.push_back(Entry{ entity, std::move(instance), entity_id, path.string(), true });
            try {
                entries_.back().instance->on_create();
            } catch (const std::exception& error) {
                entries_.back().active = false;
                base::log::error("Script on_create failed for entity {} at '{}': {}", entity_id, path.string(), error.what());
            } catch (...) {
                entries_.back().active = false;
                base::log::error("Script on_create failed for entity {} at '{}'", entity_id, path.string());
            }
        } catch (const std::exception& error) {
            base::log::error("Could not start script for entity {} at '{}': {}", entity_id, requested_path, error.what());
        } catch (...) {
            base::log::error("Could not start script for entity {} at '{}'", entity_id, requested_path);
        }
    });
}

void ScriptRuntime::update(float delta_time) {
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
