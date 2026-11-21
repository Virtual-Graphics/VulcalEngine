set(FETCHCONTENT_QUIET OFF CACHE BOOL "Show FetchContent download and population logs")

# Distributable builds must not pick up system packages, otherwise the shipped
# binaries need libSDL3/libspdlog/libfmt installed on the target machine even
# though the dependencies are also vendored below.
if (VULCAL_STATIC_DEPS)
    set(FETCHCONTENT_TRY_FIND_PACKAGE_MODE NEVER)
endif()

include(${CMAKE_CURRENT_LIST_DIR}/third_party_logging.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/third_party_sdl3.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/third_party_glm.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/third_party_entt.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/third_party_lua.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/third_party_sol2.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/third_party_yamlcpp.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/third_party_miniz.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/third_party_nfd.cmake)

add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/../third_party/glad glad)

if (VULCAL_ENABLE_PROFILING)
    include(${CMAKE_CURRENT_LIST_DIR}/third_party_tracy.cmake)
endif()
