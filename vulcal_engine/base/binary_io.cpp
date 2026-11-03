#include "base/binary_io.h"

#include <array>
#include <bit>
#include <istream>
#include <limits>
#include <ostream>
#include <stdexcept>

namespace base::binary {

static_assert(sizeof(float) == 4 && std::numeric_limits<float>::is_iec559, "Binary IO requires IEEE 754 binary32 floats");

void write_u32_le(std::ostream& output, std::uint32_t value) {
    const std::array<char, 4> bytes{
        static_cast<char>(value & 0xff),
        static_cast<char>((value >> 8) & 0xff),
        static_cast<char>((value >> 16) & 0xff),
        static_cast<char>((value >> 24) & 0xff),
    };
    output.write(bytes.data(), static_cast<std::streamsize>(bytes.size()));
    if (!output) {
        throw std::runtime_error("Could not write binary stream");
    }
}

void write_u64_le(std::ostream& output, std::uint64_t value) {
    write_u32_le(output, static_cast<std::uint32_t>(value));
    write_u32_le(output, static_cast<std::uint32_t>(value >> 32));
}

void write_float_le(std::ostream& output, float value) { write_u32_le(output, std::bit_cast<std::uint32_t>(value)); }

std::uint32_t read_u32_le(std::istream& input) {
    std::array<unsigned char, 4> bytes{};
    input.read(reinterpret_cast<char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
    if (!input) {
        throw std::runtime_error("Could not read binary stream");
    }

    return static_cast<std::uint32_t>(bytes[0]) | (static_cast<std::uint32_t>(bytes[1]) << 8) |
           (static_cast<std::uint32_t>(bytes[2]) << 16) | (static_cast<std::uint32_t>(bytes[3]) << 24);
}

std::uint64_t read_u64_le(std::istream& input) {
    const std::uint64_t low  = read_u32_le(input);
    const std::uint64_t high = read_u32_le(input);
    return low | (high << 32);
}

float read_float_le(std::istream& input) { return std::bit_cast<float>(read_u32_le(input)); }

} // namespace base::binary
