file(GLOB_RECURSE VULCAL_ENGINE_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
)

add_library(vulcal_engine STATIC ${VULCAL_ENGINE_SOURCES})

target_include_directories(vulcal_engine PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}"
)
target_compile_definitions(vulcal_engine PRIVATE
    VULCAL_RENDER_SHADER_DIR="${CMAKE_CURRENT_SOURCE_DIR}/render/shaders"
)
target_link_libraries(vulcal_engine PUBLIC
    SDL3::SDL3
    glad::glad
    spdlog::spdlog
    glm::glm
    tinyobjloader::tinyobjloader
    stb::stb_image
    EnTT::EnTT
)
