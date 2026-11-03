#include "common/imgui/imgui_integration.h"

#include "base/window.h"
#include "gfx/command.h"
#include "gfx/pipeline_state.h"
#include "gfx/render_target.h"

#include <SDL3/SDL.h>
#include <backends/imgui_impl_opengl3.h>
#include <backends/imgui_impl_sdl3.h>
#include <imgui.h>
#include <stdexcept>

namespace apps {

ImGuiIntegration::ImGuiIntegration(base::Window& window)
        : window_(window) {
    if (ImGui::GetCurrentContext() != nullptr) {
        throw std::logic_error("ImGuiIntegration requires a single ImGui context");
    }

    const auto gl_context = SDL_GL_GetCurrentContext();
    if (gl_context == nullptr || SDL_GL_GetCurrentWindow() != window_.handle()) {
        throw std::logic_error("ImGuiIntegration requires the window's OpenGL context to be current");
    }

    IMGUI_CHECKVERSION();
    context_ = ImGui::CreateContext();

    try {
        // Docking is available in the dependency; each application opts in.
        ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        ImGui::StyleColorsDark();

        sdl_initialized_ = ImGui_ImplSDL3_InitForOpenGL(window_.handle(), gl_context);
        if (!sdl_initialized_) {
            throw std::runtime_error("Failed to initialize ImGui's SDL3 backend");
        }

        opengl_initialized_ = ImGui_ImplOpenGL3_Init("#version 460 core");
        if (!opengl_initialized_) {
            throw std::runtime_error("Failed to initialize ImGui's OpenGL backend");
        }

        window_.set_event_callback([this](const SDL_Event& event) {
            ImGui::SetCurrentContext(context_);
            ImGui_ImplSDL3_ProcessEvent(&event);
        });
    } catch (...) {
        shutdown();
        throw;
    }
}

ImGuiIntegration::~ImGuiIntegration() {
    window_.set_event_callback({});
    shutdown();
}

void ImGuiIntegration::begin_frame() {
    ImGui::SetCurrentContext(context_);
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();
}

void ImGuiIntegration::end_frame() {
    ImGui::SetCurrentContext(context_);
    ImGui::Render();
}

void ImGuiIntegration::render() {
    ImGui::SetCurrentContext(context_);

    // The overlay draws over the scene without clearing its framebuffer.
    vulcal::gfx::RenderTarget::unbind();
    vulcal::gfx::Command::set_viewport(window_.width(), window_.height());
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    vulcal::gfx::PipelineState::reset_cache();
}

bool ImGuiIntegration::wants_mouse() const {
    ImGui::SetCurrentContext(context_);
    return ImGui::GetIO().WantCaptureMouse;
}

bool ImGuiIntegration::wants_keyboard() const {
    ImGui::SetCurrentContext(context_);
    return ImGui::GetIO().WantCaptureKeyboard;
}

void ImGuiIntegration::shutdown() {
    ImGui::SetCurrentContext(context_);
    if (opengl_initialized_) {
        ImGui_ImplOpenGL3_Shutdown();
    }
    if (sdl_initialized_) {
        ImGui_ImplSDL3_Shutdown();
    }
    ImGui::DestroyContext(context_);
}

} // namespace apps
