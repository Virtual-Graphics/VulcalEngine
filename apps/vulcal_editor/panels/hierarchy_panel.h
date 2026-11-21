#pragma once

namespace editor {

class EditorContext;

class HierarchyPanel {
public:
    void draw(EditorContext& context, bool& open) const;
};

} // namespace editor
