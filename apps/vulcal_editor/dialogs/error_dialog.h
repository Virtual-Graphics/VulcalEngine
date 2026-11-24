#pragma once

#include "vulcal_editor/dialogs/dialog.h"

namespace editor {

class ErrorDialog final : public Dialog {
public:
    ErrorDialog();
    void open(std::string message);

private:
    void draw_contents() override;

    std::string message_;
};

} // namespace editor
