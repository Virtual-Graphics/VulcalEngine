#pragma once

#include "vulcal_editor/dialogs/dialog.h"

#include <filesystem>

namespace editor {

class EditorActions;
class EditorContext;

class NewProjectDialog final : public Dialog {
public:
    NewProjectDialog(const EditorContext& context, EditorActions& actions);
    void open();

private:
    void draw_contents() override;

    const EditorContext& context_;
    EditorActions& actions_;
    std::string name_;
    std::filesystem::path directory_;
    std::string error_;
};

} // namespace editor
