include(FetchContent)

FetchContent_Declare(
    SDL3
    GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
    GIT_TAG        release-3.2.22
    GIT_PROGRESS   TRUE
    GIT_SHALLOW    TRUE
    FIND_PACKAGE_ARGS 
)

# FIND_PACKAGE_ARGS lets development builds reuse a system SDL3. The static
# options below only apply when SDL3 is built from source (VULCAL_STATIC_DEPS),
# so they are forced to keep stale cache entries from flipping the linkage.
set(SDL_SHARED OFF CACHE BOOL "Build a shared version of SDL3" FORCE)
set(SDL_STATIC ON CACHE BOOL "Build a static version of SDL3" FORCE)

FetchContent_MakeAvailable(SDL3)
