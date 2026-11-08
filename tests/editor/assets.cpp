#include "editor/project/scene/scene_serializer.h"
#include "scene/components.h"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>
#include <memory>
#include <stdexcept>
#include <string>

namespace {

class TemporaryDirectory {
public:
    TemporaryDirectory() {
        const auto suffix = std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());
        path_             = std::filesystem::temp_directory_path() / ("vulcal_asset_test_" + suffix);
        std::filesystem::create_directories(path_);
    }

    ~TemporaryDirectory() { std::filesystem::remove_all(path_); }

    const std::filesystem::path& path() const { return path_; }

private:
    std::filesystem::path path_;
};

TEST(SceneSerializer, PersistsStableAssetIds) {
    TemporaryDirectory directory;
    vulcal::scene::Scene scene;
    auto entity             = scene.create_entity("Mesh");
    auto& mesh              = entity.add_component<vulcal::scene::MeshRendererComponent>();
    mesh.mesh_asset_id_     = base::UUID{ 1311768467463790320ull };
    mesh.material_asset_id_ = base::UUID{ 1147797409030816545ull };

    const auto path         = directory.path() / "scene.vscene";
    editor::SceneSerializer::serialize(scene, path);
    auto loaded      = editor::SceneSerializer::deserialize(path);
    auto loaded_mesh = loaded->registry().view<vulcal::scene::MeshRendererComponent>();
    ASSERT_EQ(loaded_mesh.size(), 1u);
    const auto& component = loaded_mesh.get<vulcal::scene::MeshRendererComponent>(loaded_mesh.front());

    EXPECT_EQ(component.mesh_asset_id_, base::UUID{ 1311768467463790320ull });
    EXPECT_EQ(component.material_asset_id_, base::UUID{ 1147797409030816545ull });
}

TEST(SceneSerializer, PersistsSpriteRendererComponent) {
    TemporaryDirectory directory;
    vulcal::scene::Scene scene;
    auto entity              = scene.create_entity("Sprite");
    auto& sprite             = entity.add_component<vulcal::scene::SpriteRendererComponent>();
    sprite.texture_asset_id_ = base::UUID{ 1311768467463790320ull };
    sprite.color_            = { 0.1f, 0.2f, 0.3f, 0.4f };
    sprite.uv_rect_          = { 0.05f, 0.1f, 0.95f, 0.9f };
    sprite.pivot_            = { 0.0f, 0.0f };
    sprite.flip_x_           = true;
    sprite.flip_y_           = false;
    sprite.sorting_order_    = -3;
    sprite.visible_          = false;

    const auto path          = directory.path() / "scene.vscene";
    editor::SceneSerializer::serialize(scene, path);
    auto loaded     = editor::SceneSerializer::deserialize(path);
    const auto view = loaded->registry().view<vulcal::scene::SpriteRendererComponent>();
    ASSERT_EQ(view.size(), 1u);
    const auto& component = view.get<vulcal::scene::SpriteRendererComponent>(view.front());

    EXPECT_EQ(component.texture_asset_id_, base::UUID{ 1311768467463790320ull });
    EXPECT_EQ(component.color_, sprite.color_);
    EXPECT_EQ(component.uv_rect_, sprite.uv_rect_);
    EXPECT_EQ(component.pivot_, sprite.pivot_);
    EXPECT_TRUE(component.flip_x_);
    EXPECT_FALSE(component.flip_y_);
    EXPECT_EQ(component.sorting_order_, -3);
    EXPECT_FALSE(component.visible_);
}

TEST(SceneSerializer, PersistsScriptComponentPath) {
    TemporaryDirectory directory;
    vulcal::scene::Scene scene;
    auto entity                                                          = scene.create_entity("Scripted");
    entity.add_component<vulcal::scene::ScriptComponent>().script_path_ = "Assets/Scripts/player.lua";

    const auto path                                                      = directory.path() / "scene.vscene";
    editor::SceneSerializer::serialize(scene, path);
    auto loaded     = editor::SceneSerializer::deserialize(path);
    const auto view = loaded->registry().view<vulcal::scene::ScriptComponent>();
    ASSERT_EQ(view.size(), 1u);
    EXPECT_EQ(view.get<vulcal::scene::ScriptComponent>(view.front()).script_path_, "Assets/Scripts/player.lua");
}

TEST(SceneSerializer, RejectsLegacySceneSchema) {
    TemporaryDirectory directory;
    const auto path = directory.path() / "legacy.vscene";
    {
        std::ofstream output(path);
        output << "format_version: 1\n"
                  "entities:\n"
                  "  - id: 1\n"
                  "    name: Mesh\n"
                  "    transform:\n"
                  "      translation: [0, 0, 0]\n"
                  "      rotation: [0, 0, 0]\n"
                  "      scale: [1, 1, 1]\n"
                  "    mesh_renderer:\n"
                  "      mesh: 17\n"
                  "      material: 29\n"
                  "      visible: true\n";
    }

    EXPECT_THROW(editor::SceneSerializer::deserialize(path), std::runtime_error);
}

TEST(SceneSerializer, RejectsUnsupportedFormatVersion) {
    TemporaryDirectory directory;
    const auto path = directory.path() / "future.vscene";
    {
        std::ofstream output(path);
        output << "format_version: 2\n"
                  "entities: []\n";
    }

    EXPECT_THROW(editor::SceneSerializer::deserialize(path), std::runtime_error);
}

} // namespace
