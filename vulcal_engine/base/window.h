#pragma once

#include <cstdint>
#include <functional>
#include <string>

struct SDL_Window;
union SDL_Event;

namespace base {

struct WindowProps {
    std::string title = "VulcalEngine";
    uint32_t width = 1280;
    uint32_t height = 720;
};

class Window {
public:
    using EventCallback = std::function<void(const SDL_Event&)>;

    explicit Window(const WindowProps& props);
    ~Window();

    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;

    bool should_close() const { return should_close_; }
    void poll_events();
    // Observes every polled event without consuming the window's own handling.
    void set_event_callback(EventCallback callback);

    const std::string& title() const { return title_; }
    bool set_title(std::string title);

    uint32_t width() const { return width_; }
    uint32_t height() const { return height_; }
    bool resized() const { return resized_; }
    void clear_resized() { resized_ = false; }

    SDL_Window* handle() const { return window_; }

private:
    SDL_Window* window_ = nullptr;
    std::string title_;
    uint32_t width_ = 0;
    uint32_t height_ = 0;
    bool should_close_ = false;
    bool resized_ = false;
    EventCallback event_callback_;
};

} // namespace base
