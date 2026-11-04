#include "editor/editor_layer.h"
#include "engine/application.h"

#include <SDL3/SDL.h>
#include <gtest/gtest.h>
#include <imgui.h>
#include <imgui_internal.h>
#include <memory>

namespace {

class EditorLayerTest : public testing::Test {
protected:
    void SetUp() override {
        application_ = std::make_unique<engine::Application>(engine::ApplicationCreateInfo{
            .width  = 1000,
            .height = 700,
            .title  = "Editor input test",
        });
        auto layer   = std::make_unique<editor::EditorLayer>();
        layer_       = layer.get();
        application_->push_layer(std::move(layer));
        // Feed ImGui input directly so OS mouse motion cannot overwrite test events.
        application_->get_window().set_event_callback({});
        SDL_HideWindow(application_->get_window().handle());
        ImGui::GetIO().IniFilename                  = nullptr;
        ImGui::GetIO().ConfigInputTrickleEventQueue = false;
        frame();
        frame();
        frame();
    }

    void frame() {
        application_->get_window().poll_events();
        layer_->on_update(base::Timestep{ 1.0f / 60.0f });
        layer_->on_render();
    }

    void click(ImVec2 position, int button = ImGuiMouseButton_Left) {
        auto& io = ImGui::GetIO();
        io.AddMousePosEvent(position.x, position.y);
        frame();
        io.AddMouseButtonEvent(button, true);
        frame();
        io.AddMouseButtonEvent(button, false);
        frame();
    }

    void add_entity() {
        const auto* hierarchy = ImGui::FindWindowByName("Hierarchy");
        ASSERT_NE(hierarchy, nullptr);
        click({ hierarchy->Pos.x + 40.0f, hierarchy->Pos.y + 300.0f }, ImGuiMouseButton_Right);
        frame();
        ASSERT_EQ(ImGui::GetCurrentContext()->OpenPopupStack.Size, 1);
        auto* popup = ImGui::GetCurrentContext()->OpenPopupStack[0].Window;
        ASSERT_NE(popup, nullptr);
        click({ popup->DC.CursorStartPos.x + 30.0f, popup->DC.CursorStartPos.y + ImGui::GetFontSize() * 0.5f });
        frame();
        frame();
    }

    void new_scene_shortcut() {
        auto& io = ImGui::GetIO();
        io.AddKeyEvent(ImGuiMod_Ctrl, true);
        io.AddKeyEvent(ImGuiKey_N, true);
        frame();
        io.AddKeyEvent(ImGuiKey_N, false);
        io.AddKeyEvent(ImGuiMod_Ctrl, false);
        frame();
        frame();
    }

    float hierarchy_height() const { return ImGui::FindWindowByName("Hierarchy")->ContentSize.y; }

    std::unique_ptr<engine::Application> application_;
    editor::EditorLayer* layer_ = nullptr;
};

TEST_F(EditorLayerTest, NewSceneShortcutReplacesScene) {
    const auto initial_height = hierarchy_height();
    add_entity();
    ASSERT_GT(hierarchy_height(), initial_height);
    new_scene_shortcut();
    EXPECT_FLOAT_EQ(hierarchy_height(), initial_height);
}

TEST_F(EditorLayerTest, NewSceneShortcutDoesNotInterruptTextEditing) {
    add_entity();
    const auto height     = hierarchy_height();
    const auto* inspector = ImGui::FindWindowByName("Inspector");
    ASSERT_NE(inspector, nullptr);
    click({ inspector->DC.CursorStartPos.x + 30.0f, inspector->DC.CursorStartPos.y + ImGui::GetFrameHeight() * 0.5f });
    ASSERT_TRUE(ImGui::GetIO().WantTextInput);
    new_scene_shortcut();
    EXPECT_FLOAT_EQ(hierarchy_height(), height);
    EXPECT_TRUE(ImGui::GetIO().WantTextInput);
}

TEST_F(EditorLayerTest, NewSceneShortcutDoesNotRunWhileProjectDialogIsOpen) {
    add_entity();
    const auto height = hierarchy_height();
    click({ 15.0f, ImGui::GetFontSize() * 0.5f });
    frame();
    ASSERT_EQ(ImGui::GetCurrentContext()->OpenPopupStack.Size, 1);
    const auto* menu = ImGui::GetCurrentContext()->OpenPopupStack[0].Window;
    ASSERT_NE(menu, nullptr);
    click({ menu->DC.CursorStartPos.x + 30.0f, menu->DC.CursorStartPos.y + ImGui::GetFontSize() * 0.5f });
    frame();
    const auto* dialog = ImGui::FindWindowByName("New Project");
    ASSERT_NE(dialog, nullptr);
    ASSERT_TRUE(dialog->Active);
    new_scene_shortcut();
    EXPECT_FLOAT_EQ(hierarchy_height(), height);
    EXPECT_TRUE(dialog->Active);
}

} // namespace
