include(FetchContent)

FetchContent_Declare(
    tracy
    GIT_REPOSITORY https://github.com/wolfpld/tracy.git
    GIT_TAG        v0.15.0
    GIT_SHALLOW    TRUE
    GIT_PROGRESS   TRUE
)

set(TRACY_ENABLE ON CACHE BOOL "Enable profiling" FORCE)
set(TRACY_STATIC ON CACHE BOOL "Build the Tracy client as a static library" FORCE)
set(TRACY_ON_DEMAND ON CACHE BOOL "Record only while a Tracy profiler is connected")

FetchContent_MakeAvailable(tracy)
