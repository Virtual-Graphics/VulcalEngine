#pragma once

#include <filesystem>
#include <string>
#include <vector>

namespace apps::process {

int run_and_wait(const std::filesystem::path& executable, const std::vector<std::string>& arguments = {});

} // namespace apps::process
