#pragma once

#include <memory>

namespace vulcal::scene {
class Scene;
}

namespace editor {

// Empty editor scene containing only a sun, a camera and environment settings.
std::unique_ptr<vulcal::scene::Scene> create_default_scene();

} // namespace editor
