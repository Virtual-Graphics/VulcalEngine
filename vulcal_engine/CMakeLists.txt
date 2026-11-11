file(GLOB_RECURSE VULCAL_ENGINE_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
)

# Shader edits must regenerate the embedded sources during a normal build.
foreach(shader IN ITEMS
    blinn_phong.vert blinn_phong.frag
    shadow.vert shadow.frag
    sprite.vert sprite.frag
)
    set(shader_path "${CMAKE_CURRENT_SOURCE_DIR}/render/shaders/${shader}")
    set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${shader_path}")
    string(MAKE_C_IDENTIFIER "${shader}" shader_identifier)
    string(TOUPPER "${shader_identifier}" shader_variable)
    file(READ "${shader_path}" "${shader_variable}")
endforeach()

set(embedded_shaders_header "${CMAKE_CURRENT_BINARY_DIR}/generated/render/shaders/embedded_shaders.h")
configure_file(render/shaders/embedded_shaders.h.in "${embedded_shaders_header}" @ONLY NEWLINE_STYLE LF)

add_library(vulcal_engine STATIC ${VULCAL_ENGINE_SOURCES} "${embedded_shaders_header}")

target_include_directories(vulcal_engine PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}"
    PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/generated"
)
target_link_libraries(vulcal_engine PUBLIC
    SDL3::SDL3
    glad::glad
    spdlog::spdlog
    glm::glm
    EnTT::EnTT
    yaml-cpp::yaml-cpp
    PRIVATE
    Lua::Lua
    sol2::sol2
)
