#include "editor/editor_camera.h"
#include "editor/editor_context.h"

#include <chrono>
#include <cmath>
#include <filesystem>
#include <gtest/gtest.h>
#include <memory>
#include <stdexcept>
#include <string>

namespace {

std::filesystem::path temporary_directory() {
    const auto suffix = std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());
    return std::filesystem::temp_directory_path() / ("vulcal_editor_test_" + suffix);
}

TEST(EditorContext, StartsWithoutSceneAndRejectsEntityCreation) {
    editor::EditorContext context;
    EXPECT_FALSE(context.has_scene());
    EXPECT_FALSE(context.selected_entity());
    EXPECT_THROW(context.create_entity(), std::logic_error);
}

TEST(EditorContext, SceneReplacementClearsSelectionBeforeOldSceneIsDestroyed) {
    editor::EditorContext context;
    context.set_scene(std::make_unique<vulcal::scene::Scene>());
    context.create_entity();
    auto replacement = std::make_unique<vulcal::scene::Scene>();
    replacement->create_entity("Replacement");
    context.set_scene(std::move(replacement));
    EXPECT_FALSE(context.selected_entity());
    EXPECT_EQ(context.scene().registry().view<vulcal::scene::TagComponent>().size(), 1u);
}

TEST(EditorContext, RejectsSelectionFromAnotherScene) {
    editor::EditorContext context;
    vulcal::scene::Scene other;
    EXPECT_THROW(context.select(other.create_entity()), std::invalid_argument);
    EXPECT_FALSE(context.selected_entity());
}

TEST(EditorContext, DeletingSelectedEntityClearsSelection) {
    editor::EditorContext context;
    context.set_scene(std::make_unique<vulcal::scene::Scene>());
    auto selected = context.create_entity();
    context.destroy_entity(selected);
    EXPECT_FALSE(selected);
    EXPECT_FALSE(context.selected_entity());
}

TEST(EditorContext, DeletingOtherEntityPreservesSelection) {
    editor::EditorContext context;
    context.set_scene(std::make_unique<vulcal::scene::Scene>());
    auto other = context.create_entity();
    auto selected = context.create_entity();
    context.destroy_entity(other);
    EXPECT_EQ(context.selected_entity(), selected);
}

TEST(EditorContext, ExternalDeletionDoesNotSelectRecycledHandle) {
    editor::EditorContext context;
    context.set_scene(std::make_unique<vulcal::scene::Scene>());
    auto selected = context.create_entity();
    context.scene().destroy_entity(selected);
    context.scene().create_entity("New entity");
    EXPECT_FALSE(context.selected_entity());
}

TEST(EditorContext, NullSceneReplacementPreservesExistingSceneAndSelection) {
    editor::EditorContext context;
    context.set_scene(std::make_unique<vulcal::scene::Scene>());
    auto selected = context.create_entity();
    EXPECT_THROW(context.set_scene(nullptr), std::invalid_argument);
    EXPECT_EQ(context.selected_entity(), selected);
}

TEST(EditorContext, NewSceneCreatesDefaultSceneAndClearsSelection) {
    editor::EditorContext context;
    context.set_scene(std::make_unique<vulcal::scene::Scene>());
    context.create_entity();
    context.new_scene();
    EXPECT_FALSE(context.selected_entity());
    EXPECT_EQ(context.scene().registry().view<vulcal::scene::TagComponent>().size(), 3u);
}

TEST(EditorContext, SaveWithoutPathRequestsSaveAs) {
    editor::EditorContext context;
    context.new_scene();
    EXPECT_EQ(context.save_scene(), editor::EditorContext::SaveResult::NeedsPath);
}

TEST(EditorContext, SaveAsAddsSceneExtensionAndRemembersPath) {
    const auto directory = temporary_directory();
    std::filesystem::create_directories(directory);

    editor::EditorContext context;
    context.new_scene();
    context.save_scene_as(directory / "level");

    EXPECT_TRUE(std::filesystem::exists(directory / "level.vscene"));
    EXPECT_EQ(context.scene_dialog_directory(), directory);
    EXPECT_EQ(context.suggested_scene_name(), std::filesystem::path{ "level.vscene" });
    EXPECT_EQ(context.save_scene(), editor::EditorContext::SaveResult::Saved);

    std::filesystem::remove_all(directory);
}

TEST(EditorContext, CreateProjectStartsDefaultScene) {
    const auto directory = temporary_directory();
    std::filesystem::create_directories(directory);

    editor::EditorContext context;
    context.create_project(directory, "Demo");

    EXPECT_TRUE(std::filesystem::exists(directory / "Demo.vproj"));
    EXPECT_EQ(context.scene().registry().view<vulcal::scene::TagComponent>().size(), 3u);

    std::filesystem::remove_all(directory);
}

TEST(EditorContext, OpenProjectStartsDefaultScene) {
    const auto directory = temporary_directory();
    std::filesystem::create_directories(directory);
    {
        editor::EditorContext creator;
        creator.create_project(directory, "Demo");
    }

    editor::EditorContext context;
    context.open_project(directory / "Demo.vproj");

    EXPECT_EQ(context.scene().registry().view<vulcal::scene::TagComponent>().size(), 3u);

    std::filesystem::remove_all(directory);
}

TEST(EditorCamera, MovementIsIndependentOfFrameRate) {
    editor::EditorCamera whole;
    editor::EditorCamera split;
    const editor::EditorCamera::Input input{ .movement = { 0.0f, 0.0f, 1.0f } };
    whole.update(input, 1.0f);
    for (int i = 0; i < 100; ++i) split.update(input, 0.01f);
    EXPECT_LT(glm::length(whole.data(1.0f).position_ - split.data(1.0f).position_), 1e-3f);
}

TEST(EditorCamera, DiagonalMovementHasTheSameSpeedAsForwardMovement) {
    editor::EditorCamera forward;
    editor::EditorCamera diagonal;
    const auto origin = forward.data(1.0f).position_;
    forward.update({ .movement = { 0.0f, 0.0f, 1.0f } }, 1.0f);
    diagonal.update({ .movement = { 1.0f, 1.0f, 1.0f } }, 1.0f);
    EXPECT_NEAR(glm::length(forward.data(1.0f).position_ - origin), forward.speed(), 1e-4f);
    EXPECT_NEAR(glm::length(diagonal.data(1.0f).position_ - origin), forward.speed(), 1e-4f);
}

TEST(EditorCamera, VerticalMovementUsesWorldUp) {
    editor::EditorCamera camera;
    const auto origin = camera.data(1.0f).position_;
    camera.update({ .movement = { 0.0f, 1.0f, 0.0f } }, 1.0f);
    const auto delta = camera.data(1.0f).position_ - origin;
    EXPECT_NEAR(delta.x, 0.0f, 1e-4f);
    EXPECT_NEAR(delta.z, 0.0f, 1e-4f);
    EXPECT_NEAR(delta.y, camera.speed(), 1e-4f);
}

TEST(EditorCamera, WheelChangesSpeedWithinFinitePositiveBounds) {
    editor::EditorCamera camera;
    const float initial = camera.speed();
    camera.update({ .wheel = 1.0f }, 0.0f);
    EXPECT_GT(camera.speed(), initial);
    camera.update({ .wheel = -1.0f }, 0.0f);
    EXPECT_NEAR(camera.speed(), initial, 1e-4f);
    for (int i = 0; i < 100; ++i) camera.update({ .wheel = -100.0f }, 0.0f);
    EXPECT_TRUE(std::isfinite(camera.speed()));
    EXPECT_GT(camera.speed(), 0.0f);
    for (int i = 0; i < 100; ++i) camera.update({ .wheel = 100.0f }, 0.0f);
    EXPECT_TRUE(std::isfinite(camera.speed()));
    EXPECT_LT(camera.speed(), 10000.0f);
}

TEST(EditorCamera, ExtremePitchRemainsUprightAndFinite) {
    editor::EditorCamera camera;
    camera.update({ .look_delta = { 0.0f, -100000.0f } }, 0.0f);
    const auto view = camera.data(1.0f).view_;
    for (int col = 0; col < 4; ++col)
        for (int row = 0; row < 4; ++row) EXPECT_TRUE(std::isfinite(view[col][row]));
    EXPECT_GT(view[1][1], 0.0f);
}

TEST(EditorCamera, ProjectionUsesViewportAspect) {
    editor::EditorCamera camera;
    const auto projection = camera.data(2.0f).projection_;
    EXPECT_NEAR(projection[1][1] / projection[0][0], 2.0f, 1e-4f);
    EXPECT_THROW(camera.data(0.0f), std::invalid_argument);
}

} // namespace
