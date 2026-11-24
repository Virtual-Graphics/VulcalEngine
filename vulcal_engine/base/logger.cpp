#include "base/logger.h"

#include "base/path_utils.h"

#include <fstream>
#include <memory>
#include <stdexcept>

#include <spdlog/sinks/ostream_sink.h>

namespace base {
namespace {

struct FileLog {
    explicit FileLog(const std::filesystem::path& path)
        : stream(path, std::ios::app), sink(stream) {
        if (!stream) {
            throw std::runtime_error("Could not open log file: " + path_to_utf8(path));
        }
    }

    std::ofstream stream;
    spdlog::sinks::ostream_sink_mt sink;
};

} // namespace

void add_file_log(const std::filesystem::path& path) {
    initLogger();
    std::filesystem::create_directories(path.parent_path().empty() ? "." : path.parent_path());
    const auto file = std::make_shared<FileLog>(path);
    file->sink.set_pattern("[%Y-%m-%d %T.%e] [%l] %v");
    auto& sinks = spdlog::default_logger()->sinks();
    sinks.insert(sinks.begin(), std::shared_ptr<spdlog::sinks::sink>(file, &file->sink));
}

} // namespace base
