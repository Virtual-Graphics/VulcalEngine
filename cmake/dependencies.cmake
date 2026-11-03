set(FETCHCONTENT_QUIET OFF CACHE BOOL "Show FetchContent download and population logs")

include(${CMAKE_CURRENT_LIST_DIR}/third_party_logging.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/third_party_sdl3.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/third_party_glm.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/third_party_entt.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/third_party_yamlcpp.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/third_party_nfd.cmake)

add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/../third_party/glad glad)
add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/../third_party/tinyobjloader tinyobjloader)
add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/../third_party/stb stb)
