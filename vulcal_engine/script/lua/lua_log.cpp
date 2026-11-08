#include "script/lua/lua_log.h"

#include "base/logger.h"

#include <string>

namespace vulcal::script::lua {

void bind_log(sol::state& lua) {
    auto log     = lua.create_table();
    log["trace"] = [](const std::string& value) { base::log::trace("{}", value); };
    log["debug"] = [](const std::string& value) { base::log::debug("{}", value); };
    log["info"]  = [](const std::string& value) { base::log::info("{}", value); };
    log["warn"]  = [](const std::string& value) { base::log::warn("{}", value); };
    log["error"] = [](const std::string& value) { base::log::error("{}", value); };
    lua["Log"]   = log;
}

} // namespace vulcal::script::lua
