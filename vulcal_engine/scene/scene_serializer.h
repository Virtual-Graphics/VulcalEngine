#pragma once
#include "scene/scene.h"

#include <filesystem>
#include <memory>
#include <string_view>

namespace vulcal::resource {
class ResourceProvider;
}

namespace vulcal::scene {

class SceneSerializer {
public:
    static void serialize(const Scene& scene, const std::filesystem::path& path);
    static std::unique_ptr<Scene> deserialize(const std::filesystem::path& path);
    static std::unique_ptr<Scene> deserialize(const resource::ResourceProvider& resources, std::string_view logical_path);
};

} // namespace vulcal::scene
