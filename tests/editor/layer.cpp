#include "editor/editor_layer.h"
#include "engine/application.h"
#include "script/script_component.h"

#include <SDL3/SDL.h>
#include <gtest/gtest.h>
#include <imgui.h>
#include <imgui_internal.h>
#include <memory>
#include <fstream>
#include <sstream>
#include <spdlog/sinks/ostream_sink.h>
#include <spdlog/spdlog.h>

namespace {

class RuntimeSceneObserver {
public:
    RuntimeSceneObserver() {
        sink_ = std::make_shared<spdlog::sinks::ostream_sink_mt>(output_);
        spdlog::default_logger()->sinks().push_back(sink_);
    }
    ~RuntimeSceneObserver() { std::erase(spdlog::default_logger()->sinks(), sink_); }
    std::string output() const { return output_.str(); }
private:
    std::ostringstream output_;
    std::shared_ptr<spdlog::sinks::ostream_sink_mt> sink_;
};

class ScriptProject {
public:
    ScriptProject() {
        directory = std::filesystem::temp_directory_path() / ("vulcal_play_" + base::UUID::generate().to_string());
        std::filesystem::create_directories(directory);
    }
    ~ScriptProject() { std::filesystem::remove_all(directory); }
    vulcal::scene::Entity prepare(editor::EditorLayer& layer) {
        layer.context().create_project(directory, "Play");
        layer.context().set_scene(std::make_unique<vulcal::scene::Scene>());
        const auto entity = layer.context().create_entity();
        entity.add_component<vulcal::script::ScriptComponent>().script_path_ = "assets/lifecycle.lua";
        std::ofstream script(directory / "assets/lifecycle.lua");
        script << "function on_create() entity:set_translation(10, 0, 0) end\n"
                  "function on_update(dt) local x,y,z = entity:get_translation(); entity:set_translation(x + dt, y, z) end\n"
                  "function on_destroy() entity:set_translation(999, 0, 0); Log.info('Destroyed ' .. entity.name .. ' at ' .. entity:get_translation()) end\n";
        return entity;
    }
    std::filesystem::path directory;
};

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

TEST_F(EditorLayerTest, PlayDrivesLuaAndStopDestroysScriptsBeforeDiscardingScene) {
    ScriptProject project;
    const auto edit = project.prepare(*layer_);
    layer_->play();
    ASSERT_TRUE(layer_->context().playing());
    const auto runtime = layer_->context().selected_entity();
    EXPECT_FLOAT_EQ(runtime.get_component<vulcal::scene::TransformComponent>().translation_.x, 10.0f);
    RuntimeSceneObserver observer;
    frame();
    EXPECT_NEAR(runtime.get_component<vulcal::scene::TransformComponent>().translation_.x, 10.0f + 1.0f / 60.0f, 1e-5f);
    new_scene_shortcut();
    EXPECT_TRUE(layer_->context().playing());
    layer_->stop();
    EXPECT_FALSE(layer_->context().playing());
    EXPECT_NE(observer.output().find("Destroyed Entity at 999"), std::string::npos);
    EXPECT_EQ(layer_->context().selected_entity(), edit);
    EXPECT_FLOAT_EQ(edit.get_component<vulcal::scene::TransformComponent>().translation_.x, 0.0f);
    const auto output = observer.output();
    layer_->stop();
    EXPECT_EQ(observer.output(), output);
}

TEST_F(EditorLayerTest, DetachStopsPlayBeforeRuntimeSceneDisposal) {
    ScriptProject project;
    project.prepare(*layer_);
    layer_->play();
    RuntimeSceneObserver observer;
    layer_->on_detach();
    EXPECT_FALSE(layer_->context().playing());
    EXPECT_NE(observer.output().find("Destroyed Entity at 999"), std::string::npos);
}

TEST(EditorPlayLifecycle, LayerDestructionStopsScriptsWhileRuntimeEntitiesAreAlive) {
    ScriptProject project;
    RuntimeSceneObserver observer;
    {
        editor::EditorLayer layer;
        project.prepare(layer);
        layer.play();
    }
    EXPECT_NE(observer.output().find("Destroyed Entity at 999"), std::string::npos);
}

TEST_F(EditorLayerTest, SceneMenuStartsAndStopsPlay) {
    click({ 60.0f, ImGui::GetFontSize() * 0.5f });
    frame();
    ASSERT_EQ(ImGui::GetCurrentContext()->OpenPopupStack.Size, 1);
    const auto* menu = ImGui::GetCurrentContext()->OpenPopupStack[0].Window;
    ASSERT_NE(menu, nullptr);
    click({ menu->DC.CursorStartPos.x + 30.0f, menu->DC.CursorStartPos.y + ImGui::GetFontSize() * 0.5f });
    EXPECT_TRUE(layer_->context().playing());
    click({ 60.0f, ImGui::GetFontSize() * 0.5f });
    frame();
    ASSERT_EQ(ImGui::GetCurrentContext()->OpenPopupStack.Size, 1);
    menu = ImGui::GetCurrentContext()->OpenPopupStack[0].Window;
    ASSERT_NE(menu, nullptr);
    click({ menu->DC.CursorStartPos.x + 30.0f,
            menu->DC.CursorStartPos.y + ImGui::GetFontSize() * 0.5f + ImGui::GetTextLineHeightWithSpacing() });
    EXPECT_FALSE(layer_->context().playing());
}

} // namespace
