#include "render/shaders/embedded_shaders.h"

#include <array>
#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>
#include <iterator>
#include <string>
#include <utility>

TEST(EmbeddedShaders, MatchOriginalSources) {
    namespace shaders = vulcal::render::shaders;
    const std::array sources{
        std::pair{ "blinn_phong.vert", shaders::blinn_phong_vert },
        std::pair{ "blinn_phong.frag", shaders::blinn_phong_frag },
        std::pair{      "shadow.vert",      shaders::shadow_vert },
        std::pair{      "shadow.frag",      shaders::shadow_frag },
        std::pair{      "sprite.vert",      shaders::sprite_vert },
        std::pair{      "sprite.frag",      shaders::sprite_frag },
    };
    for (const auto& [filename, source]: sources) {
        SCOPED_TRACE(filename);
        std::ifstream input(std::filesystem::path{ VULCAL_TEST_SHADER_DIR } / filename, std::ios::binary);
        ASSERT_TRUE(input.is_open());
        std::string original{ std::istreambuf_iterator<char>{ input }, std::istreambuf_iterator<char>{} };
        // CMake and C++ raw strings normalize CRLF line endings.
        for (auto newline = original.find("\r\n"); newline != std::string::npos; newline = original.find("\r\n", newline)) {
            original.erase(newline, 1);
        }
        ASSERT_FALSE(original.empty());
        EXPECT_EQ(source, original);
    }
}
