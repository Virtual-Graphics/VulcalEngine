#include "editor/project/scene/scene_serializer.h"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>
#include <memory>
#include <string>

namespace {

class TemporaryDirectory {
public:
    TemporaryDirectory() {
        const auto suffix = std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());
        path_ = std::filesystem::temp_directory_path() / ("vulcal_asset_test_" + suffix);
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
    auto entity = scene.create_entity("Mesh");
    auto& mesh = entity.add_component<vulcal::scene::MeshRendererComponent>();
    mesh.mesh_asset_id_ = base::UUID{ 1311768467463790320ull };
    mesh.material_asset_id_ = base::UUID{ 1147797409030816545ull };

    const auto path = directory.path() / "scene.vscene";
    editor::SceneSerializer::serialize(scene, path);
    auto loaded = editor::SceneSerializer::deserialize(path);
    auto loaded_mesh = loaded->registry().view<vulcal::scene::MeshRendererComponent>();
    ASSERT_EQ(loaded_mesh.size(), 1u);
    const auto& component = loaded_mesh.get<vulcal::scene::MeshRendererComponent>(loaded_mesh.front());

    EXPECT_EQ(component.mesh_asset_id_, base::UUID{ 1311768467463790320ull });
    EXPECT_EQ(component.material_asset_id_, base::UUID{ 1147797409030816545ull });
}

TEST(SceneSerializer, LegacyRuntimeHandlesAreNotTreatedAsAssetIds) {
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

    auto loaded = editor::SceneSerializer::deserialize(path);
    const auto view = loaded->registry().view<const vulcal::scene::MeshRendererComponent>();
    ASSERT_EQ(view.size(), 1u);
    const auto& component = view.get<const vulcal::scene::MeshRendererComponent>(view.front());
    EXPECT_TRUE(component.mesh_asset_id_.is_nil());
    EXPECT_TRUE(component.material_asset_id_.is_nil());
}

} // namespace
