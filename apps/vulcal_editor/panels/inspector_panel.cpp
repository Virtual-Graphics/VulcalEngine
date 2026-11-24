#include "vulcal_editor/panels/inspector_panel.h"

#include "base/logger.h"
#include "vulcal_editor/editor_actions.h"
#include "vulcal_editor/editor_context.h"
#include "scene/components.h"

#include <algorithm>
#include <cmath>
#include <utility>
#include <imgui.h>
#include <misc/cpp/imgui_stdlib.h>

namespace editor {
namespace {

bool section(const char* label) { return ImGui::CollapsingHeader(label, ImGuiTreeNodeFlags_DefaultOpen); }

using AssetAction = void (EditorActions::*)();

template<typename Component, typename Draw>
void draw_component(EditorContext& context, vulcal::scene::Entity entity, const char* label, Draw&& draw) {
    if (!entity.has_component<Component>()) return;
    ImGui::PushID(label);
    const bool expanded = section(label);
    bool remove         = false;
    if (ImGui::BeginPopupContextItem("ComponentMenu")) {
        remove = ImGui::MenuItem("Remove Component");
        ImGui::EndPopup();
    }
    if (expanded) {
        auto draft = entity.get_component<Component>();
        if (draw(draft) && !remove) context.set_component(entity, std::move(draft));
    }
    if (remove) {
        context.remove_component<Component>(entity);
        base::log::debug("Inspector: removed {} from entity {}", label, entity.get_component<vulcal::scene::IDComponent>().id_.to_string());
    }
    ImGui::PopID();
}

template<typename Component>
void add_component_item(EditorContext& context, vulcal::scene::Entity entity, const char* label, bool singleton = false) {
    const bool already_attached = entity.has_component<Component>();
    bool occupied               = false;
    if (singleton) context.scene().each<Component>([&](auto, const auto&) { occupied = true; });
    if (ImGui::MenuItem(label, nullptr, false, !already_attached && !occupied)) {
        context.add_component<Component>(entity);
        base::log::debug("Inspector: added {} to entity {}", label, entity.get_component<vulcal::scene::IDComponent>().id_.to_string());
    }
    if ((already_attached || occupied) && ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
        ImGui::SetTooltip("%s", already_attached ? "Entity already has this component." : "Only one per scene is currently supported.");
    }
}

bool finite(const glm::vec3& value) { return std::isfinite(value.x) && std::isfinite(value.y) && std::isfinite(value.z); }

bool bounded_drag(const char* label, float& value, float minimum, float maximum = 100000.0f, float speed = 0.01f,
                  const char* format = "%.3f") {
    float candidate = value;
    if (ImGui::DragFloat(label, &candidate, speed, minimum, maximum, format, ImGuiSliderFlags_AlwaysClamp) && std::isfinite(candidate)) {
        value = std::clamp(candidate, minimum, maximum);
        return true;
    }
    return false;
}

bool draw_transform(vulcal::scene::TransformComponent& transform) {
    if (!section("Transform")) return false;
    bool changed = false;
    auto translation = transform.translation_;
    if (ImGui::DragFloat3("Position", &translation.x, 0.05f) && finite(translation)) {
        transform.translation_ = translation;
        changed = true;
    }
    auto rotation = glm::degrees(transform.rotation_);
    if (ImGui::DragFloat3("Rotation (deg)", &rotation.x, 0.5f) && finite(rotation)) {
        transform.rotation_ = glm::radians(rotation);
        changed = true;
    }
    auto scale = transform.scale_;
    if (ImGui::DragFloat3("Scale", &scale.x, 0.01f) && finite(scale)) {
        for (int i = 0; i < 3; ++i) {
            if (std::abs(scale[i]) < 0.0001f) scale[i] = std::copysign(0.0001f, scale[i]);
        }
        transform.scale_ = scale;
        changed = true;
    }
    return changed;
}

bool draw_camera(vulcal::scene::CameraComponent& camera) {
    bool changed = false;
    changed |= ImGui::Checkbox("Primary", &camera.primary_);
    int projection = static_cast<int>(camera.projection_type_);
    if (ImGui::Combo("Projection", &projection, "Perspective\0Orthographic\0")) {
        camera.projection_type_ = static_cast<vulcal::scene::CameraComponent::ProjectionType>(projection);
        changed = true;
    }
    if (camera.projection_type_ == vulcal::scene::CameraComponent::ProjectionType::Perspective) {
        float fov = glm::degrees(camera.perspective_fov_y_);
        if (ImGui::SliderFloat("FOV (deg)", &fov, 1.0f, 179.0f, "%.1f", ImGuiSliderFlags_AlwaysClamp) && std::isfinite(fov)) {
            camera.perspective_fov_y_ = glm::radians(fov);
            changed = true;
        }
        ImGui::TextDisabled("Aspect: from render target");
        changed |= bounded_drag("Near", camera.perspective_near_, 0.001f, camera.perspective_far_ - 0.01f);
        changed |= bounded_drag("Far", camera.perspective_far_, camera.perspective_near_ + 0.01f);
    } else {
        changed |= bounded_drag("Left", camera.orthographic_left_, -100000.0f, camera.orthographic_right_ - 0.01f);
        changed |= bounded_drag("Right", camera.orthographic_right_, camera.orthographic_left_ + 0.01f, 100000.0f);
        changed |= bounded_drag("Bottom", camera.orthographic_bottom_, -100000.0f, camera.orthographic_top_ - 0.01f);
        changed |= bounded_drag("Top", camera.orthographic_top_, camera.orthographic_bottom_ + 0.01f, 100000.0f);
        changed |= bounded_drag("Near", camera.orthographic_near_, 0.001f, camera.orthographic_far_ - 0.01f);
        changed |= bounded_drag("Far", camera.orthographic_far_, camera.orthographic_near_ + 0.01f);
    }
    ImGui::TextDisabled("Viewport uses the independent editor camera.");
    return changed;
}

bool draw_light(vulcal::scene::LightComponent& light) {
    bool changed = false;
    ImGui::Text("Type: Directional");
    ImGui::TextDisabled("Direction follows Transform rotation (local -Z).");
    changed |= ImGui::ColorEdit3("Color", &light.color_.x);
    changed |= bounded_drag("Intensity", light.intensity_, 0.0f, 100.0f);
    changed |= ImGui::Checkbox("Shadows", &light.shadow_enabled_);
    ImGui::BeginDisabled(!light.shadow_enabled_);
    constexpr std::uint32_t sizes[]{ 512, 1024, 2048, 4096 };
    constexpr const char* labels[]{ "512", "1024", "2048", "4096" };
    int selected = 0;
    for (int i = 0; i < 4; ++i) {
        if (sizes[i] == light.shadow_map_size_) selected = i;
    }
    if (ImGui::Combo("Shadow resolution", &selected, labels, 4)) {
        light.shadow_map_size_ = sizes[selected];
        changed = true;
    }
    auto center = light.shadow_center_;
    if (ImGui::DragFloat3("Shadow center", &center.x, 0.05f) && finite(center)) {
        light.shadow_center_ = center;
        changed = true;
    }
    changed |= bounded_drag("Shadow extent", light.shadow_extent_, 0.01f);
    changed |= bounded_drag("Shadow near", light.shadow_near_, 0.001f, light.shadow_far_ - 0.01f);
    changed |= bounded_drag("Shadow far", light.shadow_far_, light.shadow_near_ + 0.01f);
    changed |= bounded_drag("Depth bias", light.shadow_depth_bias_, 0.0f, 0.05f, 0.00001f, "%.5f");
    changed |= bounded_drag("Normal bias", light.shadow_normal_bias_, 0.0f, 0.2f, 0.001f);
    ImGui::EndDisabled();
    return changed;
}

bool draw_mesh_renderer(vulcal::scene::MeshRendererComponent& mesh, bool can_import, AssetAction& pending) {
    bool changed = false;
    changed |= ImGui::Checkbox("Visible", &mesh.visible_);
    const auto mesh_id     = mesh.mesh_asset_id_.to_string();
    const auto material_id = mesh.material_asset_id_.to_string();
    ImGui::Text("Mesh Asset: %s", mesh.mesh_asset_id_.is_nil() ? "None" : mesh_id.c_str());
    ImGui::SameLine();
    ImGui::BeginDisabled(!can_import);
    if (ImGui::Button("...##ImportMesh")) pending = &EditorActions::import_mesh_asset;
    ImGui::EndDisabled();
    ImGui::Text("Material Asset: %s", mesh.material_asset_id_.is_nil() ? "None" : material_id.c_str());
    ImGui::SameLine();
    ImGui::BeginDisabled(!can_import);
    if (ImGui::Button("...##ImportMaterial")) pending = &EditorActions::import_material_asset;
    ImGui::EndDisabled();
    if (mesh.mesh_asset_id_.is_nil()) ImGui::TextDisabled("Assign a mesh asset to render this entity.");
    return changed;
}

bool draw_sprite_renderer(vulcal::scene::SpriteRendererComponent& sprite, bool can_import, AssetAction& pending) {
    bool changed = false;
    changed |= ImGui::Checkbox("Visible", &sprite.visible_);
    const auto texture_id = sprite.texture_asset_id_.to_string();
    ImGui::Text("Texture Asset: %s", sprite.texture_asset_id_.is_nil() ? "None" : texture_id.c_str());
    ImGui::SameLine();
    ImGui::BeginDisabled(!can_import);
    if (ImGui::Button("...##ImportTexture")) pending = &EditorActions::import_texture_asset;
    ImGui::EndDisabled();
    changed |= ImGui::ColorEdit4("Color", &sprite.color_.x);
    changed |= ImGui::DragFloat4("UV Rect", &sprite.uv_rect_.x, 0.01f);
    changed |= ImGui::DragFloat2("Pivot", &sprite.pivot_.x, 0.01f);
    changed |= ImGui::Checkbox("Flip X", &sprite.flip_x_);
    changed |= ImGui::Checkbox("Flip Y", &sprite.flip_y_);
    changed |= ImGui::DragInt("Sorting Order", &sprite.sorting_order_);
    return changed;
}

bool draw_environment(vulcal::scene::EnvironmentComponent& environment) {
    bool changed = false;
    changed |= ImGui::ColorEdit3("Ambient", &environment.ambient_color_.x);
    changed |= ImGui::ColorEdit3("Background", &environment.clear_color_.x);
    return changed;
}

void draw_script(const vulcal::scene::ScriptComponent& script, bool can_import, AssetAction& pending) {
    ImGui::Text("Script: %s", script.script_path_.empty() ? "None" : script.script_path_.c_str());
    ImGui::SameLine();
    ImGui::BeginDisabled(!can_import);
    if (ImGui::Button("...##AssignScript")) pending = &EditorActions::assign_script;
    ImGui::EndDisabled();
}

} // namespace

void InspectorPanel::draw(EditorContext& context, bool& open, EditorActions* actions) const {
    if (!open) return;
    AssetAction pending = nullptr;
    if (ImGui::Begin("Inspector", &open)) {
        const auto entity = context.selected_entity();
        if (entity) {
            ImGui::BeginDisabled(context.playing());
            // Selection is resolved each frame; no component pointer survives a scene change.
            const auto entity_id = entity.get_component<vulcal::scene::IDComponent>().id_.to_string();
            ImGui::PushID(entity_id.c_str());
            if (entity.has_component<vulcal::scene::TagComponent>()) {
                auto tag = entity.get_component<vulcal::scene::TagComponent>();
                if (ImGui::InputText("Name", &tag.name_)) context.set_component(entity, std::move(tag));
            }
            if (entity.has_component<vulcal::scene::IDComponent>()) {
                const auto id = entity.get_component<vulcal::scene::IDComponent>().id_.to_string();
                ImGui::TextDisabled("UUID: %s", id.c_str());
            }
            if (entity.has_component<vulcal::scene::TransformComponent>()) {
                auto transform = entity.get_component<vulcal::scene::TransformComponent>();
                if (draw_transform(transform)) context.set_component(entity, transform);
            }
            const bool can_import = context.has_project() && actions != nullptr;
            draw_component<vulcal::scene::MeshRendererComponent>(context, entity, "Mesh Renderer",
                    [&](auto& mesh) { return draw_mesh_renderer(mesh, can_import, pending); });
            draw_component<vulcal::scene::SpriteRendererComponent>(context, entity, "Sprite Renderer",
                    [&](auto& sprite) { return draw_sprite_renderer(sprite, can_import, pending); });
            draw_component<vulcal::scene::CameraComponent>(context, entity, "Camera", draw_camera);
            draw_component<vulcal::scene::LightComponent>(context, entity, "Light", draw_light);
            draw_component<vulcal::scene::EnvironmentComponent>(context, entity, "Environment", draw_environment);
            draw_component<vulcal::scene::ScriptComponent>(context, entity, "Script",
                    [&](const auto& script) {
                        draw_script(script, can_import, pending);
                        return false;
                    });
            ImGui::TextDisabled("Right-click optional component headers to remove.");
            if (ImGui::Button("Add Component")) ImGui::OpenPopup("AddComponentMenu");
            if (ImGui::BeginPopup("AddComponentMenu")) {
                add_component_item<vulcal::scene::MeshRendererComponent>(context, entity, "Mesh Renderer");
                add_component_item<vulcal::scene::CameraComponent>(context, entity, "Camera");
                add_component_item<vulcal::scene::LightComponent>(context, entity, "Light", true);
                add_component_item<vulcal::scene::EnvironmentComponent>(context, entity, "Environment", true);
                add_component_item<vulcal::scene::ScriptComponent>(context, entity, "Script");
                add_component_item<vulcal::scene::SpriteRendererComponent>(context, entity, "Sprite Renderer");
                ImGui::EndPopup();
            }
            ImGui::PopID();
            ImGui::EndDisabled();
        } else {
            ImGui::TextDisabled("Select an entity in Hierarchy.");
        }
    }
    ImGui::End();
    // Commit widget edits before importing so a draft cannot overwrite the new binding.
    if (pending) actions->run_guarded([&] { (actions->*pending)(); });
}

} // namespace editor
