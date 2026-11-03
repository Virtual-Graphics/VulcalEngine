#include "base/binary_io.h"

#include <array>
#include <bit>
#include <cstdint>
#include <gtest/gtest.h>
#include <ostream>
#include <sstream>
#include <stdexcept>
#include <string>

namespace {

struct U32Sample {
    std::uint32_t value;
    std::string bytes;
};

const std::array<U32Sample, 3> u32_samples{
    U32Sample{           0,                  std::string(4,   '\0') },
    U32Sample{ 0x89abcdefu, std::string("\xef\xcd\xab\x89",      4) },
    U32Sample{ 0xffffffffu,                  std::string(4, '\xff') },
};

struct U64Sample {
    std::uint64_t value;
    std::string bytes;
};

const std::array<U64Sample, 3> u64_samples{
    U64Sample{                     0,                                  std::string(8,   '\0') },
    U64Sample{ 0x0123456789abcdefull, std::string("\xef\xcd\xab\x89\x67\x45\x23\x01",      8) },
    U64Sample{ 0xffffffffffffffffull,                                  std::string(8, '\xff') },
};

struct FloatSample {
    std::uint32_t bits;
    std::string bytes;
};

const std::array<FloatSample, 4> float_samples{
    FloatSample{ 0x3f800000u,     std::string("\0\0\x80\x3f", 4) },
    FloatSample{ 0x80000000u,       std::string("\0\0\0\x80", 4) },
    FloatSample{ 0x7f800000u,     std::string("\0\0\x80\x7f", 4) },
    FloatSample{ 0x7fc12345u, std::string("\x45\x23\xc1\x7f", 4) },
};

TEST(BinaryIO, WritesU32InLittleEndian) {
    for (const auto& sample: u32_samples) {
        std::ostringstream output;
        base::binary::write_u32_le(output, sample.value);
        EXPECT_EQ(output.str(), sample.bytes);
    }
}

TEST(BinaryIO, ReadsU32InLittleEndian) {
    for (const auto& sample: u32_samples) {
        std::istringstream input(sample.bytes + 'x');
        EXPECT_EQ(base::binary::read_u32_le(input), sample.value);
        EXPECT_EQ(input.get(), 'x');
    }
}

TEST(BinaryIO, WritesU64InLittleEndian) {
    for (const auto& sample: u64_samples) {
        std::ostringstream output;
        base::binary::write_u64_le(output, sample.value);
        EXPECT_EQ(output.str(), sample.bytes);
    }
}

TEST(BinaryIO, ReadsU64InLittleEndian) {
    for (const auto& sample: u64_samples) {
        std::istringstream input(sample.bytes + 'x');
        EXPECT_EQ(base::binary::read_u64_le(input), sample.value);
        EXPECT_EQ(input.get(), 'x');
    }
}

TEST(BinaryIO, WritesFloatBitPatternsInLittleEndian) {
    for (const auto& sample: float_samples) {
        std::ostringstream output;
        base::binary::write_float_le(output, std::bit_cast<float>(sample.bits));
        EXPECT_EQ(output.str(), sample.bytes);
    }
}

TEST(BinaryIO, ReadsFloatBitPatternsInLittleEndian) {
    for (const auto& sample: float_samples) {
        std::istringstream input(sample.bytes + 'x');
        EXPECT_EQ(std::bit_cast<std::uint32_t>(base::binary::read_float_le(input)), sample.bits);
        EXPECT_EQ(input.get(), 'x');
    }
}

TEST(BinaryIO, RejectsTruncatedReads) {
    for (std::size_t size = 0; size < 8; ++size) {
        SCOPED_TRACE(size);
        std::istringstream u64_input(std::string(size, '\xff'));
        EXPECT_THROW(base::binary::read_u64_le(u64_input), std::runtime_error);
        if (size < 4) {
            std::istringstream u32_input(std::string(size, '\xff'));
            EXPECT_THROW(base::binary::read_u32_le(u32_input), std::runtime_error);
            std::istringstream float_input(std::string(size, '\xff'));
            EXPECT_THROW(base::binary::read_float_le(float_input), std::runtime_error);
        }
    }
}

TEST(BinaryIO, RejectsFailedWrites) {
    std::ostream output(nullptr);
    EXPECT_THROW(base::binary::write_u32_le(output, 1), std::runtime_error);
    EXPECT_THROW(base::binary::write_u64_le(output, 1), std::runtime_error);
    EXPECT_THROW(base::binary::write_float_le(output, 1.0f), std::runtime_error);
}

} // namespace
