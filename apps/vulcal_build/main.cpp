#include "base/path_utils.h"
#include "vulcal_build/build_pipeline.h"

#include <SDL3/SDL_main.h>
#include <iostream>

int main(int argc, char** argv) {
    if (argc != 3) {
        std::cerr << "Usage: vulcal_build <project.vproj> <vulcal_player>\n";
        return 2;
    }

    try {
        const auto result = apps::vulcal_build::build_project(base::path_from_utf8(argv[1]), base::path_from_utf8(argv[2]));
        std::cout << "Built " << base::path_to_utf8(result.pak_file) << " and " << base::path_to_utf8(result.player_file) << '\n';
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "Build failed: " << error.what() << '\n';
        return 1;
    }
}
