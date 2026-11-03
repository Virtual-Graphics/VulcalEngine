#pragma once
#include <filesystem>
#include <string>
#include "base/logger.h"
#include "render/resources/mesh_data.h"
#include "tiny_obj_loader.h"
namespace vulcal::asset{

class OBJImporter {
public:
static MeshData load_obj(const std::filesystem::path& path){
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn, err;

    bool success = tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, path.string().c_str());

    if (!warn.empty()) {
        base::log::warn("{}", warn);
    }

    if (!err.empty()) {
        base::log::error("{}", err);
    }

    if (!success) {
        return {};
    }

    base::log::info("loaded obj: {}", path.string());

    bool has_normals = !attrib.normals.empty();
    if (!has_normals) {
        base::log::warn("file: {}, does not have normal attributes", path.string());
    }
    
    MeshData mesh;
    std::uint32_t next = 0;

    auto push_vertex = [&](glm::vec3 pos, glm::vec3 normal){
        mesh.vertices_.push_back({pos, normal});
        mesh.indices_.push_back(next++);
    };

    for (const auto& shape : shapes) {
        const auto& idx = shape.mesh.indices;
        if (has_normals){
            for (const auto& i : idx) {
                float vx = attrib.vertices[3 * i.vertex_index + 0];
                float vy = attrib.vertices[3 * i.vertex_index + 1];
                float vz = attrib.vertices[3 * i.vertex_index + 2];
          
                float nx = attrib.normals[3 * i.normal_index + 0];
                float ny = attrib.normals[3 * i.normal_index + 1];
                float nz = attrib.normals[3 * i.normal_index + 2];
            
                push_vertex(glm::vec3{vx, vy, vz}, glm::vec3{nx, ny, nz});
            }
        } else {
            for (std::size_t i = 0; i + 2 < idx.size(); i += 3) {

                auto p = [&](std::size_t k){ 
                    int vi = idx[i+k].vertex_index;
                    return glm::vec3{attrib.vertices[3*vi], attrib.vertices[3*vi+1], attrib.vertices[3*vi+2]};
                };

                glm::vec3 p0=p(0), p1=p(1), p2=p(2);
                glm::vec3 n = glm::cross(p1-p0, p2-p0);
                n = glm::normalize(n);

                push_vertex(p0,n);
                push_vertex(p1,n);
                push_vertex(p2,n);

            }
        }
    }
    return mesh;
}
};
}
