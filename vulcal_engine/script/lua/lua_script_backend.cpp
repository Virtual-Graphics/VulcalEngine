#include "script/lua/lua_script_backend.h"

#include "script/lua/lua_bindings.h"

#include <sol/sol.hpp>

#include <stdexcept>
#include <string>
#include <utility>

namespace vulcal::script::lua {
namespace {

class LuaInstance final : public ScriptInstance {
public:
    LuaInstance(const ScriptContext& context, const std::filesystem::path& path) {
        lua_.open_libraries(sol::lib::base, sol::lib::coroutine, sol::lib::math, sol::lib::string, sol::lib::table, sol::lib::utf8);
        lua_["dofile"]         = sol::lua_nil;
        lua_["loadfile"]       = sol::lua_nil;
        lua_["load"]           = sol::lua_nil;
        lua_["collectgarbage"] = sol::lua_nil;

        LuaBindingContext bindings{ lua_, context.scene, context.entity, time_ };
        bind_all(bindings);

        const auto result = lua_.safe_script_file(path.string(), sol::script_pass_on_error);
        if (!result.valid()) throw std::runtime_error(result.template get<sol::error>().what());
    }

    void on_create() override { call("on_create"); }

    void on_update(float delta_time) override {
        time_.delta = delta_time;
        time_.elapsed += delta_time;
        ++time_.frame;
        call("on_update", delta_time);
    }

    void on_destroy() override { call("on_destroy"); }

private:
    template<typename... Args>
    void call(const char* callback_name, Args&&... args) {
        const sol::object callback = lua_[callback_name];
        if (callback.get_type() == sol::type::none || callback.get_type() == sol::type::lua_nil) return;
        if (!callback.is<sol::protected_function>()) throw std::runtime_error(std::string(callback_name) + " must be a function");
        const auto result = callback.as<sol::protected_function>()(std::forward<Args>(args)...);
        if (!result.valid()) throw std::runtime_error(result.template get<sol::error>().what());
    }

    TimeState time_;
    sol::state lua_;
};

} // namespace

std::string_view LuaScriptBackend::extension() const { return ".lua"; }

std::unique_ptr<ScriptInstance> LuaScriptBackend::create(const ScriptContext& context, const std::filesystem::path& script_path) {
    return std::make_unique<LuaInstance>(context, script_path);
}

} // namespace vulcal::script::lua
