#include "vulcal_editor/dialogs/new_project_dialog.h"

#include "base/logger.h"
#include "base/path_utils.h"
#include "common/nfd/file_dialog.h"
#include "vulcal_editor/editor_actions.h"
#include "vulcal_editor/editor_context.h"

#include <imgui.h>
#include <misc/cpp/imgui_stdlib.h>

namespace editor {

NewProjectDialog::NewProjectDialog(const EditorContext& context, EditorActions& actions)
        : Dialog("New Project"), context_(context), actions_(actions) {}

void NewProjectDialog::open() {
    name_.clear();
    directory_ = std::filesystem::current_path();
    error_.clear();
    request_open();
}

void NewProjectDialog::draw_contents() {
    ImGui::SetNextItemWidth(360.0f);
    ImGui::InputText("Project name", &name_);
    ImGui::TextUnformatted("Project directory");
    ImGui::TextWrapped("%s", directory_.empty() ? "Choose an existing directory" : base::path_to_utf8(directory_).c_str());
    if (ImGui::Button("Browse...")) {
        try {
            if (auto path = apps::nfd::pick_folder(directory_)) {
                directory_ = *path;
                if (name_.empty()) name_ = base::path_to_utf8(directory_.filename());
            }
        } catch (const std::exception& error) {
            base::log::error("Editor: could not choose project directory: {}", error.what());
            error_ = error.what();
        }
    }

    draw_inline_error(error_);

    const bool can_create = !context_.playing() && !name_.empty() && !directory_.empty();
    ImGui::BeginDisabled(!can_create);
    if (ImGui::Button("Create")) {
        try {
            actions_.create_project(directory_, name_);
            close();
        } catch (const std::exception& error) {
            base::log::error("Editor: could not create project '{}' in {}: {}", name_, directory_.string(), error.what());
            error_ = error.what();
        }
    }
    ImGui::EndDisabled();
    ImGui::SameLine();
    if (ImGui::Button("Cancel")) close();
}

} // namespace editor
