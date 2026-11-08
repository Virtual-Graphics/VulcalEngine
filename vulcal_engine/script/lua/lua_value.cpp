#include "script/lua/lua_value.h"

#include <charconv>
#include <cstdint>
#include <stdexcept>

namespace vulcal::script::lua {

base::UUID parse_uuid(const std::string& value) {
    if (value.empty()) throw std::invalid_argument("UUID string is empty");

    std::uint64_t parsed = 0;
    const auto* begin    = value.data();
    const auto* end      = begin + value.size();
    const auto result    = std::from_chars(begin, end, parsed);
    if (result.ec != std::errc{} || result.ptr != end) {
        throw std::invalid_argument("invalid UUID string: " + value);
    }
    return base::UUID{ parsed };
}

void require_size(const std::vector<float>& values, std::size_t expected, const char* name) {
    if (values.size() != expected) {
        throw std::invalid_argument(std::string(name) + " expects " + std::to_string(expected) + " values");
    }
}

void require_size_3_or_4(const std::vector<float>& values, const char* name) {
    if (values.size() != 3 && values.size() != 4) {
        throw std::invalid_argument(std::string(name) + " expects 3 or 4 values");
    }
}

} // namespace vulcal::script::lua
