#pragma once

#include "base/uuid.h"
#include "scene/entity.h"

#include <stdexcept>
#include <string>
#include <vector>

namespace vulcal::script::lua {

base::UUID parse_uuid(const std::string& value);

template<typename T>
T& require_component(scene::Entity entity, const char* name) {
    if (!entity.valid()) throw std::runtime_error("entity is invalid");
    if (!entity.has_component<T>()) throw std::runtime_error(std::string("entity has no ") + name + " component");
    return entity.get_component<T>();
}

void require_size(const std::vector<float>& values, std::size_t expected, const char* name);
void require_size_3_or_4(const std::vector<float>& values, const char* name);

} // namespace vulcal::script::lua
