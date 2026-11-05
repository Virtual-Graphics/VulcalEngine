#pragma once
#include <vector>
#include <cstdint>
#include <glm/glm.hpp>

namespace vulcal::render{
struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;
};

class MeshData {
public:
    std::vector<Vertex> vertices_;
    std::vector<std::uint32_t> indices_;
};
}
