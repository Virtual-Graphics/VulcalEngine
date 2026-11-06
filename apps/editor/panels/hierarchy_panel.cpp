#include "editor/panels/hierarchy_panel.h"

#include "editor/editor_context.h"

#include <imgui.h>

namespace editor {

void HierarchyPanel::draw(EditorContext& context, bool& open) const {
    if (!open) return;
    if (ImGui::Begin("Hierarchy", &open)) {
        vulcal::scene::Entity pending_delete;
        bool create = false;
        bool over_entity = false;
        const auto selected = context.selected_entity();
        context.scene().each<vulcal::scene::TagComponent>([&](vulcal::scene::Entity entity, const auto& tag) {
            const auto id = entity.get_component<vulcal::scene::IDComponent>().id_.to_string();
            ImGui::PushID(id.c_str());
            // A fixed hidden ID keeps duplicate names and names containing ## independent.
            if (ImGui::Selectable("##entity", entity == selected)) context.select(entity);
            const auto position = ImGui::GetItemRectMin();
            ImGui::GetWindowDrawList()->AddText(ImVec2{ position.x, position.y + ImGui::GetStyle().FramePadding.y },
                    ImGui::GetColorU32(ImGuiCol_Text), tag.name_.c_str());
            over_entity |= ImGui::IsItemHovered();
            if (ImGui::BeginPopupContextItem("EntityMenu")) {
                if (ImGui::MenuItem("Delete Entity", nullptr, false, !context.playing())) pending_delete = entity;
                ImGui::EndPopup();
            }
            ImGui::PopID();
        });
        if (ImGui::BeginPopupContextWindow("CreateEntityMenu", ImGuiPopupFlags_MouseButtonRight | ImGuiPopupFlags_NoOpenOverItems)) {
            create = ImGui::MenuItem("Create Entity", nullptr, false, !context.playing());
            ImGui::EndPopup();
        }
        if (!over_entity && ImGui::IsWindowHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) context.select({});
        // Registry storage is never mutated while its entity view is being traversed.
        if (pending_delete) context.destroy_entity(pending_delete);
        if (create) context.create_entity();
    }
    ImGui::End();
}

} // namespace editor
