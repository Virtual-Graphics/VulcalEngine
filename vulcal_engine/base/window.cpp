#include "base/window.h"

#include "base/profiler.h"

#include <SDL3/SDL.h>

#include <stdexcept>
#include <utility>

#include "base/input.h"
#include "base/logger.h"

namespace base {

Window::Window(const WindowProps& props)
        : title_(props.title),
          width_(props.width),
          height_(props.height) {
    if (!SDL_InitSubSystem(SDL_INIT_VIDEO)) {
        const std::string reason = SDL_GetError();
        log::error("SDL_InitSubSystem failed: {}", reason);
        throw std::runtime_error("SDL_InitSubSystem failed: " + reason);
    }

    window_ =
            SDL_CreateWindow(title_.c_str(), static_cast<int>(width_), static_cast<int>(height_), SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
    if (!window_) {
        const std::string reason = SDL_GetError();
        log::error("SDL_CreateWindow failed: {}", reason);
        throw std::runtime_error("SDL_CreateWindow failed: " + reason);
    }

    int width = 0;
    int height = 0;
    SDL_GetWindowSizeInPixels(window_, &width, &height);
    width_ = static_cast<uint32_t>(width);
    height_ = static_cast<uint32_t>(height);
    Input::reset(window_);
    log::debug("Window '{}' created: {}x{}", title_, width_, height_);
}

Window::~Window() {
    Input::reset();
    if (window_) {
        SDL_DestroyWindow(window_);
    }
    SDL_QuitSubSystem(SDL_INIT_VIDEO);
    log::debug("Window '{}' destroyed", title_);
}

void Window::poll_events() {
    VULCAL_PROFILE_SCOPE();
    Input::begin_frame();

    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        const bool own_window = SDL_GetWindowFromEvent(&event) == window_;
        if (own_window) Input::process_event(event);
        if (event_callback_) {
            event_callback_(event);
        }

        switch (event.type) {
            case SDL_EVENT_QUIT:
                log::debug("Quit requested");
                should_close_ = true;
                break;
            case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
                if (own_window) {
                    log::debug("Window close requested");
                    should_close_ = true;
                }
                break;
            case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED: {
                int width = 0;
                int height = 0;
                SDL_GetWindowSizeInPixels(window_, &width, &height);
                width_ = static_cast<uint32_t>(width);
                height_ = static_cast<uint32_t>(height);
                resized_ = true;
                log::trace("Window pixel size changed: {}x{}", width_, height_);
                break;
            }
            case SDL_EVENT_WINDOW_FOCUS_GAINED:
                if (own_window) log::debug("Window gained focus");
                break;
            case SDL_EVENT_WINDOW_FOCUS_LOST:
                if (own_window) log::debug("Window lost focus");
                break;
            default:
                break;
        }
    }
}

void Window::set_event_callback(EventCallback callback) { event_callback_ = std::move(callback); }

} // namespace base
