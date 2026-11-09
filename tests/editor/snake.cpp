#include "editor/project/scene/scene_serializer.h"
#include "scene/components.h"
#include "scene/scene.h"
#include "script/script_runtime.h"

#include <filesystem>
#include <gtest/gtest.h>
#include <string>

namespace {

vulcal::scene::Entity find_by_name(vulcal::scene::Scene& scene, const std::string& name) {
    for (const auto handle: scene.registry().view<const vulcal::scene::TagComponent>()) {
        const auto& tag = scene.registry().get<const vulcal::scene::TagComponent>(handle);
        if (tag.name_ == name) return vulcal::scene::Entity{ handle, scene };
    }
    return {};
}

} // namespace

TEST(SnakeProject, SceneAndScriptCreateBoardAndGameOver) {
    const auto project = std::filesystem::path{ VULCAL_TEST_SOURCE_DIR } / "projects" / "snake";
    auto scene         = editor::SceneSerializer::deserialize(project / "assets" / "scenes" / "snake.vscene");
    ASSERT_TRUE(scene);

    vulcal::script::ScriptRuntime runtime;
    runtime.start(*scene, project);
    EXPECT_TRUE(runtime.running());

    EXPECT_TRUE(find_by_name(*scene, "Board"));
    EXPECT_TRUE(find_by_name(*scene, "WallTop"));
    EXPECT_TRUE(find_by_name(*scene, "SnakeHead"));
    EXPECT_TRUE(find_by_name(*scene, "Food"));

    // No input means the snake keeps moving right until it hits the wall.
    for (int i = 0; i < 6; ++i) runtime.update(0.5f);

    const auto environment_view = scene->registry().view<const vulcal::scene::EnvironmentComponent>();
    ASSERT_FALSE(environment_view.empty());
    const auto& environment = environment_view.get<const vulcal::scene::EnvironmentComponent>(environment_view.front());
    EXPECT_GT(environment.clear_color_.r, environment.clear_color_.b);

    const auto head = find_by_name(*scene, "SnakeHead");
    ASSERT_TRUE(head);
    EXPECT_GT(head.get_component<vulcal::scene::SpriteRendererComponent>().color_.r,
              head.get_component<vulcal::scene::SpriteRendererComponent>().color_.g);

    runtime.stop();
}
