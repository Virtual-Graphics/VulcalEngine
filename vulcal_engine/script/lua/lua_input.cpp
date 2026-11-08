#include "script/lua/lua_input.h"

#include "base/input.h"
#include "base/key_code.h"

#include <tuple>

namespace vulcal::script::lua {
namespace {

void add_key(sol::table& keys, const char* name, base::Key key) { keys[name] = static_cast<int>(key); }

} // namespace

// clang-format off
void bind_input(sol::state& lua) {
    auto input                       = lua.create_table();
    input["is_key_down"]             = [](int key) { return base::Input::is_key_down(static_cast<base::Key>(key)); };
    input["is_key_pressed"]          = [](int key) { return base::Input::is_key_pressed(static_cast<base::Key>(key)); };
    input["is_key_released"]         = [](int key) { return base::Input::is_key_released(static_cast<base::Key>(key)); };
    input["is_mouse_button_down"]    = [](int button) { return base::Input::is_mouse_button_down(static_cast<base::MouseButton>(button)); };
    input["is_mouse_button_pressed"] = [](int button) {return base::Input::is_mouse_button_pressed(static_cast<base::MouseButton>(button));};
    input["is_mouse_button_released"] = [](int button) {return base::Input::is_mouse_button_released(static_cast<base::MouseButton>(button));};
    input["mouse_x"]        = &base::Input::mouse_x;
    input["mouse_y"]        = &base::Input::mouse_y;
    input["mouse_delta_x"]  = &base::Input::mouse_delta_x;
    input["mouse_delta_y"]  = &base::Input::mouse_delta_y;
    input["mouse_wheel_x"]  = &base::Input::mouse_wheel_x;
    input["mouse_wheel_y"]  = &base::Input::mouse_wheel_y;
    input["mouse_position"] = [] { return std::make_tuple(base::Input::mouse_x(), base::Input::mouse_y()); };
    input["key_axis"]       = [](int negative, int positive) {
        const float negative_down = base::Input::is_key_down(static_cast<base::Key>(negative)) ? 1.0f : 0.0f;
        const float positive_down = base::Input::is_key_down(static_cast<base::Key>(positive)) ? 1.0f : 0.0f;
        return positive_down - negative_down;
    };
    lua["Input"] = input;

    auto keys    = lua.create_table();
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
    lua["Key"]         = keys;

    auto buttons       = lua.create_table();
    buttons["Left"]    = static_cast<int>(base::MouseButton::Left);
    buttons["Middle"]  = static_cast<int>(base::MouseButton::Middle);
    buttons["Right"]   = static_cast<int>(base::MouseButton::Right);
    buttons["X1"]      = static_cast<int>(base::MouseButton::X1);
    buttons["X2"]      = static_cast<int>(base::MouseButton::X2);
    lua["MouseButton"] = buttons;
}
// clang-format on
} // namespace vulcal::script::lua
