#pragma once

#include <cstddef>
#include <glm/glm.hpp>
#include <stdexcept>
#include <string>
#include <yaml-cpp/yaml.h>

namespace base::yaml {

template<glm::length_t N>
glm::vec<N, float> read_vec(const YAML::Node& node, const char* key) {
    const auto values = node[key];
    if (!values.IsSequence() || values.size() != static_cast<std::size_t>(N)) {
        throw std::runtime_error(std::string("YAML field must contain ") + std::to_string(N) + " values: " + key);
    }
    glm::vec<N, float> result{};
    for (glm::length_t i = 0; i < N; ++i) result[i] = values[i].as<float>();
    return result;
}

} // namespace base::yaml
