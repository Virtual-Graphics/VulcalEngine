#include "engine/application.h"

#include "base/profiler.h"

#include <chrono>

#include "base/logger.h"
#include "engine/layer.h"
#include "gfx/command.h"

namespace engine {

Application::Application(const ApplicationCreateInfo& info) : info_(info) {
    base::initLogger();
    base::log::info("Starting application '{}' ({}x{})", info_.title, info_.width, info_.height);

    vulcal::gfx::Context::prepare_attributes();
    window_ = std::make_unique<base::Window>(base::WindowProps{ .title = info.title, .width = info.width, .height = info.height });
    context_ = std::make_unique<vulcal::gfx::Context>(window_->handle());
}

Application::~Application() {
    base::log::info("Shutting down application '{}'", info_.title);
    layer_stack_.detach_all();
    context_.reset();
    window_.reset();
    spdlog::default_logger()->flush();
}

void Application::push_layer(std::unique_ptr<Layer> layer) {
    base::log::debug("Pushing layer '{}'", layer->get_name());
    layer->application_ = this;
    layer_stack_.push_layer(std::move(layer));
}

void Application::push_overlay(std::unique_ptr<Layer> overlay) {
    base::log::debug("Pushing overlay '{}'", overlay->get_name());
    overlay->application_ = this;
    layer_stack_.push_overlay(std::move(overlay));
}

void Application::run() {
    VULCAL_PROFILE_SCOPE();
    base::log::info("Entering main loop");
    VULCAL_PROFILE_THREAD("Main");
    running_ = true;
    auto last = std::chrono::high_resolution_clock::now();

    while (running_ && !window_->should_close()) {
        VULCAL_PROFILE_FRAME();
        VULCAL_PROFILE_SCOPE_NAMED("Frame");
        window_->poll_events();

        const auto now = std::chrono::high_resolution_clock::now();
        const base::Timestep dt(std::chrono::duration<float>(now - last).count());
        last = now;
        VULCAL_PROFILE_PLOT("Frame/Delta (ms)", dt.get_milliseconds());

        if (window_->width() == 0 || window_->height() == 0) {
            continue;
        }

        vulcal::gfx::Command::set_viewport(window_->width(), window_->height());

        {
            VULCAL_PROFILE_SCOPE_NAMED("Layer update");
            for (auto& layer : layer_stack_) {
                VULCAL_PROFILE_SCOPE_DYNAMIC(layer->get_name().c_str());
                layer->on_update(dt);
            }
        }

        {
            VULCAL_PROFILE_SCOPE_NAMED("Layer render");
            for (auto& layer : layer_stack_) {
                VULCAL_PROFILE_SCOPE_DYNAMIC(layer->get_name().c_str());
                layer->on_render();
            }
        }

        context_->swap_buffers();
    }

    running_ = false;
    base::log::info("Main loop exited");
}

} // namespace engine
