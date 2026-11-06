#include "base/yaml.h"

#include <gtest/gtest.h>
#include <stdexcept>
#include <string>
#include <yaml-cpp/yaml.h>

namespace {

TEST(Yaml, ReadsVec2) {
    const auto node  = YAML::Load("value: [1.5, 2.5]");
    const auto value = base::yaml::read_vec<2>(node, "value");
    EXPECT_FLOAT_EQ(value.x, 1.5f);
    EXPECT_FLOAT_EQ(value.y, 2.5f);
}

TEST(Yaml, ReadsVec3) {
    const auto node  = YAML::Load("value: [1.5, 2.5, 3.5]");
    const auto value = base::yaml::read_vec<3>(node, "value");
    EXPECT_FLOAT_EQ(value.x, 1.5f);
    EXPECT_FLOAT_EQ(value.y, 2.5f);
    EXPECT_FLOAT_EQ(value.z, 3.5f);
}

TEST(Yaml, ReadsVec4) {
    const auto node  = YAML::Load("value: [1.5, 2.5, 3.5, 4.5]");
    const auto value = base::yaml::read_vec<4>(node, "value");
    EXPECT_FLOAT_EQ(value.x, 1.5f);
    EXPECT_FLOAT_EQ(value.y, 2.5f);
    EXPECT_FLOAT_EQ(value.z, 3.5f);
    EXPECT_FLOAT_EQ(value.w, 4.5f);
}

TEST(Yaml, RejectsMissingField) {
    const auto node = YAML::Load("value: [1.5, 2.5, 3.5]");
    EXPECT_THROW(base::yaml::read_vec<3>(node, "missing"), std::runtime_error);

    try {
        static_cast<void>(base::yaml::read_vec<3>(node, "missing"));
        FAIL() << "Expected a missing field to be rejected";
    } catch (const std::runtime_error& error) {
        EXPECT_NE(std::string(error.what()).find("missing"), std::string::npos);
    }
}

TEST(Yaml, RejectsNonSequenceField) {
    const auto node = YAML::Load("value: 42");
    EXPECT_THROW(base::yaml::read_vec<3>(node, "value"), std::runtime_error);
}

TEST(Yaml, RejectsWrongLength) {
    const auto node = YAML::Load("value: [1.5, 2.5]");
    EXPECT_THROW(base::yaml::read_vec<3>(node, "value"), std::runtime_error);

    try {
        static_cast<void>(base::yaml::read_vec<3>(node, "value"));
        FAIL() << "Expected a sequence with the wrong length to be rejected";
    } catch (const std::runtime_error& error) {
        const std::string message = error.what();
        EXPECT_NE(message.find("3 values"), std::string::npos);
        EXPECT_NE(message.find("value"), std::string::npos);
    }
}

TEST(Yaml, PropagatesElementConversionErrors) {
    const auto node = YAML::Load("value: [1.5, 2.5, nope]");
    EXPECT_THROW(base::yaml::read_vec<3>(node, "value"), YAML::Exception);
}

} // namespace
