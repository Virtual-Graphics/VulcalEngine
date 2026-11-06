#pragma once

namespace editor {

class EditorActions;
class EditorContext;

class InspectorPanel {
public:
    void draw(EditorContext& context, bool& open, EditorActions* actions = nullptr) const;
};

} // namespace editor
