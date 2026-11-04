#include "base/uuid.h"

#include <iomanip>
#include <random>
#include <sstream>

namespace base {

UUID::UUID(std::uint64_t value)
        : value_(value) {}

UUID UUID::generate() {
    static thread_local std::mt19937_64 engine{ std::random_device{}() };
    std::uniform_int_distribution<std::uint64_t> distribution;
    const auto value = distribution(engine);
    return UUID{ value == 0 ? 1 : value };
}

std::string UUID::to_string() const {
    std::ostringstream stream;
    stream << std::dec << value_;
    return stream.str();
}

} // namespace base
