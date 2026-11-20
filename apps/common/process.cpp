#include "common/process.h"

#include "base/path_utils.h"

#include <SDL3/SDL_error.h>
#include <SDL3/SDL_process.h>

#include <memory>
#include <stdexcept>
#include <string_view>
#include <vector>

namespace apps::process {

int run_and_wait(const std::filesystem::path& executable, const std::vector<std::string>& arguments) {
    if (executable.empty()) {
        throw std::invalid_argument("Process executable path cannot be empty");
    }

    std::vector<std::string> storage;
    storage.reserve(arguments.size() + 1);
    storage.push_back(base::path_to_utf8(executable));
    storage.insert(storage.end(), arguments.begin(), arguments.end());

    std::vector<const char*> argv;
    argv.reserve(storage.size() + 1);
    for (const auto& argument: storage) {
        argv.push_back(argument.c_str());
    }
    argv.push_back(nullptr);

    using Process = std::unique_ptr<SDL_Process, decltype(&SDL_DestroyProcess)>;
    Process process{ SDL_CreateProcess(argv.data(), false), SDL_DestroyProcess };
    if (!process) { 
        throw std::runtime_error("Could not start process '" + executable.string() + "': " + SDL_GetError());
    }

    int exit_code = -1;
    if (!SDL_WaitProcess(process.get(), true, &exit_code)) {
        throw std::runtime_error("Could not wait for process '" + executable.string() + "': " + SDL_GetError());
    }
    return exit_code;
}

} // namespace apps::process
