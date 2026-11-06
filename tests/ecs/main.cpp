#include "base/uuid.h"
#include "scene/components.h"
#include "scene/scene.h"
#include "scene/scene_extractor.h"
#include "script/script_component.h"
#include "script/script_runtime.h"

#include <cmath>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <gtest/gtest.h>
#include <stdexcept>
#include <string>
#include <unordered_map>

namespace {

void expect_near(const glm::vec3& actual, const glm::vec3& expected, float epsilon = 1e-4f) {
    EXPECT_NEAR(actual.x, expected.x, epsilon);
    EXPECT_NEAR(actual.y, expected.y, epsilon);
    EXPECT_NEAR(actual.z, expected.z, epsilon);
}

vulcal::scene::Entity find_entity_by_id(vulcal::scene::Scene& scene, const base::UUID& id) {
    for (const auto entity: scene.registry().view<vulcal::scene::IDComponent>()) {
        if (scene.registry().get<vulcal::scene::IDComponent>(entity).id_ == id) {
            return vulcal::scene::Entity{ entity, scene };
        }
    }

    return {};
}

TEST(Uuid, DefaultIsNil) {
    const base::UUID nil;
    EXPECT_TRUE(nil.is_nil());
    EXPECT_EQ(nil.value(), 0ull);
}

TEST(Uuid, GeneratedUuidsAreUnique) {
    const auto first  = base::UUID::generate();
    const auto second = base::UUID::generate();
    EXPECT_FALSE(first.is_nil());
    EXPECT_NE(first, second);
}

TEST(Uuid, ToStringIsDecimal) {
    EXPECT_EQ(base::UUID{}.to_string(), std::string{ "0" });
    EXPECT_EQ(base::UUID{ 1ull }.to_string(), std::string{ "1" });
    EXPECT_EQ(base::UUID{ 4660ull }.to_string(), std::string{ "4660" });
    EXPECT_EQ(base::UUID{ 18446744073709551615ull }.to_string(), std::string{ "18446744073709551615" });
}

TEST(Uuid, UsableAsUnorderedMapKey) {
    const auto first  = base::UUID::generate();
    const auto second = base::UUID::generate();

    std::unordered_map<base::UUID, int> values;
    values.emplace(first, 1);
    values.emplace(second, 2);
    EXPECT_EQ(values.size(), static_cast<std::size_t>(2));
}

TEST(Scene, CreatesEntityWithDefaultComponents) {
    vulcal::scene::Scene scene;
    auto entity = scene.create_entity("Player");

    EXPECT_TRUE(static_cast<bool>(entity));
    EXPECT_TRUE(entity.has_component<vulcal::scene::IDComponent>());
    EXPECT_TRUE(entity.has_component<vulcal::scene::TagComponent>());
    EXPECT_TRUE(entity.has_component<vulcal::scene::TransformComponent>());
    EXPECT_EQ(entity.get_component<vulcal::scene::TagComponent>().name_, std::string{ "Player" });
}

TEST(Scene, AddGetAndRemoveComponents) {
    vulcal::scene::Scene scene;
    auto entity                      = scene.create_entity("Player");

    auto& mesh_renderer              = entity.add_component<vulcal::scene::MeshRendererComponent>();
    mesh_renderer.mesh_asset_id_     = base::UUID{ 7 };
    mesh_renderer.material_asset_id_ = base::UUID{ 9 };
    EXPECT_TRUE(entity.has_component<vulcal::scene::MeshRendererComponent>());
    EXPECT_EQ(entity.get_component<vulcal::scene::MeshRendererComponent>().mesh_asset_id_, base::UUID{ 7 });

    entity.remove_component<vulcal::scene::MeshRendererComponent>();
    EXPECT_FALSE(entity.has_component<vulcal::scene::MeshRendererComponent>());
}

TEST(Scene, EntityIdsAreUnique) {
    vulcal::scene::Scene scene;
    auto first  = scene.create_entity("Player");
    auto second = scene.create_entity("Enemy");

    EXPECT_NE(first.get_component<vulcal::scene::IDComponent>().id_, second.get_component<vulcal::scene::IDComponent>().id_);
}

TEST(Scene, SceneLevelComponentAccess) {
    vulcal::scene::Scene scene;
    auto entity = scene.create_entity("Player");

    EXPECT_TRUE(scene.has_component<vulcal::scene::IDComponent>(entity));

    scene.get_component<vulcal::scene::TagComponent>(entity).name_ = "Boss";
    EXPECT_EQ(scene.get_component<vulcal::scene::TagComponent>(entity).name_, std::string{ "Boss" });
}

TEST(Scene, RejectsForeignEntityEvenWhenItsHandleMatches) {
    vulcal::scene::Scene scene;
    vulcal::scene::Scene other;
    auto local   = scene.create_entity("Local");
    auto foreign = other.create_entity("Foreign");
    EXPECT_THROW(scene.destroy_entity(foreign), std::invalid_argument);
    EXPECT_TRUE(local.valid());
    EXPECT_TRUE(foreign.valid());
}

TEST(Scene, RequiredComponentsCannotBeRemovedThroughEitherApi) {
    vulcal::scene::Scene scene;
    auto entity = scene.create_entity();
    EXPECT_THROW(entity.remove_component<vulcal::scene::IDComponent>(), std::logic_error);
    EXPECT_THROW(scene.remove_component<vulcal::scene::TagComponent>(entity), std::logic_error);
    EXPECT_THROW(entity.remove_component<vulcal::scene::TransformComponent>(), std::logic_error);
    EXPECT_THROW(entity.remove_component<const vulcal::scene::TagComponent>(), std::logic_error);
    EXPECT_TRUE(entity.has_component<vulcal::scene::IDComponent>());
    EXPECT_TRUE(entity.has_component<vulcal::scene::TagComponent>());
    EXPECT_TRUE(entity.has_component<vulcal::scene::TransformComponent>());
}

TEST(Scene, WrapperTraversalFiltersComponentsAndReturnsOwnedEntities) {
    vulcal::scene::Scene scene;
    auto entity = scene.create_entity("Renderable");
    entity.add_component<vulcal::scene::MeshRendererComponent>();
    scene.create_entity("Empty");
    int visited = 0;
    scene.each<vulcal::scene::TagComponent, vulcal::scene::MeshRendererComponent>(
            [&](vulcal::scene::Entity current, auto& tag, auto& mesh) {
                EXPECT_EQ(current, entity);
                EXPECT_TRUE(scene.contains(current));
                tag.name_     = "Edited";
                mesh.visible_ = false;
                ++visited;
            });
    EXPECT_EQ(visited, 1);
    EXPECT_EQ(entity.get_component<vulcal::scene::TagComponent>().name_, "Edited");
    EXPECT_FALSE(entity.get_component<vulcal::scene::MeshRendererComponent>().visible_);
}

TEST(Scene, DuplicateOrInvalidComponentAccessFailsBeforeRegistryMutation) {
    vulcal::scene::Scene scene;
    auto entity = scene.create_entity();
    entity.add_component<vulcal::scene::CameraComponent>();
    EXPECT_THROW(entity.add_component<vulcal::scene::CameraComponent>(), std::logic_error);
    EXPECT_THROW(entity.get_component<vulcal::scene::LightComponent>(), std::out_of_range);
    scene.destroy_entity(entity);
    EXPECT_FALSE(scene.contains(entity));
    EXPECT_THROW(entity.add_component<vulcal::scene::LightComponent>(), std::invalid_argument);
}

TEST(Scene, DestroyEntityInvalidatesOnlyThatEntity) {
    vulcal::scene::Scene scene;
    auto entity = scene.create_entity("Player");
    auto second = scene.create_entity("Enemy");

    scene.destroy_entity(second);
    EXPECT_FALSE(second.valid());
    EXPECT_FALSE(static_cast<bool>(second));
    EXPECT_TRUE(entity.valid());
}

TEST(TransformAndCamera, TransformMatrixAppliesTranslationAndScale) {
    vulcal::scene::TransformComponent transform;
    transform.translation_ = glm::vec3{ 1.0f, 2.0f, 3.0f };
    transform.scale_       = glm::vec3{ 2.0f };

    const auto matrix      = transform.matrix();
    expect_near(glm::vec3{ matrix[3] }, glm::vec3{ 1.0f, 2.0f, 3.0f });
    expect_near(
            glm::vec3{
                matrix * glm::vec4{ 1.0f, 0.0f, 0.0f, 1.0f }
    },
            glm::vec3{ 3.0f, 2.0f, 3.0f });
}

TEST(TransformAndCamera, SetAspectUpdatesPerspectiveAspect) {
    vulcal::scene::CameraComponent camera;
    camera.perspective_aspect_ = 1.5f;

    camera.set_aspect(2.0f);
    EXPECT_FLOAT_EQ(camera.perspective_aspect_, 2.0f);
}

TEST(TransformAndCamera, TransformRoundTripsCameraOrientation) {
    const glm::mat4 camera_world =
            glm::inverse(glm::lookAt(glm::vec3{ 0.0f, 4.0f, 8.0f }, glm::vec3{ 0.0f, -0.5f, 0.0f }, glm::vec3{ 0.0f, 1.0f, 0.0f }));
    vulcal::scene::TransformComponent camera_transform;
    camera_transform.translation_  = glm::vec3{ camera_world[3] };
    camera_transform.rotation_     = glm::eulerAngles(glm::quat_cast(camera_world));

    const glm::mat3 expected_basis = glm::mat3{ camera_world };
    const glm::mat3 actual_basis   = glm::mat3{ camera_transform.matrix() };
    expect_near(glm::vec3{ actual_basis[0] }, glm::vec3{ expected_basis[0] });
    expect_near(glm::vec3{ actual_basis[1] }, glm::vec3{ expected_basis[1] });
    expect_near(glm::vec3{ actual_basis[2] }, glm::vec3{ expected_basis[2] });
}

TEST(SceneCopy, CopyPreservesEntitiesAndComponents) {
    vulcal::scene::Scene scene;
    auto source                                                            = scene.create_entity("Source");
    source.get_component<vulcal::scene::TransformComponent>().translation_ = glm::vec3{ 4.0f, 5.0f, 6.0f };

    auto& source_renderer                                                  = source.add_component<vulcal::scene::MeshRendererComponent>();
    source_renderer.mesh_asset_id_                                         = base::UUID{ 21 };
    source_renderer.material_asset_id_                                     = base::UUID{ 22 };

    auto& camera                                                           = source.add_component<vulcal::scene::CameraComponent>();
    camera.primary_                                                        = true;
    camera.perspective_fov_y_                                              = glm::radians(50.0f);

    auto environment_entity                                                = scene.create_entity("Environment");
    auto& environment          = environment_entity.add_component<vulcal::scene::EnvironmentComponent>();
    environment.ambient_color_ = glm::vec3{ 0.2f, 0.3f, 0.4f };
    environment.clear_color_   = glm::vec4{ 0.5f, 0.6f, 0.7f, 1.0f };

    auto light_entity          = scene.create_entity("Light");
    auto& light                = light_entity.add_component<vulcal::scene::LightComponent>();
    light_entity.get_component<vulcal::scene::TransformComponent>().rotation_ = { 0.0f, glm::radians(90.0f), 0.0f };
    light.color_                                                              = glm::vec3{ 1.0f };
    light.intensity_                                                          = 2.0f;
    light.shadow_enabled_                                                     = false;
    light.shadow_map_size_                                                    = 1024;
    light.shadow_extent_                                                      = 6.0f;

    const auto source_id                                                      = source.get_component<vulcal::scene::IDComponent>().id_;
    const auto source_translation = source.get_component<vulcal::scene::TransformComponent>().translation_;

    vulcal::scene::Scene copy     = scene;
    auto copied_source            = find_entity_by_id(copy, source_id);
    ASSERT_TRUE(copied_source.valid());
    EXPECT_EQ(copied_source.get_component<vulcal::scene::IDComponent>().id_, source_id);
    EXPECT_EQ(copied_source.get_component<vulcal::scene::TagComponent>().name_, std::string{ "Source" });
    expect_near(copied_source.get_component<vulcal::scene::TransformComponent>().translation_, source_translation);
    EXPECT_TRUE(copied_source.has_component<vulcal::scene::CameraComponent>());
    EXPECT_TRUE(copied_source.get_component<vulcal::scene::CameraComponent>().primary_);
    EXPECT_EQ(copied_source.get_component<vulcal::scene::MeshRendererComponent>().mesh_asset_id_, base::UUID{ 21 });

    const auto environment_view = copy.registry().view<const vulcal::scene::EnvironmentComponent>();
    ASSERT_FALSE(environment_view.empty());
    const auto& copied_environment = environment_view.get<const vulcal::scene::EnvironmentComponent>(environment_view.front());
    expect_near(copied_environment.ambient_color_, glm::vec3{ 0.2f, 0.3f, 0.4f });
    expect_near(glm::vec3{ copied_environment.clear_color_ }, glm::vec3{ 0.5f, 0.6f, 0.7f });

    const auto light_view = copy.registry().view<const vulcal::scene::LightComponent>();
    ASSERT_FALSE(light_view.empty());
    const auto& copied_light = light_view.get<const vulcal::scene::LightComponent>(light_view.front());
    EXPECT_EQ(copied_light.type_, vulcal::scene::LightType::Directional);
    expect_near(copy.registry().get<vulcal::scene::TransformComponent>(light_view.front()).rotation_, { 0.0f, glm::radians(90.0f), 0.0f });
    EXPECT_FLOAT_EQ(copied_light.intensity_, 2.0f);
    EXPECT_FALSE(copied_light.shadow_enabled_);
    EXPECT_EQ(copied_light.shadow_map_size_, 1024u);
}

TEST(SceneCopy, CopiesAreIndependent) {
    vulcal::scene::Scene scene;
    auto source                                                            = scene.create_entity("Source");
    source.get_component<vulcal::scene::TransformComponent>().translation_ = glm::vec3{ 4.0f, 5.0f, 6.0f };
    const auto source_id                                                   = source.get_component<vulcal::scene::IDComponent>().id_;

    vulcal::scene::Scene copy                                              = scene;
    auto copied_source                                                     = find_entity_by_id(copy, source_id);
    ASSERT_TRUE(copied_source.valid());

    copied_source.get_component<vulcal::scene::TransformComponent>().translation_.x = 100.0f;
    EXPECT_FLOAT_EQ(source.get_component<vulcal::scene::TransformComponent>().translation_.x, 4.0f);

    copy.destroy_entity(copied_source);
    EXPECT_FALSE(copied_source.valid());
    EXPECT_TRUE(source.valid());
}

TEST(SceneCopy, PreservesScriptComponent) {
    vulcal::scene::Scene scene;
    auto source                                                          = scene.create_entity("Scripted");
    source.add_component<vulcal::script::ScriptComponent>().script_path_ = "Scripts/player.lua";

    vulcal::scene::Scene copy                                            = scene;
    auto copied_source = find_entity_by_id(copy, source.get_component<vulcal::scene::IDComponent>().id_);

    ASSERT_TRUE(copied_source.valid());
    ASSERT_TRUE(copied_source.has_component<vulcal::script::ScriptComponent>());
    EXPECT_EQ(copied_source.get_component<vulcal::script::ScriptComponent>().script_path_, "Scripts/player.lua");
}

TEST(ScriptRuntime, LuaCallbacksUseBindingsAndIsolatedEnvironments) {
    const auto root   = std::filesystem::temp_directory_path() / ("vulcal_script_runtime_lua_test_" + base::UUID::generate().to_string());
    const auto assets = root / "Assets";
    std::filesystem::create_directories(assets);
    const auto script_path = assets / "counter.lua";
    {
        std::ofstream script(script_path);
        script << "shared = (shared or 0) + 1\n"
                  "function on_create()\n"
                  "  assert(entity.name == 'First' or entity.name == 'Second')\n"
                  "  entity:set_translation(shared, 2, 3)\n"
                  "  Log.info('created')\n"
                  "  assert(Input.is_key_down(Key.Space) == false)\n"
                  "  assert(Input.is_mouse_button_down(MouseButton.Left) == false)\n"
                  "end\n"
                  "function on_update(dt) local x, y, z = entity:get_translation(); entity:set_translation(x + dt, y, z) end\n"
                  "function on_destroy() local x, y, z = entity:get_translation(); entity:set_translation(x, y, z + 1) end\n";
    }

    vulcal::scene::Scene scene;
    auto first                                                           = scene.create_entity("First");
    auto second                                                          = scene.create_entity("Second");
    first.add_component<vulcal::script::ScriptComponent>().script_path_  = "Assets/counter.lua";
    second.add_component<vulcal::script::ScriptComponent>().script_path_ = "Assets/counter.lua";
    vulcal::script::ScriptRuntime runtime;

    runtime.start(scene, root, assets);
    EXPECT_TRUE(runtime.running());
    EXPECT_FLOAT_EQ(first.get_component<vulcal::scene::TransformComponent>().translation_.x, 1.0f);
    EXPECT_FLOAT_EQ(second.get_component<vulcal::scene::TransformComponent>().translation_.x, 1.0f);
    runtime.update(0.5f);
    EXPECT_FLOAT_EQ(first.get_component<vulcal::scene::TransformComponent>().translation_.x, 1.5f);
    EXPECT_FLOAT_EQ(second.get_component<vulcal::scene::TransformComponent>().translation_.x, 1.5f);
    runtime.stop();
    EXPECT_FLOAT_EQ(first.get_component<vulcal::scene::TransformComponent>().translation_.z, 4.0f);
    EXPECT_FALSE(runtime.running());
    std::filesystem::remove_all(root);
}

TEST(ScriptRuntime, LuaExposesRotationAndScaleGettersAndSetters) {
    const auto root =
            std::filesystem::temp_directory_path() / ("vulcal_script_runtime_transform_test_" + base::UUID::generate().to_string());
    const auto assets = root / "Assets";
    std::filesystem::create_directories(assets);
    std::ofstream(assets / "transform.lua")
            << "assert(type(coroutine.create) == 'function')\n"
               "function on_create() entity:set_rotation(1, 2, 3); entity:set_scale(4, 5, 6) end\n"
               "function on_update(dt) local x, y, z = entity:get_rotation(); local sx, sy, sz = entity:get_scale(); "
               "entity:set_rotation(x + dt, y, z); entity:set_scale(sx + dt, sy, sz) end\n";
    vulcal::scene::Scene scene;
    auto entity                                                          = scene.create_entity("TransformScript");
    entity.add_component<vulcal::script::ScriptComponent>().script_path_ = "Assets/transform.lua";
    vulcal::script::ScriptRuntime runtime;

    runtime.start(scene, root, assets);
    expect_near(entity.get_component<vulcal::scene::TransformComponent>().rotation_, { 1.0f, 2.0f, 3.0f });
    expect_near(entity.get_component<vulcal::scene::TransformComponent>().scale_, { 4.0f, 5.0f, 6.0f });
    runtime.update(0.25f);
    expect_near(entity.get_component<vulcal::scene::TransformComponent>().rotation_, { 1.25f, 2.0f, 3.0f });
    expect_near(entity.get_component<vulcal::scene::TransformComponent>().scale_, { 4.25f, 5.0f, 6.0f });
    runtime.stop();
    std::filesystem::remove_all(root);
}

TEST(ScriptRuntime, RejectsPathsOutsideAssetsAndUnsupportedExtensions) {
    const auto root   = std::filesystem::temp_directory_path() / ("vulcal_script_runtime_paths_test_" + base::UUID::generate().to_string());
    const auto assets = root / "Assets";
    std::filesystem::create_directories(assets);
    std::ofstream(assets / "valid.lua") << "function on_create() entity:set_translation(99, 0, 0) end\n";
    std::ofstream(root / "outside.lua") << "function on_create() entity:set_translation(99, 0, 0) end\n";
    std::ofstream(assets / "unsupported.txt") << "function on_create() entity:set_translation(99, 0, 0) end\n";
    vulcal::scene::Scene scene;
    auto outside                                                              = scene.create_entity("Outside");
    auto unsupported                                                          = scene.create_entity("Unsupported");
    auto absolute                                                             = scene.create_entity("Absolute");
    auto missing                                                              = scene.create_entity("Missing");
    outside.add_component<vulcal::script::ScriptComponent>().script_path_     = "outside.lua";
    unsupported.add_component<vulcal::script::ScriptComponent>().script_path_ = "Assets/unsupported.txt";
    absolute.add_component<vulcal::script::ScriptComponent>().script_path_    = (assets / "valid.lua").string();
    missing.add_component<vulcal::script::ScriptComponent>().script_path_     = "Assets/missing.lua";
    vulcal::script::ScriptRuntime runtime;

    EXPECT_NO_THROW(runtime.start(scene, root, assets));
    EXPECT_TRUE(runtime.running());
    runtime.stop();
    EXPECT_FALSE(runtime.running());
    expect_near(outside.get_component<vulcal::scene::TransformComponent>().translation_, { 0.0f, 0.0f, 0.0f });
    expect_near(unsupported.get_component<vulcal::scene::TransformComponent>().translation_, { 0.0f, 0.0f, 0.0f });
    expect_near(absolute.get_component<vulcal::scene::TransformComponent>().translation_, { 0.0f, 0.0f, 0.0f });
    expect_near(missing.get_component<vulcal::scene::TransformComponent>().translation_, { 0.0f, 0.0f, 0.0f });
    std::filesystem::remove_all(root);
}

TEST(ScriptRuntime, FakeBackendUsesLanguageNeutralRuntimeContractAndContainsFailures) {
    struct State {
        int creates  = 0;
        int updates  = 0;
        int destroys = 0;
    } state;
    class FakeInstance final : public vulcal::script::ScriptInstance {
    public:
        FakeInstance(State& state, bool fail)
                : state_(state),
                  fail_(fail) {
            ++state_.creates;
        }
        void on_create() override {}
        void on_update(float) override {
            ++state_.updates;
            if (fail_) throw std::runtime_error("script failure");
        }
        void on_destroy() override { ++state_.destroys; }

    private:
        State& state_;
        bool fail_;
    };
    class FakeBackend final : public vulcal::script::ScriptBackend {
    public:
        explicit FakeBackend(State& state)
                : state_(state) {}
        std::string_view extension() const override { return ".lua"; }
        std::unique_ptr<vulcal::script::ScriptInstance> create(vulcal::scene::Entity entity, const std::filesystem::path&) override {
            const auto fails = entity.get_component<vulcal::scene::TagComponent>().name_ == "Fails";
            return std::make_unique<FakeInstance>(state_, fails);
        }

    private:
        State& state_;
    };

    const auto root   = std::filesystem::temp_directory_path() / ("vulcal_script_runtime_fake_test_" + base::UUID::generate().to_string());
    const auto assets = root / "Assets";
    std::filesystem::create_directories(assets);
    std::ofstream(assets / "fake.lua") << "-- fake backend file\n";
    vulcal::scene::Scene scene;
    auto failing                                                          = scene.create_entity("Fails");
    auto healthy                                                          = scene.create_entity("Healthy");
    failing.add_component<vulcal::script::ScriptComponent>().script_path_ = "Assets/fake.lua";
    healthy.add_component<vulcal::script::ScriptComponent>().script_path_ = "Assets/fake.lua";
    vulcal::script::ScriptRuntime runtime;
    runtime.register_backend(std::make_unique<FakeBackend>(state));

    runtime.start(scene, root, assets);
    EXPECT_EQ(state.creates, 2);
    runtime.update(1.0f);
    EXPECT_EQ(state.updates, 2);
    scene.destroy_entity(failing);
    runtime.update(1.0f);
    EXPECT_EQ(state.updates, 3);
    EXPECT_EQ(state.destroys, 1);
    runtime.stop();
    EXPECT_EQ(state.destroys, 2);
    EXPECT_FALSE(runtime.running());
    std::filesystem::remove_all(root);
}

TEST(SceneCopy, CopyAssignmentPreservesTransforms) {
    vulcal::scene::Scene scene;
    auto source                                                            = scene.create_entity("Source");
    source.get_component<vulcal::scene::TransformComponent>().translation_ = glm::vec3{ 4.0f, 5.0f, 6.0f };
    const auto source_id                                                   = source.get_component<vulcal::scene::IDComponent>().id_;
    const auto source_translation = source.get_component<vulcal::scene::TransformComponent>().translation_;

    vulcal::scene::Scene assigned;
    assigned             = scene;

    auto assigned_source = find_entity_by_id(assigned, source_id);
    ASSERT_TRUE(assigned_source.valid());
    expect_near(assigned_source.get_component<vulcal::scene::TransformComponent>().translation_, source_translation);
}

class SceneExtractorTest : public ::testing::Test {
protected:
    void SetUp() override {
        camera_entity_                                                                 = scene_.create_entity("Camera");
        camera_entity_.get_component<vulcal::scene::TransformComponent>().translation_ = glm::vec3{ 0.0f, 0.0f, 5.0f };
        camera_                      = &camera_entity_.add_component<vulcal::scene::CameraComponent>();
        camera_->primary_            = true;
        camera_->perspective_fov_y_  = glm::radians(45.0f);
        camera_->perspective_aspect_ = 2.0f;
        camera_->perspective_near_   = 0.1f;
        camera_->perspective_far_    = 100.0f;

        auto environment_entity      = scene_.create_entity("Environment");
        auto& environment            = environment_entity.add_component<vulcal::scene::EnvironmentComponent>();
        environment.ambient_color_   = glm::vec3{ 0.1f, 0.2f, 0.3f };
        environment.clear_color_     = glm::vec4{ 0.4f, 0.5f, 0.6f, 1.0f };

        auto light_entity            = scene_.create_entity("Light");
        auto& light                  = light_entity.add_component<vulcal::scene::LightComponent>();
        light_entity.get_component<vulcal::scene::TransformComponent>().rotation_   = { 0.0f, glm::radians(90.0f), 0.0f };
        light.color_                                                                = glm::vec3{ 1.0f };
        light.intensity_                                                            = 3.0f;
        light.shadow_enabled_                                                       = true;
        light.shadow_map_size_                                                      = 1024;
        light.shadow_center_                                                        = glm::vec3{ 0.0f, -0.5f, 0.0f };
        light.shadow_extent_                                                        = 6.0f;
        light.shadow_near_                                                          = 0.2f;
        light.shadow_far_                                                           = 30.0f;
        light.shadow_depth_bias_                                                    = 0.002f;
        light.shadow_normal_bias_                                                   = 0.03f;

        auto mesh_entity                                                            = scene_.create_entity("Mesh");
        mesh_entity.get_component<vulcal::scene::TransformComponent>().translation_ = glm::vec3{ 1.0f, 2.0f, 3.0f };
        auto& mesh_renderer                = mesh_entity.add_component<vulcal::scene::MeshRendererComponent>();
        mesh_renderer.mesh_asset_id_       = base::UUID{ 11 };
        mesh_renderer.material_asset_id_   = base::UUID{ 22 };

        auto hidden_entity                 = scene_.create_entity("Hidden");
        auto& hidden_renderer              = hidden_entity.add_component<vulcal::scene::MeshRendererComponent>();
        hidden_renderer.mesh_asset_id_     = base::UUID{ 33 };
        hidden_renderer.material_asset_id_ = base::UUID{ 44 };
        hidden_renderer.visible_           = false;
    }

    vulcal::scene::Scene scene_;
    vulcal::scene::Entity camera_entity_;
    vulcal::scene::CameraComponent* camera_ = nullptr;
    vulcal::scene::SceneExtractor extractor_;
};

TEST_F(SceneExtractorTest, UsesPrimaryCamera) {
    const auto data = extractor_.extract(scene_);

    expect_near(data.camera_.position_, glm::vec3{ 0.0f, 0.0f, 5.0f });
    expect_near(glm::vec3{ data.camera_.view_[3] }, glm::vec3{ 0.0f, 0.0f, -5.0f });

    const float expected_projection_00 = 1.0f / (camera_->perspective_aspect_ * std::tan(camera_->perspective_fov_y_ * 0.5f));
    EXPECT_NEAR(data.camera_.projection_[0][0], expected_projection_00, 1e-4f);
}

TEST_F(SceneExtractorTest, ExtractsCameraAndLightsWithoutRenderResources) {
    const auto data = extractor_.extract(scene_);
    EXPECT_TRUE(data.items_.empty());
    EXPECT_TRUE(data.directional_light_);
    expect_near(data.camera_.position_, { 0.0f, 0.0f, 5.0f });
}

TEST_F(SceneExtractorTest, ExtractsEnvironment) {
    const auto data = extractor_.extract(scene_);

    expect_near(data.environment_.ambient_color_, glm::vec3{ 0.1f, 0.2f, 0.3f });
    expect_near(glm::vec3{ data.environment_.clear_color_ }, glm::vec3{ 0.4f, 0.5f, 0.6f });
}

TEST_F(SceneExtractorTest, ExtractsDirectionalLightAndShadow) {
    const auto data = extractor_.extract(scene_);

    ASSERT_TRUE(data.directional_light_.has_value());
    expect_near(glm::vec3{ data.directional_light_->direction_ }, glm::vec3{ -1.0f, 0.0f, 0.0f });
    expect_near(glm::vec3{ data.directional_light_->color_ }, glm::vec3{ 1.0f });
    EXPECT_FLOAT_EQ(data.directional_light_->intensity_, 3.0f);
    EXPECT_TRUE(data.directional_light_->shadow_.enabled_);
    EXPECT_EQ(data.directional_light_->shadow_.map_size_, 1024u);
    expect_near(glm::vec3{ data.directional_light_->shadow_.center_ }, glm::vec3{ 0.0f, -0.5f, 0.0f });
    EXPECT_FLOAT_EQ(data.directional_light_->shadow_.extent_, 6.0f);
    EXPECT_FLOAT_EQ(data.directional_light_->shadow_.near_, 0.2f);
    EXPECT_FLOAT_EQ(data.directional_light_->shadow_.far_, 30.0f);
    EXPECT_FLOAT_EQ(data.directional_light_->shadow_.depth_bias_, 0.002f);
    EXPECT_FLOAT_EQ(data.directional_light_->shadow_.normal_bias_, 0.03f);
}

TEST_F(SceneExtractorTest, BuildsOrthographicProjection) {
    camera_->projection_type_     = vulcal::scene::CameraComponent::ProjectionType::Orthographic;
    camera_->orthographic_left_   = -2.0f;
    camera_->orthographic_right_  = 2.0f;
    camera_->orthographic_bottom_ = -1.0f;
    camera_->orthographic_top_    = 1.0f;
    camera_->orthographic_near_   = 0.1f;
    camera_->orthographic_far_    = 100.0f;

    const auto data               = extractor_.extract(scene_);
    EXPECT_NEAR(data.camera_.projection_[0][0], 0.5f, 1e-4f);
}

TEST_F(SceneExtractorTest, CameraOverrideTakesPrecedence) {
    vulcal::render::CameraData override_camera;
    override_camera.view_       = glm::mat4{ 2.0f };
    override_camera.projection_ = glm::mat4{ 3.0f };
    override_camera.position_   = glm::vec3{ 7.0f, 8.0f, 9.0f };

    vulcal::scene::SceneExtractor::Options options;
    options.camera_override_ = override_camera;
    const auto data          = extractor_.extract(scene_, options);

    expect_near(glm::vec3{ data.camera_.view_[3] }, glm::vec3{ 0.0f, 0.0f, 0.0f });
    expect_near(glm::vec3{ data.camera_.view_[0] }, glm::vec3{ 2.0f, 0.0f, 0.0f });
    expect_near(data.camera_.position_, override_camera.position_);
}

TEST_F(SceneExtractorTest, FallsBackToFirstCameraWhenNoneIsPrimary) {
    camera_->primary_ = false;

    const auto data   = extractor_.extract(scene_);
    expect_near(data.camera_.position_, glm::vec3{ 0.0f, 0.0f, 5.0f });
}

TEST_F(SceneExtractorTest, ThrowsWhenSceneHasNoCamera) {
    vulcal::scene::Scene empty;
    EXPECT_THROW(extractor_.extract(empty), std::runtime_error);
}

TEST(SceneLight, DirectionUsesRotationWithoutTranslationOrScale) {
    vulcal::scene::Scene scene;
    auto entity = scene.create_entity("Light");
    entity.add_component<vulcal::scene::LightComponent>();
    auto& transform        = entity.get_component<vulcal::scene::TransformComponent>();
    transform.rotation_    = { 0.0f, glm::radians(90.0f), 0.0f };
    transform.translation_ = { 100.0f, -20.0f, 30.0f };
    transform.scale_       = { -2.0f, 4.0f, 0.25f };
    vulcal::scene::SceneExtractor::Options options;
    options.camera_override_ = vulcal::render::CameraData{};
    const auto data          = vulcal::scene::SceneExtractor{}.extract(scene, options);
    ASSERT_TRUE(data.directional_light_);
    expect_near(data.directional_light_->direction_, { -1.0f, 0.0f, 0.0f });
}

TEST(SceneExtractor, SkipsMeshDataWithoutBoundCache) {
    vulcal::scene::Scene scene;
    auto entity = scene.create_entity();
    auto& mesh  = entity.add_component<vulcal::scene::MeshRendererComponent>();
    vulcal::scene::SceneExtractor::Options options;
    options.camera_override_ = vulcal::render::CameraData{};
    vulcal::scene::SceneExtractor extractor;
    EXPECT_TRUE(extractor.extract(scene, options).items_.empty());
    mesh.mesh_asset_id_ = base::UUID{ 1 };
    EXPECT_TRUE(extractor.extract(scene, options).items_.empty());
    mesh.material_asset_id_ = base::UUID{ 2 };
    EXPECT_TRUE(extractor.extract(scene, options).items_.empty());
}

} // namespace
