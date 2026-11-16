#include "gfx/context.h"

#include "gfx/profiler.h"

#include <SDL3/SDL.h>
#include <glad/glad.h>

#include <algorithm>
#include <stdexcept>
#include <string>

#include "base/logger.h"
#include "gfx/debug.h"

namespace vulcal::gfx {

void Context::prepare_attributes() {
    SDL_InitSubSystem(SDL_INIT_VIDEO);

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 6);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
}

Context::Context(SDL_Window* window)
        : window_(window) {
    gl_context_ = SDL_GL_CreateContext(window_);
    if (!gl_context_) {
        const std::string reason = SDL_GetError();
        base::log::error("SDL_GL_CreateContext failed: {}", reason);
        throw std::runtime_error("SDL_GL_CreateContext failed: " + reason);
    }

    if (!SDL_GL_MakeCurrent(window_, static_cast<SDL_GLContext>(gl_context_))) {
        const std::string reason = SDL_GetError();
        base::log::error("SDL_GL_MakeCurrent failed: {}", reason);
        throw std::runtime_error("SDL_GL_MakeCurrent failed: " + reason);
    }
    if (!SDL_GL_SetSwapInterval(0)) {
        base::log::warn("Could not disable VSync: {}", SDL_GetError());
    }

    if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(SDL_GL_GetProcAddress))) {
        base::log::error("gladLoadGLLoader failed: could not load OpenGL functions");
        throw std::runtime_error("gladLoadGLLoader failed: could not load OpenGL functions");
    }

    base::log::info("OpenGL context created: {} ({} / {})", reinterpret_cast<const char*>(glGetString(GL_VERSION)),
            reinterpret_cast<const char*>(glGetString(GL_VENDOR)), reinterpret_cast<const char*>(glGetString(GL_RENDERER)));

    init_debug_output();
    VULCAL_PROFILE_GPU_CONTEXT();

    GLint texture_size = 0, framebuffer_width = 0, framebuffer_height = 0;
    GLint viewport_size[2]{};
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &texture_size);
    glGetIntegerv(GL_MAX_FRAMEBUFFER_WIDTH, &framebuffer_width);
    glGetIntegerv(GL_MAX_FRAMEBUFFER_HEIGHT, &framebuffer_height);
    glGetIntegerv(GL_MAX_VIEWPORT_DIMS, viewport_size);
    limits_ = {
        .max_texture_size = static_cast<std::uint32_t>(texture_size),
        .max_render_target_width = static_cast<std::uint32_t>(std::min({ texture_size, framebuffer_width, viewport_size[0] })),
        .max_render_target_height = static_cast<std::uint32_t>(std::min({ texture_size, framebuffer_height, viewport_size[1] })),
    };

    base::log::debug("Render target limit: {}x{}", limits_.max_render_target_width, limits_.max_render_target_height);
}

Context::~Context() {
    if (gl_context_) {
        SDL_GL_DestroyContext(static_cast<SDL_GLContext>(gl_context_));
    }
}

void Context::swap_buffers() const {
    {
        VULCAL_PROFILE_SCOPE_NAMED("Swap buffers");
        SDL_GL_SwapWindow(window_);
    }
    {
        VULCAL_PROFILE_SCOPE_NAMED("Collect GPU timestamps");
        VULCAL_PROFILE_GPU_COLLECT();
    }
    VULCAL_PROFILE_RENDER_FRAME();
}

void Context::set_swap_interval(int interval) const { SDL_GL_SetSwapInterval(interval); }

} // namespace vulcal::gfx
