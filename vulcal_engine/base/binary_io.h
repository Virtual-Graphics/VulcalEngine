#pragma once

#include <cstdint>
#include <iosfwd>

namespace base::binary {

// Little-endian integers and IEEE 754 binary32 floats. Throws on stream failure.
void write_u32_le(std::ostream& output, std::uint32_t value);
void write_u64_le(std::ostream& output, std::uint64_t value);
void write_float_le(std::ostream& output, float value);

std::uint32_t read_u32_le(std::istream& input);
std::uint64_t read_u64_le(std::istream& input);
float read_float_le(std::istream& input);

} // namespace base::binary
