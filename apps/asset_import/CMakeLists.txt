add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/../../third_party/tinyobjloader" "${CMAKE_BINARY_DIR}/third_party/tinyobjloader")
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/../../third_party/stb" "${CMAKE_BINARY_DIR}/third_party/stb")

add_library(vulcal_asset_import STATIC meta_data.cpp obj_asset_importer.cpp runtime_mesh_asset_writer.cpp)

target_include_directories(vulcal_asset_import PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/..")
target_link_libraries(vulcal_asset_import
    PUBLIC
    vulcal_engine
    tinyobjloader::tinyobjloader
    stb::stb_image
    PRIVATE yaml-cpp::yaml-cpp
)
