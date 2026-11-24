#include "base/logger.h"
#include "base/path_utils.h"
#include "common/application_paths.h"
#include "engine/application.h"
#include "player_layer.h"

#include <SDL3/SDL_main.h>
#include <spdlog/fmt/chrono.h>

#include <chrono>
#include <memory>
#include <random>
#include <string>
#include <string_view>
#include <vector>

namespace {

std::filesystem::path player_log_path(const std::filesystem::path& executable_directory) {
    const auto now = std::chrono::system_clock::now();
    const auto suffix = std::random_device{}();
    const auto filename = fmt::format("vulcal_player-{:%Y%m%d-%H%M%S}-{:08x}.log", std::chrono::floor<std::chrono::seconds>(now), suffix);
    return executable_directory / filename;
}

} // namespace

int main(int argc, char** argv) {
    base::initLogger();
    try {
        std::vector<std::string> arguments;
        bool log_requested = false;
        bool parse_options = true;
        for (int index = 1; index < argc; ++index) {
            const std::string_view argument(argv[index]);
            if (parse_options && argument == "--log") {
                log_requested = true;
            } else if (parse_options && argument == "--") {
                parse_options = false;
            } else {
                arguments.emplace_back(argument);
            }
        }
#ifdef VULCAL_PLAYER_DEFAULT_FILE_LOG
        constexpr bool log_by_default = true;
#else
        constexpr bool log_by_default = false;
#endif
        const auto executable_directory = apps::app_paths::running_executable_directory();
        if (log_requested || log_by_default) {
            try {
                const auto path = player_log_path(executable_directory);
                base::add_file_log(path);
                base::log::info("Player log: {}", base::path_to_utf8(path));
            } catch (const std::exception& error) {
                if (log_requested) throw;
                base::log::warn("Could not enable default file logging: {}", error.what());
            }
        }
        const auto pak_file = vulcal::player::select_pak_file(arguments, executable_directory);
        engine::Application app({ .width = 1280, .height = 720, .title = "Vulcal Player" });
        app.push_layer(std::make_unique<vulcal::player::PlayerLayer>(pak_file));
        app.run();
    } catch (const std::exception& error) {
        base::log::error("vulcal_player: {}", error.what());
        spdlog::default_logger()->flush();
        return 1;
    }
    return 0;
}
