#include "editor/editor_context.h"
#include "editor/panels/hierarchy_panel.h"
#include "editor/panels/inspector_panel.h"
#include "scene/components.h"

#include <gtest/gtest.h>
#include <imgui.h>
#include <imgui_internal.h>
#include <memory>

namespace {

class EditorPanels : public ::testing::Test {
protected:
    void SetUp() override {
        ImGui::CreateContext();
        auto& io = ImGui::GetIO();
        io.IniFilename = nullptr;
        io.DisplaySize = { 1000.0f, 700.0f };
        io.DeltaTime = 1.0f / 60.0f;
        io.ConfigInputTrickleEventQueue = false;
        unsigned char* pixels = nullptr;
        int width = 0, height = 0;
        io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);
        io.Fonts->SetTexID(ImTextureID{ 1 });
        context_.set_scene(std::make_unique<vulcal::scene::Scene>());
    }

    void TearDown() override { ImGui::DestroyContext(); }

    void frame() {
        ImGui::NewFrame();
        ImGui::SetNextWindowPos({ 0.0f, 0.0f });
        ImGui::SetNextWindowSize({ 300.0f, 650.0f });
        hierarchy_.draw(context_, open_);
        ImGui::SetNextWindowPos({ 320.0f, 0.0f });
        ImGui::SetNextWindowSize({ 650.0f, 650.0f });
        inspector_.draw(context_, open_);
        ImGui::Render();
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

    ImVec2 first_row() {
        const auto* window = ImGui::FindWindowByName("Hierarchy");
        return { window->DC.CursorStartPos.x + 50.0f, window->DC.CursorStartPos.y + ImGui::GetFontSize() * 0.5f };
    }

    void choose_popup_action(int index = 0) {
        frame();
        ASSERT_EQ(ImGui::GetCurrentContext()->OpenPopupStack.Size, 1);
        auto* popup = ImGui::GetCurrentContext()->OpenPopupStack[0].Window;
        ASSERT_NE(popup, nullptr);
        click({ popup->DC.CursorStartPos.x + 30.0f,
            popup->DC.CursorStartPos.y + ImGui::GetFontSize() * 0.5f + index * ImGui::GetTextLineHeightWithSpacing() });
    }

    void open_add_component() {
        auto* window = ImGui::FindWindowByName("Inspector");
        click({ window->DC.CursorStartPos.x + 40.0f, window->DC.CursorPosPrevLine.y + ImGui::GetFrameHeight() * 0.5f });
    }

    editor::EditorContext context_;
    editor::HierarchyPanel hierarchy_;
    editor::InspectorPanel inspector_;
    bool open_ = true;
};

TEST_F(EditorPanels, LeftClickSelectsEntityAndBlankClickClearsSelection) {
    auto entity = context_.scene().create_entity("Entity");
    frame();
    frame();
    click(first_row());
    EXPECT_EQ(context_.selected_entity(), entity);
    click({ 100.0f, 300.0f });
    EXPECT_FALSE(context_.selected_entity());
}

TEST_F(EditorPanels, BlankContextMenuCreatesAndSelectsEntity) {
    frame();
    frame();
    click({ 100.0f, 300.0f }, ImGuiMouseButton_Right);
    choose_popup_action();
    ASSERT_TRUE(context_.selected_entity());
    EXPECT_EQ(context_.scene().registry().view<vulcal::scene::TagComponent>().size(), 1u);
}

TEST_F(EditorPanels, EntityContextMenuDeletesEntityAndClearsInspectorSelection) {
    auto entity = context_.create_entity();
    frame();
    frame();
    click(first_row(), ImGuiMouseButton_Right);
    choose_popup_action();
    EXPECT_FALSE(entity);
    EXPECT_FALSE(context_.selected_entity());
}

TEST_F(EditorPanels, DuplicateNamesRemainIndependentlySelectable) {
    auto first = context_.scene().create_entity("Same##Name");
    auto second = context_.scene().create_entity("Same##Name");
    frame();
    frame();
    auto position = first_row();
    click(position);
    const auto selected = context_.selected_entity();
    ASSERT_TRUE(selected);
    position.y += ImGui::GetFrameHeightWithSpacing() - ImGui::GetStyle().FramePadding.y * 2.0f;
    click(position);
    EXPECT_TRUE(context_.selected_entity());
    EXPECT_NE(context_.selected_entity(), selected);
    EXPECT_TRUE(first && second);
}

TEST_F(EditorPanels, InspectorEditsNameThroughTextInput) {
    auto entity = context_.create_entity();
    frame();
    frame();
    auto* window = ImGui::FindWindowByName("Inspector");
    click({ window->DC.CursorStartPos.x + 40.0f, window->DC.CursorStartPos.y + ImGui::GetFrameHeight() * 0.5f });
    auto& io = ImGui::GetIO();
    io.AddKeyEvent(ImGuiMod_Ctrl, true);
    io.AddKeyEvent(ImGuiKey_A, true);
    frame();
    io.AddKeyEvent(ImGuiKey_A, false);
    io.AddKeyEvent(ImGuiMod_Ctrl, false);
    io.AddInputCharactersUTF8("Renamed entity");
    frame();
    EXPECT_EQ(entity.get_component<vulcal::scene::TagComponent>().name_, "Renamed entity");
}

TEST_F(EditorPanels, InspectorPreventsSingularScaleFromNumericInput) {
    auto entity = context_.create_entity();
    frame();
    frame();
    auto* window = ImGui::FindWindowByName("Inspector");
    const auto start = window->DC.CursorStartPos;
    // Name, UUID, Transform header, Position and Rotation precede the Scale row.
    const float y = start.y + ImGui::GetFrameHeightWithSpacing() * 4.0f + ImGui::GetTextLineHeightWithSpacing();
    auto& io = ImGui::GetIO();
    io.AddKeyEvent(ImGuiMod_Ctrl, true);
    click({ start.x + 30.0f, y + ImGui::GetFrameHeight() * 0.5f });
    io.AddKeyEvent(ImGuiKey_A, true);
    frame();
    io.AddKeyEvent(ImGuiKey_A, false);
    io.AddKeyEvent(ImGuiMod_Ctrl, false);
    io.AddInputCharactersUTF8("0");
    frame();
    io.AddKeyEvent(ImGuiKey_Enter, true);
    frame();
    const auto scale = entity.get_component<vulcal::scene::TransformComponent>().scale_;
    EXPECT_NEAR(scale.x, 0.0001f, 1e-6f);
    EXPECT_FLOAT_EQ(scale.y, 1.0f);
    EXPECT_FLOAT_EQ(scale.z, 1.0f);
}

TEST_F(EditorPanels, InspectorAddsAndRemovesOptionalComponentThroughMenus) {
    auto entity = context_.create_entity();
    frame();
    frame();
    open_add_component();
    choose_popup_action();
    ASSERT_TRUE(entity.has_component<vulcal::scene::MeshRendererComponent>());
    auto* window = ImGui::FindWindowByName("Inspector");
    const auto start = window->DC.CursorStartPos;
    const float y = start.y + ImGui::GetFrameHeightWithSpacing() * 5.0f + ImGui::GetTextLineHeightWithSpacing();
    click({ start.x + 40.0f, y + ImGui::GetFrameHeight() * 0.5f }, ImGuiMouseButton_Right);
    choose_popup_action();
    EXPECT_FALSE(entity.has_component<vulcal::scene::MeshRendererComponent>());
    EXPECT_TRUE(entity.has_component<vulcal::scene::TransformComponent>());
}

TEST_F(EditorPanels, InspectorAddsScriptComponentThroughMenu) {
    auto entity = context_.create_entity();
    frame();
    frame();
    open_add_component();
    choose_popup_action(4);
    EXPECT_TRUE(entity.has_component<vulcal::scene::ScriptComponent>());
}

TEST_F(EditorPanels, InspectorDisablesSecondSceneLight) {
    context_.scene().create_entity("Sun").add_component<vulcal::scene::LightComponent>();
    auto entity = context_.create_entity();
    frame();
    frame();
    open_add_component();
    choose_popup_action(2);
    EXPECT_FALSE(entity.has_component<vulcal::scene::LightComponent>());
    EXPECT_EQ(ImGui::GetCurrentContext()->OpenPopupStack.Size, 1);
}

TEST_F(EditorPanels, PlayingHierarchyAllowsSelectionButDisablesCreateAndDelete) {
    context_.create_entity();
    context_.begin_play();
    context_.select({});
    frame();
    frame();
    click(first_row());
    ASSERT_TRUE(context_.selected_entity());
    const auto selected = context_.selected_entity();
    click(first_row(), ImGuiMouseButton_Right);
    choose_popup_action();
    EXPECT_TRUE(context_.scene().contains(selected));
    EXPECT_EQ(context_.selected_entity(), selected);
    // Close the disabled menu and try the blank-window Create command.
    click({ 100.0f, 400.0f });
    click({ 100.0f, 300.0f }, ImGuiMouseButton_Right);
    choose_popup_action();
    EXPECT_EQ(context_.scene().registry().view<vulcal::scene::TagComponent>().size(), 1u);
}

TEST_F(EditorPanels, PlayingInspectorDisablesNameAndAddComponent) {
    context_.create_entity();
    context_.begin_play();
    const auto runtime = context_.selected_entity();
    frame();
    frame();
    const auto* window = ImGui::FindWindowByName("Inspector");
    click({ window->DC.CursorStartPos.x + 40.0f, window->DC.CursorStartPos.y + ImGui::GetFrameHeight() * 0.5f });
    ImGui::GetIO().AddInputCharactersUTF8("Unexpected edit");
    frame();
    EXPECT_EQ(runtime.get_component<vulcal::scene::TagComponent>().name_, "Entity");
    EXPECT_FALSE(ImGui::GetIO().WantTextInput);
    open_add_component();
    EXPECT_EQ(ImGui::GetCurrentContext()->OpenPopupStack.Size, 0);
    EXPECT_FALSE(runtime.has_component<vulcal::scene::ScriptComponent>());
}

TEST_F(EditorPanels, PlayingInspectorDisablesTransformAndComponentRemoval) {
    auto edit = context_.create_entity();
    edit.add_component<vulcal::scene::ScriptComponent>().script_path_ = "assets/test.lua";
    context_.begin_play();
    const auto runtime = context_.selected_entity();
    frame();
    frame();
    const auto* window = ImGui::FindWindowByName("Inspector");
    const auto start = window->DC.CursorStartPos;
    const float y = start.y + ImGui::GetFrameHeightWithSpacing() * 4.0f + ImGui::GetTextLineHeightWithSpacing();
    auto& io = ImGui::GetIO();
    io.AddKeyEvent(ImGuiMod_Ctrl, true);
    click({ start.x + 30.0f, y + ImGui::GetFrameHeight() * 0.5f });
    io.AddInputCharactersUTF8("25");
    io.AddKeyEvent(ImGuiMod_Ctrl, false);
    io.AddKeyEvent(ImGuiKey_Enter, true);
    frame();
    io.AddKeyEvent(ImGuiKey_Enter, false);
    frame();
    EXPECT_FLOAT_EQ(runtime.get_component<vulcal::scene::TransformComponent>().scale_.x, 1.0f);
    // The Script header follows Transform's three rows.
    const float header_y = start.y + ImGui::GetFrameHeightWithSpacing() * 5.0f + ImGui::GetTextLineHeightWithSpacing();
    click({ start.x + 40.0f, header_y + ImGui::GetFrameHeight() * 0.5f }, ImGuiMouseButton_Right);
    if (ImGui::GetCurrentContext()->OpenPopupStack.Size != 0) choose_popup_action();
    EXPECT_TRUE(runtime.has_component<vulcal::scene::ScriptComponent>());
    EXPECT_EQ(runtime.get_component<vulcal::scene::ScriptComponent>().script_path_, "assets/test.lua");
}

} // namespace
