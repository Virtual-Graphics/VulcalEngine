#pragma once

#include "vulcal_editor/dialogs/dialog.h"

#include <filesystem>

namespace editor {

class EditorActions;
class EditorContext;

class BuildSettingsDialog final : public Dialog {
public:
    BuildSettingsDialog(const EditorContext& context, EditorActions& actions);
    void open();

private:
    void draw_contents() override;

    const EditorContext& context_;
    EditorActions& actions_;
    std::filesystem::path scene_file_;
    std::filesystem::path output_directory_;
    std::string error_;
};

} // namespace editor
