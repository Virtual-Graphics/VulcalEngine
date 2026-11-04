#include "asset/runtime_asset_library.h"
#include "asset_import/meta_data.h"
#include "asset_import/obj_asset_importer.h"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>
#include <string>

namespace {

class TemporaryDirectory {
public:
    TemporaryDirectory() {
        const auto suffix = std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());
        path_             = std::filesystem::temp_directory_path() / ("vulcal_import_test_" + suffix);
        std::filesystem::create_directories(path_);
    }

    ~TemporaryDirectory() { std::filesystem::remove_all(path_); }

    const std::filesystem::path& path() const { return path_; }

private:
    std::filesystem::path path_;
};

TEST(ObjAssetImporter, ReimportKeepsIdAndRuntimeLibraryLoadsCookedMesh) {
    TemporaryDirectory directory;
    const auto assets  = directory.path() / "Assets";
    const auto library = directory.path() / ".vulcal_engine";
    std::filesystem::create_directories(assets);
    const auto source = assets / "triangle.obj";
    {
        std::ofstream output(source);
        output << "v 0 0 0\nv 1 0 0\nv 0 1 0\nf 1 2 3\n";
    }

    const auto first_id  = apps::asset_import::ObjAssetImporter::import(source, assets, library);
    const auto second_id = apps::asset_import::ObjAssetImporter::import(source, assets, library);
    ASSERT_EQ(first_id, second_id);
    EXPECT_TRUE(std::filesystem::exists(source.string() + ".meta"));
    const auto metadata = apps::asset_import::read_metadata(apps::asset_import::metadata_path(source));
    EXPECT_EQ(metadata.id, first_id);
    EXPECT_EQ(metadata.format_version, apps::asset_import::metadata_format_version);

    const auto mesh = vulcal::asset::RuntimeAssetLibrary{ library }.load_mesh(first_id);
    ASSERT_EQ(mesh.vertices_.size(), 3u);
    ASSERT_EQ(mesh.indices_.size(), 3u);
    EXPECT_FLOAT_EQ(mesh.vertices_[1].position.x, 1.0f);
    EXPECT_FLOAT_EQ(mesh.vertices_[2].position.y, 1.0f);
}

TEST(RuntimeAssetLibrary, RejectsCorruptCookedMesh) {
    TemporaryDirectory directory;
    const base::UUID id{ 1311768467463790320ull };
    const auto path = directory.path() / (id.to_string() + ".vmesh");
    {
        std::ofstream output(path, std::ios::binary);
        output << "invalid";
    }

    EXPECT_THROW(vulcal::asset::RuntimeAssetLibrary{ directory.path() }.load_mesh(id), std::runtime_error);
}

} // namespace
