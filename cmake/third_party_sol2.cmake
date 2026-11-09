include(FetchContent)

FetchContent_Declare(
    sol2
    GIT_REPOSITORY https://github.com/ThePhD/sol2.git
    GIT_TAG        v3.5.0
    GIT_SHALLOW    TRUE
    GIT_PROGRESS   TRUE
)

set(SOL2_BUILD_LUA                OFF CACHE BOOL "" FORCE)
set(SOL2_TESTS                    OFF CACHE BOOL "" FORCE)
set(SOL2_EXAMPLES                 OFF CACHE BOOL "" FORCE)
set(SOL2_INTEROP_EXAMPLES         OFF CACHE BOOL "" FORCE)
set(SOL2_DYNAMIC_LOADING_EXAMPLES OFF CACHE BOOL "" FORCE)
set(SOL2_SINGLE                   OFF CACHE BOOL "" FORCE)
set(SOL2_DOCS                     OFF CACHE BOOL "" FORCE)
set(SOL2_ENABLE_INSTALL           OFF CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(sol2)

# Upstream Lua supplies C headers only; do not pick up an unrelated system lua.hpp.
target_compile_definitions(sol2 INTERFACE SOL_NO_LUA_HPP=1)
