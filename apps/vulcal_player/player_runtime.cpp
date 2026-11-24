#include "vulcal_player/player_runtime.h"
#include "base/path_utils.h"
#include "base/string_utils.h"
#include "common/project_paths.h"

#include "resource/resource_provider.h"

#include <yaml-cpp/yaml.h>

#include <algorithm>
#include <stdexcept>
#include <string_view>
#include <system_error>

namespace vulcal::player {

std::filesystem::path select_pak_file(const std::vector<std::string>& arguments, const std::filesystem::path& running_executable_directory) {
    // 指定了 .vpak
    if (arguments.size() > 1) { 
        throw std::invalid_argument("Usage: vulcal_player [--log] [game.vpak]");
    }
    if (!arguments.empty()) {
        const auto explicit_path = base::path_from_utf8(arguments.front());
        std::error_code error;
        const auto resolved = std::filesystem::canonical(explicit_path, error);
        if (error || !std::filesystem::is_regular_file(resolved) || !base::has_extension(resolved, ".vpak")) {
            throw std::runtime_error("The requested game package does not exist or is not a .vpak file");
        }
        return resolved;
    }

    // 没指定 .vpak
    std::vector<std::filesystem::path> packages;
    std::error_code error;
    for (std::filesystem::directory_iterator it(running_executable_directory, error), end; !error && it != end; it.increment(error)) {
        if (it->is_regular_file(error) && !error && base::has_extension(it->path(), ".vpak")) { 
            packages.push_back(it->path());
        }
    }
    if (error) { 
        throw std::runtime_error("Could not search for a game package beside vulcal_player: " + error.message());
    }
    if (packages.size() != 1) {
        throw std::runtime_error(packages.empty() ? "No .vpak package was found beside vulcal_player"
                                                  : "More than one .vpak package was found beside vulcal_player; pass one explicitly");
    }
    return std::filesystem::canonical(packages.front());
}

Manifest load_manifest(const resource::ResourceProvider& resources) {
    constexpr std::string_view manifest_path = "vulcal_game.vmanifest";
    if (!resources.contains(manifest_path)) { 
        throw std::runtime_error("Package is missing vulcal_game.vmanifest");
    }
    try {
        const auto node = YAML::Load(resources.read_text(manifest_path));
        if (!node.IsMap()) { 
            throw std::runtime_error("Manifest must be a map");
        }
        Manifest manifest{
            .version        = node["version"].as<std::uint32_t>(),
            .engine_version = node["engine_version"].as<std::string>(),
            .name           = node["name"].as<std::string>(),
            .open_scene     = node["open_scene"].as<std::string>(),
        };


        
        if (manifest.version != 1 || manifest.engine_version.empty() || manifest.name.empty() || manifest.open_scene.empty()) {
            throw std::runtime_error("Manifest is incomplete or uses an unsupported version");
        }
        try {
            (void) apps::project_paths::validate_scene_path(base::path_from_utf8(manifest.open_scene), ".", "Manifest open_scene");
        } catch (const std::invalid_argument&) {
            throw std::runtime_error("Manifest open_scene must be a safe relative .vscene path");
        }
        if (!resources.contains(manifest.open_scene)) { 
            throw std::runtime_error("Manifest open_scene is missing from the package");
        }
        return manifest;
    } catch (const YAML::Exception& error) {
        throw std::runtime_error("Invalid game manifest: " + std::string(error.what()));
    }
}

} // namespace vulcal::player
