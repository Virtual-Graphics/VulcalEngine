#include "script/lua/lua_scene.h"

#include "scene/components.h"
#include "scene/scene.h"
#include "script/lua/lua_value.h"

namespace vulcal::script::lua {

BoundScene::BoundScene(scene::Scene& scene)
        : scene_(scene) {}

std::optional<BoundEntity> BoundScene::find(const std::string& name) const {
    const auto view = scene_.registry().view<const scene::TagComponent>();
    for (const auto handle: view) {
        if (view.get<const scene::TagComponent>(handle).name_ == name) {
            return BoundEntity{
                scene::Entity{ handle, scene_ }
            };
        }
    }
    return std::nullopt;
}

std::optional<BoundEntity> BoundScene::find_by_id(const std::string& id) const {
    const auto expected = parse_uuid(id);
    const auto view     = scene_.registry().view<const scene::IDComponent>();
    for (const auto handle: view) {
        if (view.get<const scene::IDComponent>(handle).id_ == expected) {
            return BoundEntity{
                scene::Entity{ handle, scene_ }
            };
        }
    }
    return std::nullopt;
}

BoundEntity BoundScene::create_entity(const std::string& name) { return BoundEntity{ scene_.create_entity(name) }; }

void BoundScene::destroy(BoundEntity entity) const { entity.destroy(); }

std::vector<BoundEntity> BoundScene::entities() const {
    std::vector<BoundEntity> result;
    const auto view = scene_.registry().view<const scene::IDComponent>();
    result.reserve(view.size());
    for (const auto handle: view) {
        result.emplace_back(scene::Entity{ handle, scene_ });
    }
    return result;
}

std::optional<BoundEnvironment> BoundScene::environment() const {
    const auto view = scene_.registry().view<const scene::EnvironmentComponent>();
    if (view.empty()) return std::nullopt;
    return BoundEnvironment{
        scene::Entity{ view.front(), scene_ }
    };
}

} // namespace vulcal::script::lua
