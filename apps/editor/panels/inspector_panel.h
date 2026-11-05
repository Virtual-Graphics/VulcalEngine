#pragma once

#include "scene/entity.h"

#include <functional>

namespace editor {

class EditorContext;

class InspectorPanel {
public:
    struct AssetImportCallbacks {
        std::function<void(vulcal::scene::Entity)> import_mesh;
        std::function<void(vulcal::scene::Entity)> import_material;
    };

    void draw(EditorContext& context, bool& open, const AssetImportCallbacks& callbacks = {}) const;
};

} // namespace editor
