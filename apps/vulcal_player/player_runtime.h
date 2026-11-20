#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace vulcal::resource {
class ResourceProvider;
}

namespace vulcal::player {

struct Manifest {
    std::uint32_t version = 0;
    std::string engine_version;
    std::string name;
    std::string open_scene;
};

std::filesystem::path select_pak_file(const std::vector<std::string>& arguments, const std::filesystem::path& running_executable_directory);
Manifest load_manifest(const resource::ResourceProvider& resources);

} // namespace vulcal::player
