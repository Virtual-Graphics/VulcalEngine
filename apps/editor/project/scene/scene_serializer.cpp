#include "editor/project/scene/scene_serializer.h"

#include <fstream>
#include <yaml-cpp/yaml.h>

namespace editor {
namespace {
using namespace vulcal::scene;

template<typename T>
void field(YAML::Node node, const char* key, T& value, bool loading) {
    if (loading) value = node[key].as<T>();
    else node[key] = value;
}

template<glm::length_t N>
void field(YAML::Node node, const char* key, glm::vec<N, float>& value, bool loading) {
    if (loading) {
        for (glm::length_t i = 0; i < N; ++i) value[i] = node[key][i].as<float>();
    } else {
        YAML::Node data(YAML::NodeType::Sequence);
        data.SetStyle(YAML::EmitterStyle::Flow);
        for (glm::length_t i = 0; i < N; ++i) data.push_back(value[i]);
        node[key] = data;
    }
}

void fields(YAML::Node n, TransformComponent& c, bool load) {
    field(n, "translation", c.translation_, load);
    field(n, "rotation", c.rotation_, load);
    field(n, "scale", c.scale_, load);
}

void fields(YAML::Node n, MeshRendererComponent& c, bool load) {
    field(n, "mesh", c.mesh_.value_, load);
    field(n, "material", c.material_.value_, load);
    field(n, "visible", c.visible_, load);
}

void fields(YAML::Node n, CameraComponent& c, bool load) {
    std::string projection = c.projection_type_ == CameraComponent::ProjectionType::Perspective ? "perspective" : "orthographic";
    field(n, "projection", projection, load);
    if (projection == "perspective") c.projection_type_ = CameraComponent::ProjectionType::Perspective;
    else c.projection_type_ = CameraComponent::ProjectionType::Orthographic;
    field(n, "primary", c.primary_, load);
    field(n, "perspective_fov_y", c.perspective_fov_y_, load);
    field(n, "perspective_aspect", c.perspective_aspect_, load);
    field(n, "perspective_near", c.perspective_near_, load);
    field(n, "perspective_far", c.perspective_far_, load);
    field(n, "orthographic_left", c.orthographic_left_, load);
    field(n, "orthographic_right", c.orthographic_right_, load);
    field(n, "orthographic_bottom", c.orthographic_bottom_, load);
    field(n, "orthographic_top", c.orthographic_top_, load);
    field(n, "orthographic_near", c.orthographic_near_, load);
    field(n, "orthographic_far", c.orthographic_far_, load);
}

void fields(YAML::Node n, LightComponent& c, bool load) {
    std::string type = "directional";
    field(n, "type", type, load);
    field(n, "color", c.color_, load);
    field(n, "intensity", c.intensity_, load);
    field(n, "shadow_enabled", c.shadow_enabled_, load);
    field(n, "shadow_map_size", c.shadow_map_size_, load);
    field(n, "shadow_center", c.shadow_center_, load);
    field(n, "shadow_extent", c.shadow_extent_, load);
    field(n, "shadow_near", c.shadow_near_, load);
    field(n, "shadow_far", c.shadow_far_, load);
    field(n, "shadow_depth_bias", c.shadow_depth_bias_, load);
    field(n, "shadow_normal_bias", c.shadow_normal_bias_, load);
}

void fields(YAML::Node n, EnvironmentComponent& c, bool load) {
    field(n, "ambient_color", c.ambient_color_, load);
    field(n, "clear_color", c.clear_color_, load);
}

template<typename T>
void save_component(YAML::Node node, const char* key, const entt::registry& registry, entt::entity entity) {
    if (const auto* component = registry.try_get<T>(entity)) {
        auto copy = *component;
        fields(node[key], copy, false);
    }
}

template<typename T>
void load_component(const YAML::Node& node, const char* key, Entity entity) {
    if (auto data = node[key]) fields(data, entity.add_component<T>(), true);
}
} // namespace

void SceneSerializer::serialize(const vulcal::scene::Scene& scene, const std::filesystem::path& path) {
    YAML::Node root;
    root["format_version"] = 1;
    auto entities = root["entities"] = YAML::Node(YAML::NodeType::Sequence);
    const auto& registry = scene.registry();
    for (const auto entity : registry.view<const IDComponent, const TagComponent, const TransformComponent>()) {
        YAML::Node node;
        node["id"] = registry.get<IDComponent>(entity).id_.value();
        node["name"] = registry.get<TagComponent>(entity).name_;
        save_component<TransformComponent>(node, "transform", registry, entity);
        save_component<MeshRendererComponent>(node, "mesh_renderer", registry, entity);
        save_component<CameraComponent>(node, "camera", registry, entity);
        save_component<LightComponent>(node, "light", registry, entity);
        save_component<EnvironmentComponent>(node, "environment", registry, entity);
        entities.push_back(node);
    }
    std::ofstream output(path, std::ios::binary);
    output << root;
}

std::unique_ptr<vulcal::scene::Scene> SceneSerializer::deserialize(const std::filesystem::path& path) {
    std::ifstream input(path, std::ios::binary);
    const auto root = YAML::Load(input);
    const auto entities = root["entities"];
    auto scene = std::make_unique<vulcal::scene::Scene>();
    for (const auto& node : entities) {
        const auto id = node["id"].as<std::uint64_t>();
        auto entity = scene->create_entity(node["name"].as<std::string>());
        entity.get_component<IDComponent>().id_ = base::UUID{ id };
        fields(node["transform"], entity.get_component<TransformComponent>(), true);
        load_component<MeshRendererComponent>(node, "mesh_renderer", entity);
        load_component<CameraComponent>(node, "camera", entity);
        load_component<LightComponent>(node, "light", entity);
        load_component<EnvironmentComponent>(node, "environment", entity);
    }
    return scene;
}
} // namespace editor
