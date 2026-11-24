#pragma once

#include <filesystem>
#include <mutex>
#include <utility>

#include <spdlog/spdlog.h>

namespace base {
// Configure file output during startup, before other threads start logging.
void add_file_log(const std::filesystem::path& path);

inline void initLogger() {
    static std::once_flag once;
    std::call_once(once, [] {
        spdlog::set_pattern("[%T.%e] [%^%l%$] %v");
#ifdef NDEBUG
        spdlog::set_level(spdlog::level::info);
#else
        spdlog::set_level(spdlog::level::debug);
#endif
        spdlog::flush_on(spdlog::level::warn);
    });
}

namespace log {

template <typename... Args>
inline void trace(spdlog::format_string_t<Args...> fmt, Args&&... args) {
    spdlog::trace(fmt, std::forward<Args>(args)...);
}

template <typename... Args>
inline void debug(spdlog::format_string_t<Args...> fmt, Args&&... args) {
    spdlog::debug(fmt, std::forward<Args>(args)...);
}

template <typename... Args>
inline void info(spdlog::format_string_t<Args...> fmt, Args&&... args) {
    spdlog::info(fmt, std::forward<Args>(args)...);
}

template <typename... Args>
inline void warn(spdlog::format_string_t<Args...> fmt, Args&&... args) {
    spdlog::warn(fmt, std::forward<Args>(args)...);
}

template <typename... Args>
inline void error(spdlog::format_string_t<Args...> fmt, Args&&... args) {
    spdlog::error(fmt, std::forward<Args>(args)...);
}

template <typename... Args>
inline void critical(spdlog::format_string_t<Args...> fmt, Args&&... args) {
    spdlog::critical(fmt, std::forward<Args>(args)...);
}

} // namespace log
} // namespace base
