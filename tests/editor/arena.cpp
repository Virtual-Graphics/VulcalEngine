#include "editor/project/project.h"
#include "editor/project/scene/scene_serializer.h"
#include "scene/components.h"
#include "scene/scene.h"
#include "script/script_runtime.h"

#include <filesystem>
#include <gtest/gtest.h>
#include <sol/sol.hpp>
#include <string>

namespace {

const auto project_root = std::filesystem::path{ VULCAL_TEST_SOURCE_DIR } / "projects" / "arena";

vulcal::scene::Entity find_by_name(vulcal::scene::Scene& scene, const std::string& name) {
    for (const auto handle: scene.registry().view<const vulcal::scene::TagComponent>()) {
        if (scene.registry().get<const vulcal::scene::TagComponent>(handle).name_ == name) {
            return vulcal::scene::Entity{ handle, scene };
        }
    }
    return {};
}

TEST(ArenaProject, RealBindingsCreateUpdateStopAndRestartWithoutLeaking) {
    editor::ProjectInfo info;
    ASSERT_TRUE(editor::load_project_info(project_root / "arena.vproj", info));
    EXPECT_EQ(info.name, "arena");
    EXPECT_EQ(info.assets_path, "assets");
    auto scene = editor::SceneSerializer::deserialize(project_root / info.last_edited_scene);
    ASSERT_TRUE(scene);
    const auto initial_count = scene->registry().view<vulcal::scene::TagComponent>().size();
    EXPECT_EQ(initial_count, 3u);

    vulcal::script::ScriptRuntime runtime;
    std::size_t stage_count = 0;
    for (int run = 0; run < 2; ++run) {
        runtime.start(*scene, project_root);
        ASSERT_TRUE(runtime.running());
        ASSERT_TRUE(find_by_name(*scene, "Board"));
        ASSERT_TRUE(find_by_name(*scene, "WallTop"));
        ASSERT_TRUE(find_by_name(*scene, "Player"));
        ASSERT_TRUE(find_by_name(*scene, "TimeBar"));
        ASSERT_TRUE(find_by_name(*scene, "PromptPanel"));
        const auto count = scene->registry().view<vulcal::scene::TagComponent>().size();
        if (run == 0) stage_count = count;
        EXPECT_EQ(count, stage_count);
        for (int frame = 0; frame < 120; ++frame) runtime.update(1.0f / 60.0f);
        EXPECT_FALSE(find_by_name(*scene, "Enemy"));
        const auto player = find_by_name(*scene, "Player");
        ASSERT_TRUE(player);
        EXPECT_EQ(player.get_component<vulcal::scene::TransformComponent>().translation_, glm::vec3(0.0f));
        EXPECT_TRUE(find_by_name(*scene, "PromptPanel").get_component<vulcal::scene::SpriteRendererComponent>().visible_);
        runtime.stop();
        EXPECT_EQ(scene->registry().view<vulcal::scene::TagComponent>().size(), initial_count);
        EXPECT_EQ(find_by_name(*scene, "Camera").get_component<vulcal::scene::TransformComponent>().translation_,
                  glm::vec3(0.0f, 0.0f, 10.0f));
    }
}

class ArenaLogic : public testing::TestWithParam<const char*> {
protected:
    void SetUp() override {
        // Debug is test-only: it inspects local state without adding production hooks.
        lua_.open_libraries(sol::lib::base, sol::lib::math, sol::lib::string, sol::lib::table, sol::lib::debug);
        const auto harness = std::filesystem::path{ VULCAL_TEST_SOURCE_DIR } / "tests" / "editor" / "arena_test.lua";
        const auto result  = lua_.safe_script_file(harness.string(), sol::script_pass_on_error);
        ASSERT_TRUE(result.valid()) << result.get<sol::error>().what();
        const sol::protected_function load = lua_["load_arena"];
        const auto loaded                  = load((project_root / "assets" / "scripts" / "arena.lua").string());
        ASSERT_TRUE(loaded.valid()) << loaded.get<sol::error>().what();
    }

    sol::state lua_;
};

TEST_P(ArenaLogic, Gameplay) {
    const sol::protected_function test = lua_["arena_tests"][GetParam()];
    const auto result                  = test();
    ASSERT_TRUE(result.valid()) << result.get<sol::error>().what();
}

INSTANTIATE_TEST_SUITE_P(Arena, ArenaLogic,
                         testing::Values("waiting_and_cleanup", "movement_and_boundaries", "frame_rate_and_bounded_catchup",
                                         "aim_priority_and_fire_cadence", "substep_input_is_not_lost_or_repeated",
                                         "dash_duration_cooldown_and_bounds", "hurt_invulnerability_and_loss", "dash_invulnerability",
                                         "swept_hits_nearest_enemy_and_kills_once", "bullet_expiry_and_misses",
                                         "spawning_difficulty_and_caps", "victory_and_damage_precedence",
                                         "restarts_clear_dynamic_entities_and_effects", "particle_budget_and_terminal_freeze"),
                         [](const testing::TestParamInfo<const char*>& info) { return info.param; });

} // namespace
