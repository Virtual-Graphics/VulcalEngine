#pragma once

#include "script/script_backend.h"

namespace vulcal::script {

class LuaScriptBackend final : public ScriptBackend {
public:
    std::string_view extension() const override;
    std::unique_ptr<ScriptInstance> create(scene::Entity entity, const std::filesystem::path& script_path) override;
};

} // namespace vulcal::script
