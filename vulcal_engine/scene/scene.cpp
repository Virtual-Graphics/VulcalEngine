#include "scene/scene.h"

#include "base/logger.h"

#include <tuple>

namespace vulcal::scene {
namespace {

template<typename T>
void copy_component(entt::registry& destination, const entt::registry& source, entt::entity entity) {
    if (const auto* component = source.try_get<T>(entity)) {
        destination.emplace<T>(entity, *component);
    }
}

using SceneComponentTypes = std::tuple<
    IDComponent,
    TagComponent,
    TransformComponent,
    MeshRendererComponent,
    SpriteRendererComponent,
    CameraComponent,
    LightComponent,
    EnvironmentComponent,
    ScriptComponent>;

template<typename... Components>
void copy_components(entt::registry& destination, const entt::registry& source, entt::entity entity, std::tuple<Components...>) {
    (copy_component<Components>(destination, source, entity), ...);
}

} // namespace

Scene::Scene(const Scene& other) {
    const auto& entities = *other.registry_.storage<entt::entity>();

    for (auto [entity]: entities.each()) {
        const auto copy = registry_.create(entity);
        copy_components(registry_, other.registry_, copy, SceneComponentTypes{});
    }
}

Scene& Scene::operator=(const Scene& other) {
    if (this != &other) {
        Scene copy(other);
        registry_.swap(copy.registry_);
    }

    return *this;
}

Entity Scene::create_entity(const std::string& name) {
    const auto entity = registry_.create();
    registry_.emplace<IDComponent>(entity);
    registry_.emplace<TagComponent>(entity, TagComponent{ name });
    registry_.emplace<TransformComponent>(entity);
    base::log::debug("Created entity '{}' (handle {})", name, static_cast<std::uint32_t>(entity));
    return Entity{ entity, *this };
}

bool Scene::contains(Entity entity) const { return entity.scene_ == this && registry_.valid(entity.handle_); }

void Scene::require_entity(Entity entity) const {
    if (!contains(entity)) {
        base::log::error("Entity is invalid or belongs to another scene");
        throw std::invalid_argument("Entity is invalid or belongs to another scene");
    }
}

void Scene::destroy_entity(Entity entity) {
    require_entity(entity);
    base::log::debug("Destroyed entity (handle {})", static_cast<std::uint32_t>(entity.handle_));
    registry_.destroy(entity.handle());
}

} // namespace vulcal::scene
