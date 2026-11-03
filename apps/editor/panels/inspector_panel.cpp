#include "editor/panels/inspector_panel.h"

#include "editor/editor_context.h"

#include <algorithm>
#include <cmath>
#include <imgui.h>
#include <misc/cpp/imgui_stdlib.h>

namespace editor {
namespace {

bool section(const char* label) { return ImGui::CollapsingHeader(label, ImGuiTreeNodeFlags_DefaultOpen); }

template<typename Component, typename Draw>
void draw_component(vulcal::scene::Entity entity, const char* label, Draw&& draw) {
    if (!entity.has_component<Component>()) return;
    ImGui::PushID(label);
    const bool expanded = section(label);
    bool remove = false;
    if (ImGui::BeginPopupContextItem("ComponentMenu")) {
        remove = ImGui::MenuItem("Remove Component");
        ImGui::EndPopup();
    }
    if (expanded) draw(entity.get_component<Component>());
    // No reference to this component is used after removal.
    if (remove) entity.remove_component<Component>();
    ImGui::PopID();
}

template<typename Component>
void add_component_item(EditorContext& context, vulcal::scene::Entity entity, const char* label, bool singleton = false) {
    const bool already_attached = entity.has_component<Component>();
    bool occupied = false;
    if (singleton) context.scene().each<Component>([&](auto, const auto&) { occupied = true; });
    if (ImGui::MenuItem(label, nullptr, false, !already_attached && !occupied)) entity.add_component<Component>();
    if ((already_attached || occupied) && ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
        ImGui::SetTooltip("%s", already_attached ? "Entity already has this component." : "Only one per scene is currently supported.");
    }
}

bool finite(const glm::vec3& value) { return std::isfinite(value.x) && std::isfinite(value.y) && std::isfinite(value.z); }

void bounded_drag(const char* label, float& value, float minimum, float maximum = 100000.0f, float speed = 0.01f,
        const char* format = "%.3f") {
    float candidate = value;
    if (ImGui::DragFloat(label, &candidate, speed, minimum, maximum, format, ImGuiSliderFlags_AlwaysClamp) && std::isfinite(candidate)) {
        value = std::clamp(candidate, minimum, maximum);
    }
}

void draw_transform(vulcal::scene::TransformComponent& transform) {
    if (!section("Transform")) return;
    auto translation = transform.translation_;
    if (ImGui::DragFloat3("Position", &translation.x, 0.05f) && finite(translation)) transform.translation_ = translation;
    auto rotation = glm::degrees(transform.rotation_);
    if (ImGui::DragFloat3("Rotation (deg)", &rotation.x, 0.5f) && finite(rotation)) transform.rotation_ = glm::radians(rotation);
    auto scale = transform.scale_;
    if (ImGui::DragFloat3("Scale", &scale.x, 0.01f) && finite(scale)) {
        for (int i = 0; i < 3; ++i) {
            if (std::abs(scale[i]) < 0.0001f) scale[i] = std::copysign(0.0001f, scale[i]);
        }
        transform.scale_ = scale;
    }
}

void draw_camera(vulcal::scene::CameraComponent& camera) {
    ImGui::Checkbox("Primary", &camera.primary_);
    int projection = static_cast<int>(camera.projection_type_);
    if (ImGui::Combo("Projection", &projection, "Perspective\0Orthographic\0")) {
        camera.projection_type_ = static_cast<vulcal::scene::CameraComponent::ProjectionType>(projection);
    }
    if (camera.projection_type_ == vulcal::scene::CameraComponent::ProjectionType::Perspective) {
        float fov = glm::degrees(camera.perspective_fov_y_);
        if (ImGui::SliderFloat("FOV (deg)", &fov, 1.0f, 179.0f, "%.1f", ImGuiSliderFlags_AlwaysClamp) && std::isfinite(fov)) {
            camera.perspective_fov_y_ = glm::radians(fov);
        }
        bounded_drag("Aspect", camera.perspective_aspect_, 0.01f, 100.0f);
        bounded_drag("Near", camera.perspective_near_, 0.001f, camera.perspective_far_ - 0.01f);
        bounded_drag("Far", camera.perspective_far_, camera.perspective_near_ + 0.01f);
    } else {
        auto edit_bound = [](const char* label, float& value, float minimum, float maximum) {
            float candidate = value;
            if (ImGui::DragFloat(label, &candidate, 0.01f, minimum, maximum, "%.3f", ImGuiSliderFlags_AlwaysClamp) &&
                    std::isfinite(candidate))
                value = std::clamp(candidate, minimum, maximum);
        };
        edit_bound("Left", camera.orthographic_left_, -100000.0f, camera.orthographic_right_ - 0.01f);
        edit_bound("Right", camera.orthographic_right_, camera.orthographic_left_ + 0.01f, 100000.0f);
        edit_bound("Bottom", camera.orthographic_bottom_, -100000.0f, camera.orthographic_top_ - 0.01f);
        edit_bound("Top", camera.orthographic_top_, camera.orthographic_bottom_ + 0.01f, 100000.0f);
        bounded_drag("Near", camera.orthographic_near_, 0.001f, camera.orthographic_far_ - 0.01f);
        bounded_drag("Far", camera.orthographic_far_, camera.orthographic_near_ + 0.01f);
    }
    ImGui::TextDisabled("Viewport uses the independent editor camera.");
}

void draw_light(vulcal::scene::LightComponent& light) {
    ImGui::Text("Type: Directional");
    ImGui::TextDisabled("Direction follows Transform rotation (local -Z).");
    ImGui::ColorEdit3("Color", &light.color_.x);
    bounded_drag("Intensity", light.intensity_, 0.0f, 100.0f);
    ImGui::Checkbox("Shadows", &light.shadow_enabled_);
    ImGui::BeginDisabled(!light.shadow_enabled_);
    constexpr std::uint32_t sizes[]{ 512, 1024, 2048, 4096 };
    constexpr const char* labels[]{ "512", "1024", "2048", "4096" };
    int selected = 0;
    for (int i = 0; i < 4; ++i) {
        if (sizes[i] == light.shadow_map_size_) selected = i;
    }
    if (ImGui::Combo("Shadow resolution", &selected, labels, 4)) light.shadow_map_size_ = sizes[selected];
    auto center = light.shadow_center_;
    if (ImGui::DragFloat3("Shadow center", &center.x, 0.05f) && finite(center)) light.shadow_center_ = center;
    bounded_drag("Shadow extent", light.shadow_extent_, 0.01f);
    bounded_drag("Shadow near", light.shadow_near_, 0.001f, light.shadow_far_ - 0.01f);
    bounded_drag("Shadow far", light.shadow_far_, light.shadow_near_ + 0.01f);
    bounded_drag("Depth bias", light.shadow_depth_bias_, 0.0f, 0.05f, 0.00001f, "%.5f");
    bounded_drag("Normal bias", light.shadow_normal_bias_, 0.0f, 0.2f, 0.001f);
    ImGui::EndDisabled();
}

} // namespace

void InspectorPanel::draw(EditorContext& context, bool& open) const {
    if (!open) return;
    if (ImGui::Begin("Inspector", &open)) {
        const auto entity = context.selected_entity();
        if (entity) {
            // Selection is resolved each frame; no component pointer survives a scene change.
            const auto entity_id = entity.get_component<vulcal::scene::IDComponent>().id_.to_string();
            ImGui::PushID(entity_id.c_str());
            if (entity.has_component<vulcal::scene::TagComponent>()) {
                ImGui::InputText("Name", &entity.get_component<vulcal::scene::TagComponent>().name_);
            }
            if (entity.has_component<vulcal::scene::IDComponent>()) {
                const auto id = entity.get_component<vulcal::scene::IDComponent>().id_.to_string();
                ImGui::TextDisabled("UUID: %s", id.c_str());
            }
            if (entity.has_component<vulcal::scene::TransformComponent>())
                draw_transform(entity.get_component<vulcal::scene::TransformComponent>());
            draw_component<vulcal::scene::MeshRendererComponent>(entity, "Mesh Renderer", [](auto& mesh) {
                ImGui::Checkbox("Visible", &mesh.visible_);
                ImGui::Text("Mesh: %llu", static_cast<unsigned long long>(mesh.mesh_.value_));
                ImGui::Text("Material: %llu", static_cast<unsigned long long>(mesh.material_.value_));
                if (mesh.mesh_.value_ == 0 || mesh.material_.value_ == 0)
                    ImGui::TextDisabled("Not rendered: mesh and material must both be assigned.");
            });
            draw_component<vulcal::scene::CameraComponent>(entity, "Camera", draw_camera);
            draw_component<vulcal::scene::LightComponent>(entity, "Light", draw_light);
            draw_component<vulcal::scene::EnvironmentComponent>(entity, "Environment", [](auto& environment) {
                ImGui::ColorEdit3("Ambient", &environment.ambient_color_.x);
                ImGui::ColorEdit3("Background", &environment.clear_color_.x);
            });
            ImGui::TextDisabled("Right-click optional component headers to remove.");
            if (ImGui::Button("Add Component")) ImGui::OpenPopup("AddComponentMenu");
            if (ImGui::BeginPopup("AddComponentMenu")) {
                add_component_item<vulcal::scene::MeshRendererComponent>(context, entity, "Mesh Renderer");
                add_component_item<vulcal::scene::CameraComponent>(context, entity, "Camera");
                add_component_item<vulcal::scene::LightComponent>(context, entity, "Light", true);
                add_component_item<vulcal::scene::EnvironmentComponent>(context, entity, "Environment", true);
                ImGui::EndPopup();
            }
            ImGui::PopID();
        } else {
            ImGui::TextDisabled("Select an entity in Hierarchy.");
        }
    }
    ImGui::End();
}

} // namespace editor
