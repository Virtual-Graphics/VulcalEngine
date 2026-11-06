#include "script/lua_script_backend.h"

#include "base/input.h"
#include "base/key_code.h"
#include "base/logger.h"
#include "scene/components.h"
#include "scene/scene.h"

#include <sol/sol.hpp>

#include <stdexcept>
#include <string>
#include <tuple>
#include <utility>

namespace vulcal::script {
namespace {

class BoundEntity {
public:
    explicit BoundEntity(scene::Entity entity)
            : entity_(entity) {}

    std::string name() const { return entity_.get_component<scene::TagComponent>().name_; }
    std::tuple<float, float, float> get_translation() const {
        const auto& value = entity_.get_component<scene::TransformComponent>().translation_;
        return { value.x, value.y, value.z };
    }
    void set_translation(float x, float y, float z) { entity_.get_component<scene::TransformComponent>().translation_ = { x, y, z }; }
    std::tuple<float, float, float> get_rotation() const {
        const auto& value = entity_.get_component<scene::TransformComponent>().rotation_;
        return { value.x, value.y, value.z };
    }
    void set_rotation(float x, float y, float z) { entity_.get_component<scene::TransformComponent>().rotation_ = { x, y, z }; }
    std::tuple<float, float, float> get_scale() const {
        const auto& value = entity_.get_component<scene::TransformComponent>().scale_;
        return { value.x, value.y, value.z };
    }
    void set_scale(float x, float y, float z) { entity_.get_component<scene::TransformComponent>().scale_ = { x, y, z }; }

private:
    scene::Entity entity_;
};

void add_key(sol::table& keys, const char* name, base::Key key) { keys[name] = static_cast<int>(key); }

class LuaInstance final : public ScriptInstance {
public:
    LuaInstance(scene::Entity entity, const std::filesystem::path& path) {
        lua_.open_libraries(sol::lib::base, sol::lib::coroutine, sol::lib::math, sol::lib::string, sol::lib::table, sol::lib::utf8);
        lua_["dofile"]         = sol::lua_nil;
        lua_["loadfile"]       = sol::lua_nil;
        lua_["load"]           = sol::lua_nil;
        lua_["collectgarbage"] = sol::lua_nil;
        bind_entity(entity);
        bind_input();
        bind_log();

        const auto result = lua_.safe_script_file(path.string(), sol::script_pass_on_error);
        if (!result.valid()) throw std::runtime_error(result.template get<sol::error>().what());
    }

    void on_create() override { call("on_create"); }
    void on_update(float delta_time) override { call("on_update", delta_time); }
    void on_destroy() override { call("on_destroy"); }

private:
    void bind_entity(scene::Entity entity) {
        lua_.new_usertype<BoundEntity>("BoundEntity", sol::no_constructor, "name", sol::property(&BoundEntity::name), "get_translation",
                                       &BoundEntity::get_translation, "set_translation", &BoundEntity::set_translation, "get_rotation",
                                       &BoundEntity::get_rotation, "set_rotation", &BoundEntity::set_rotation, "get_scale",
                                       &BoundEntity::get_scale, "set_scale", &BoundEntity::set_scale);
        lua_["entity"] = BoundEntity{ entity };
    }

    void bind_input() {
        auto input                    = lua_.create_table();
        input["is_key_down"]          = [](int key) { return base::Input::is_key_down(static_cast<base::Key>(key)); };
        input["is_key_pressed"]       = [](int key) { return base::Input::is_key_pressed(static_cast<base::Key>(key)); };
        input["is_key_released"]      = [](int key) { return base::Input::is_key_released(static_cast<base::Key>(key)); };
        input["is_mouse_button_down"] = [](int button) {
            return base::Input::is_mouse_button_down(static_cast<base::MouseButton>(button));
        };
        input["is_mouse_button_pressed"] = [](int button) {
            return base::Input::is_mouse_button_pressed(static_cast<base::MouseButton>(button));
        };
        input["is_mouse_button_released"] = [](int button) {
            return base::Input::is_mouse_button_released(static_cast<base::MouseButton>(button));
        };
        input["mouse_x"]       = &base::Input::mouse_x;
        input["mouse_y"]       = &base::Input::mouse_y;
        input["mouse_delta_x"] = &base::Input::mouse_delta_x;
        input["mouse_delta_y"] = &base::Input::mouse_delta_y;
        input["mouse_wheel_x"] = &base::Input::mouse_wheel_x;
        input["mouse_wheel_y"] = &base::Input::mouse_wheel_y;
        lua_["Input"]          = input;

        auto keys              = lua_.create_table();
#define ADD_KEY(name) add_key(keys, #name, base::Key::name)
        ADD_KEY(Unknown);
        ADD_KEY(A);
        ADD_KEY(B);
        ADD_KEY(C);
        ADD_KEY(D);
        ADD_KEY(E);
        ADD_KEY(F);
        ADD_KEY(G);
        ADD_KEY(H);
        ADD_KEY(I);
        ADD_KEY(J);
        ADD_KEY(K);
        ADD_KEY(L);
        ADD_KEY(M);
        ADD_KEY(N);
        ADD_KEY(O);
        ADD_KEY(P);
        ADD_KEY(Q);
        ADD_KEY(R);
        ADD_KEY(S);
        ADD_KEY(T);
        ADD_KEY(U);
        ADD_KEY(V);
        ADD_KEY(W);
        ADD_KEY(X);
        ADD_KEY(Y);
        ADD_KEY(Z);
        ADD_KEY(Num0);
        ADD_KEY(Num1);
        ADD_KEY(Num2);
        ADD_KEY(Num3);
        ADD_KEY(Num4);
        ADD_KEY(Num5);
        ADD_KEY(Num6);
        ADD_KEY(Num7);
        ADD_KEY(Num8);
        ADD_KEY(Num9);
        ADD_KEY(Escape);
        ADD_KEY(Enter);
        ADD_KEY(Tab);
        ADD_KEY(Backspace);
        ADD_KEY(Space);
        ADD_KEY(Delete);
        ADD_KEY(Insert);
        ADD_KEY(Home);
        ADD_KEY(End);
        ADD_KEY(PageUp);
        ADD_KEY(PageDown);
        ADD_KEY(Left);
        ADD_KEY(Right);
        ADD_KEY(Up);
        ADD_KEY(Down);
        ADD_KEY(F1);
        ADD_KEY(F2);
        ADD_KEY(F3);
        ADD_KEY(F4);
        ADD_KEY(F5);
        ADD_KEY(F6);
        ADD_KEY(F7);
        ADD_KEY(F8);
        ADD_KEY(F9);
        ADD_KEY(F10);
        ADD_KEY(F11);
        ADD_KEY(F12);
        ADD_KEY(LeftShift);
        ADD_KEY(RightShift);
        ADD_KEY(LeftCtrl);
        ADD_KEY(RightCtrl);
        ADD_KEY(LeftAlt);
        ADD_KEY(RightAlt);
        ADD_KEY(LeftSuper);
        ADD_KEY(RightSuper);
        ADD_KEY(CapsLock);
        ADD_KEY(Minus);
        ADD_KEY(Equal);
        ADD_KEY(LeftBracket);
        ADD_KEY(RightBracket);
        ADD_KEY(Backslash);
        ADD_KEY(Semicolon);
        ADD_KEY(Apostrophe);
        ADD_KEY(Comma);
        ADD_KEY(Period);
        ADD_KEY(Slash);
        ADD_KEY(Grave);
        ADD_KEY(Count);
#undef ADD_KEY
        lua_["Key"]         = keys;

        auto buttons        = lua_.create_table();
        buttons["Left"]     = static_cast<int>(base::MouseButton::Left);
        buttons["Middle"]   = static_cast<int>(base::MouseButton::Middle);
        buttons["Right"]    = static_cast<int>(base::MouseButton::Right);
        buttons["X1"]       = static_cast<int>(base::MouseButton::X1);
        buttons["X2"]       = static_cast<int>(base::MouseButton::X2);
        lua_["MouseButton"] = buttons;
    }

    void bind_log() {
        auto log     = lua_.create_table();
        log["trace"] = [](const std::string& value) { base::log::trace("{}", value); };
        log["debug"] = [](const std::string& value) { base::log::debug("{}", value); };
        log["info"]  = [](const std::string& value) { base::log::info("{}", value); };
        log["warn"]  = [](const std::string& value) { base::log::warn("{}", value); };
        log["error"] = [](const std::string& value) { base::log::error("{}", value); };
        lua_["Log"]  = log;
    }

    template<typename... Args>
    void call(const char* callback_name, Args&&... args) {
        const sol::object callback = lua_[callback_name];
        if (callback.get_type() == sol::type::none || callback.get_type() == sol::type::lua_nil) return;
        if (!callback.is<sol::protected_function>()) throw std::runtime_error(std::string(callback_name) + " must be a function");
        const auto result = callback.as<sol::protected_function>()(std::forward<Args>(args)...);
        if (!result.valid()) throw std::runtime_error(result.template get<sol::error>().what());
    }

    sol::state lua_;
};

} // namespace

std::string_view LuaScriptBackend::extension() const { return ".lua"; }

std::unique_ptr<ScriptInstance> LuaScriptBackend::create(scene::Entity entity, const std::filesystem::path& script_path) {
    return std::make_unique<LuaInstance>(entity, script_path);
}

} // namespace vulcal::script
