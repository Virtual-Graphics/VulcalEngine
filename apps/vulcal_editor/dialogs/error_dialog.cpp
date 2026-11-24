#include "vulcal_editor/dialogs/error_dialog.h"

#include <imgui.h>
#include <utility>

namespace editor {

ErrorDialog::ErrorDialog()
        : Dialog("Operation Failed") {}

void ErrorDialog::open(std::string message) {
    message_ = std::move(message);
    request_open();
}

void ErrorDialog::draw_contents() {
    ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + 480.0f);
    ImGui::TextUnformatted(message_.c_str());
    ImGui::PopTextWrapPos();
    if (ImGui::Button("OK")) close();
}

} // namespace editor
