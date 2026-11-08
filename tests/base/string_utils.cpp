#include "base/string_utils.h"

#include <gtest/gtest.h>
#include <string>

TEST(StringUtils, ToLowerConvertsAsciiLetters) {
    EXPECT_EQ(base::to_lower("HeLLo"), "hello");
    EXPECT_EQ(base::to_lower("already_lower"), "already_lower");
    EXPECT_EQ(base::to_lower(""), "");
    EXPECT_EQ(base::to_lower("1234._-"), "1234._-");
}

TEST(StringUtils, LowercasesFileExtensions) {
    EXPECT_EQ(base::lowercase_extension("Hero.PNG"), ".png");
    EXPECT_EQ(base::lowercase_extension("archive.TAR.GZ"), ".gz");
    EXPECT_EQ(base::lowercase_extension("README"), "");
}

TEST(StringUtils, ComparesExtensionsIgnoringCase) {
    EXPECT_TRUE(base::has_extension("scene.VSCENE", ".vscene"));
    EXPECT_TRUE(base::has_extension("scene.VSCENE", "vscene"));
    EXPECT_TRUE(base::has_extension("material.MAT", ".mat"));
    EXPECT_FALSE(base::has_extension("scene.vscene", ".vproj"));
    EXPECT_FALSE(base::has_extension("README", ".txt"));
    EXPECT_FALSE(base::has_extension("scene.vscene", ""));
}
