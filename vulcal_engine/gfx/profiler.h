#pragma once

#include "base/profiler.h"

#if defined(VULCAL_ENABLE_GPU_PROFILING)
#include <glad/glad.h>
#include <tracy/TracyOpenGL.hpp>

#define VULCAL_PROFILE_GPU_CONTEXT()           TracyGpuContext
#define VULCAL_PROFILE_GPU_SCOPE(name)         TracyGpuZone(name)
#define VULCAL_PROFILE_GPU_SCOPE_DYNAMIC(name) TracyGpuZoneTransient(___vulcal_gpu_zone, name, true)
#define VULCAL_PROFILE_GPU_COLLECT()           TracyGpuCollect
#else
#define VULCAL_PROFILE_GPU_CONTEXT()           ((void) 0)
#define VULCAL_PROFILE_GPU_SCOPE(name)         ((void) 0)
#define VULCAL_PROFILE_GPU_SCOPE_DYNAMIC(name) ((void) 0)
#define VULCAL_PROFILE_GPU_COLLECT()           ((void) 0)
#endif

#if defined(VULCAL_ENABLE_PROFILING)
#include <cstdint>

namespace vulcal::gfx::profiling {

struct FrameStats {
    std::uint64_t draw_calls_ = 0;
    std::uint64_t indices_    = 0;
};

inline thread_local FrameStats frame_stats;

inline void record_draw(std::uint64_t indices) {
    ++frame_stats.draw_calls_;
    frame_stats.indices_ += indices;
}

inline void end_frame() {
    VULCAL_PROFILE_PLOT("Render/Draw calls", frame_stats.draw_calls_);
    VULCAL_PROFILE_PLOT("Render/Submitted indices", frame_stats.indices_);
    frame_stats = {};
}

} // namespace vulcal::gfx::profiling

#define VULCAL_PROFILE_DRAW(indices)  ::vulcal::gfx::profiling::record_draw(indices)
#define VULCAL_PROFILE_RENDER_FRAME() ::vulcal::gfx::profiling::end_frame()
#else
#define VULCAL_PROFILE_DRAW(indices)  ((void) 0)
#define VULCAL_PROFILE_RENDER_FRAME() ((void) 0)
#endif
