#pragma once

#include <cstdint>

namespace vulcal::script::lua {

struct TimeState {
    float delta         = 0.0f;
    float elapsed       = 0.0f;
    std::uint32_t frame = 0;
};

class BoundTime {
public:
    explicit BoundTime(TimeState& state)
            : state_(state) {}

    float delta() const { return state_.delta; }
    float elapsed() const { return state_.elapsed; }
    std::uint32_t frame() const { return state_.frame; }

private:
    TimeState& state_;
};

} // namespace vulcal::script::lua
