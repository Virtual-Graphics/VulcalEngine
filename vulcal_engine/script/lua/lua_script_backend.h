#pragma once

#include "script/script_backend.h"

namespace vulcal::script::lua {

class LuaScriptBackend final : public ScriptBackend {
public:
    std::string_view extension() const override;
    std::unique_ptr<ScriptInstance> create(const ScriptContext& context, const std::filesystem::path& script_path) override;
    std::unique_ptr<ScriptInstance> create_source(const ScriptContext& context, std::string_view source,
                                                  std::string_view source_name) override;
};

} // namespace vulcal::script::lua
