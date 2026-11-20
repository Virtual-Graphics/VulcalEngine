#include "common/application_paths.h"
#include "engine/application.h"
#include "player_layer.h"
#include "resource/pak_provider.h"

#include <SDL3/SDL_main.h>

#include <iostream>
#include <memory>
#include <string>
#include <vector>

int main(int argc, char** argv) {
    try {
        std::vector<std::string> arguments;
        for (int index = 1; index < argc; ++index) arguments.emplace_back(argv[index]);
        const auto pak_file = vulcal::player::select_pak_file(arguments, apps::application_paths::executable_directory());
        engine::Application app({ .width = 1280, .height = 720, .title = "Vulcal Player" });
        app.push_layer(std::make_unique<vulcal::player::PlayerLayer>(pak_file));
        app.run();
    } catch (const std::exception& error) {
        std::cerr << "vulcal_player: " << error.what() << '\n';
        return 1;
    }
    return 0;
}
