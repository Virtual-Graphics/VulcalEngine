#include "editor/editor_context.h"
#include "scene/components.h"

#include <gtest/gtest.h>
#include <memory>
#include <stdexcept>

TEST(EditorPlay, ClonesSceneAndRoutesSelectionWithoutChangingEditData) {
    editor::EditorContext context;
    context.new_scene();
    auto edit = context.create_entity();
    edit.add_component<vulcal::scene::ScriptComponent>().script_path_ = "assets/move.lua";
    auto* edit_scene = &context.scene();
    const auto id = edit.get_component<vulcal::scene::IDComponent>().id_;
    context.begin_play();
    ASSERT_TRUE(context.playing());
    EXPECT_NE(&context.scene(), edit_scene);
    const editor::EditorContext& read_only = context;
    EXPECT_EQ(&read_only.scene(), &context.scene());
    const auto runtime = context.selected_entity();
    ASSERT_TRUE(runtime);
    EXPECT_NE(runtime, edit);
    EXPECT_EQ(runtime.get_component<vulcal::scene::IDComponent>().id_, id);
    EXPECT_EQ(runtime.get_component<vulcal::scene::ScriptComponent>().script_path_, "assets/move.lua");
    runtime.get_component<vulcal::scene::TransformComponent>().translation_.x = 42.0f;
    runtime.get_component<vulcal::scene::ScriptComponent>().script_path_ = "assets/other.lua";
    EXPECT_THROW(context.select(edit), std::invalid_argument);
    context.end_play();
    EXPECT_FALSE(context.playing());
    EXPECT_EQ(&context.scene(), edit_scene);
    EXPECT_EQ(context.selected_entity(), edit);
    EXPECT_FLOAT_EQ(edit.get_component<vulcal::scene::TransformComponent>().translation_.x, 0.0f);
    EXPECT_EQ(edit.get_component<vulcal::scene::ScriptComponent>().script_path_, "assets/move.lua");
}

TEST(EditorPlay, ClonesSpriteRendererComponent) {
    editor::EditorContext context;
    context.new_scene();
    auto edit    = context.create_entity();
    auto& sprite = edit.add_component<vulcal::scene::SpriteRendererComponent>();
    sprite.texture_asset_id_ = base::UUID{ 123 };
    sprite.sorting_order_    = 7;
    const auto id            = edit.get_component<vulcal::scene::IDComponent>().id_;

    context.begin_play();
    vulcal::scene::Entity runtime;
    context.scene().each<vulcal::scene::IDComponent>([&](auto entity, const auto& component) {
        if (component.id_ == id) runtime = entity;
    });
    ASSERT_TRUE(runtime.valid());
    ASSERT_TRUE(runtime.has_component<vulcal::scene::SpriteRendererComponent>());
    EXPECT_EQ(runtime.get_component<vulcal::scene::SpriteRendererComponent>().texture_asset_id_, base::UUID{ 123 });
    EXPECT_EQ(runtime.get_component<vulcal::scene::SpriteRendererComponent>().sorting_order_, 7);
    context.end_play();
}

TEST(EditorPlay, RestoresCurrentRuntimeSelectionByUuidRatherThanEntityHandle) {
    editor::EditorContext context;
    context.new_scene();
    auto first = context.create_entity();
    auto second = context.create_entity();
    context.select(first);
    context.begin_play();
    vulcal::scene::Entity runtime_second;
    context.scene().each<vulcal::scene::IDComponent>([&](auto entity, const auto& id) {
        if (id.id_ == second.get_component<vulcal::scene::IDComponent>().id_) runtime_second = entity;
    });
    // Recreate the UUID at a different handle so a handle-based rebind cannot pass.
    const auto second_id = runtime_second.get_component<vulcal::scene::IDComponent>().id_;
    context.scene().destroy_entity(runtime_second);
    runtime_second = context.scene().create_entity("Recreated");
    runtime_second.get_component<vulcal::scene::IDComponent>().id_ = second_id;
    ASSERT_NE(runtime_second.handle(), second.handle());
    context.select(runtime_second);
    context.end_play();
    EXPECT_EQ(context.selected_entity(), second);
    context.begin_play();
    context.select(context.scene().create_entity("Runtime only"));
    context.end_play();
    EXPECT_FALSE(context.selected_entity());
    context.begin_play();
    EXPECT_FALSE(context.selected_entity());
    context.end_play();
}

TEST(EditorPlay, DeletedRuntimeSelectionClearsOnStopAndTransitionsAreIdempotent) {
    editor::EditorContext context;
    EXPECT_THROW(context.begin_play(), std::logic_error);
    context.new_scene();
    auto edit = context.create_entity();
    context.begin_play();
    auto* runtime = &context.scene();
    context.begin_play();
    EXPECT_EQ(&context.scene(), runtime);
    context.scene().destroy_entity(context.selected_entity());
    context.end_play();
    context.end_play();
    EXPECT_FALSE(context.selected_entity());
    EXPECT_TRUE(edit);
}

TEST(EditorPlay, UsesPrimaryCameraOnlyWhilePlayingAndOnlyWhenPresent) {
    editor::EditorContext context;
    context.new_scene();
    auto camera_entity = context.scene().create_entity("Camera");
    auto& camera       = camera_entity.add_component<vulcal::scene::CameraComponent>();
    camera.primary_    = false;

    EXPECT_FALSE(context.uses_primary_camera());
    camera.primary_ = true;
    EXPECT_FALSE(context.uses_primary_camera());

    context.begin_play();
    EXPECT_TRUE(context.uses_primary_camera());
    context.scene().each<vulcal::scene::CameraComponent>([](auto, auto& runtime_camera) { runtime_camera.primary_ = false; });
    EXPECT_FALSE(context.uses_primary_camera());
    context.end_play();
}

TEST(EditorPlay, RejectsDocumentProjectAndStructuralEditsWhilePlaying) {
    editor::EditorContext context;
    context.new_scene();
    context.create_entity();
    context.begin_play();
    auto* runtime = &context.scene();
    auto selected = context.selected_entity();
    EXPECT_THROW(context.save_scene(), std::logic_error);
    EXPECT_THROW(context.save_scene_as("blocked.vscene"), std::logic_error);
    EXPECT_THROW(context.new_scene(), std::logic_error);
    EXPECT_THROW(context.open_scene("missing.vscene"), std::logic_error);
    EXPECT_THROW(context.set_scene(std::make_unique<vulcal::scene::Scene>()), std::logic_error);
    EXPECT_THROW(context.create_project("missing", "Blocked"), std::logic_error);
    EXPECT_THROW(context.open_project("missing.vproj"), std::logic_error);
    EXPECT_THROW(context.create_entity(), std::logic_error);
    EXPECT_THROW(context.destroy_entity(selected), std::logic_error);
    EXPECT_EQ(&context.scene(), runtime);
    EXPECT_EQ(context.selected_entity(), selected);
    EXPECT_FALSE(context.has_project());
    context.end_play();
}
