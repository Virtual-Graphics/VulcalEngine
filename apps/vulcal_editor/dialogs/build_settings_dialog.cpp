#include "vulcal_editor/dialogs/build_settings_dialog.h"

#include "base/path_utils.h"
#include "common/nfd/file_dialog.h"
#include "vulcal_editor/editor_actions.h"
#include "vulcal_editor/editor_context.h"

#include <imgui.h>

namespace editor {

BuildSettingsDialog::BuildSettingsDialog(const EditorContext& context, EditorActions& actions)
        : Dialog("Build Settings"), context_(context), actions_(actions) {}

void BuildSettingsDialog::open() {
    const auto& build_info = context_.project_info().build_info;
    scene_file_ = build_info ? context_.project_directory() / build_info->open_scene : std::filesystem::path{};
    output_directory_ = build_info ? build_info->build_dir : std::filesystem::path{};
    error_.clear();
    request_open();
}

void BuildSettingsDialog::draw_contents() {
    ImGui::TextUnformatted("Open scene");
    ImGui::TextWrapped("%s", scene_file_.empty() ? "Choose a scene" : base::path_to_utf8(scene_file_).c_str());
    ImGui::SameLine();
    if (ImGui::Button("Browse scene...")) {
        try {
            if (auto path = apps::nfd::open_file(apps::nfd::FileFilter{ "Vulcal Scene", "vscene" }, context_.assets_directory()))
                scene_file_ = *path;
        } catch (const std::exception& error) {
            error_ = error.what();
        }
    }

    ImGui::TextUnformatted("Output directory");
    ImGui::TextWrapped("%s", output_directory_.empty() ? "Default: project/build" : base::path_to_utf8(output_directory_).c_str());
    ImGui::SameLine();
    if (ImGui::Button("Browse output...")) {
        try {
            if (auto path = apps::nfd::pick_folder(output_directory_.empty() ? context_.build_directory() : output_directory_))
                output_directory_ = *path;
        } catch (const std::exception& error) {
            error_ = error.what();
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("Use default")) output_directory_.clear();

    draw_inline_error(error_);

    ImGui::BeginDisabled(context_.playing() || scene_file_.empty());
    if (ImGui::Button("Save")) {
        try {
            actions_.set_build_settings(scene_file_, output_directory_);
            close();
        } catch (const std::exception& error) {
            error_ = error.what();
        }
    }
    ImGui::EndDisabled();
    ImGui::SameLine();
    if (ImGui::Button("Cancel")) close();
}

} // namespace editor
