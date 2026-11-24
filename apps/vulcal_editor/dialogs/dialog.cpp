#include "vulcal_editor/dialogs/dialog.h"

#include <imgui.h>

namespace editor {

Dialog::Dialog(const char* title)
        : title_(title) {}

void Dialog::request_open() { open_requested_ = true; }

void Dialog::draw() {
    const auto* title = title_.c_str();
    if (open_requested_) {
        ImGui::OpenPopup(title);
        open_requested_ = false;
    }
    if (!ImGui::IsPopupOpen(title)) return;

    const auto center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, { 0.5f, 0.5f });
    if (ImGui::BeginPopupModal(title, nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        draw_contents();
        ImGui::EndPopup();
    }
}

void Dialog::close() { ImGui::CloseCurrentPopup(); }

void Dialog::draw_inline_error(const std::string& error) {
    if (error.empty()) return;
    ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + 440.0f);
    ImGui::TextColored({ 1.0f, 0.4f, 0.3f, 1.0f }, "%s", error.c_str());
    ImGui::PopTextWrapPos();
}

} // namespace editor
