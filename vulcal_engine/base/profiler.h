#pragma once

#if defined(VULCAL_ENABLE_PROFILING)
#include <tracy/Tracy.hpp>

#define VULCAL_PROFILE_SCOPE() ZoneScoped
// Scope names must be string literals. Attach runtime details with VULCAL_PROFILE_TEXT.
#define VULCAL_PROFILE_SCOPE_NAMED(name)   ZoneScopedN(name)
#define VULCAL_PROFILE_SCOPE_DYNAMIC(name) ZoneTransientN(___tracy_scoped_zone, name, true)
#define VULCAL_PROFILE_TEXT(text, size)    ZoneText(text, size)
#define VULCAL_PROFILE_FRAME()             FrameMark
#define VULCAL_PROFILE_PLOT(name, value)   TracyPlot(name, static_cast<double>(value))
#define VULCAL_PROFILE_THREAD(name)        tracy::SetThreadName(name)
#else
#define VULCAL_PROFILE_SCOPE()             ((void) 0)
#define VULCAL_PROFILE_SCOPE_NAMED(name)   ((void) 0)
#define VULCAL_PROFILE_SCOPE_DYNAMIC(name) ((void) 0)
#define VULCAL_PROFILE_TEXT(text, size)    ((void) 0)
#define VULCAL_PROFILE_FRAME()             ((void) 0)
#define VULCAL_PROFILE_PLOT(name, value)   ((void) 0)
#define VULCAL_PROFILE_THREAD(name)        ((void) 0)
#endif
