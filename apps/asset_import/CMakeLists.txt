add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/../../third_party/tinyobjloader" "${CMAKE_BINARY_DIR}/third_party/tinyobjloader")
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/../../third_party/stb" "${CMAKE_BINARY_DIR}/third_party/stb")

add_library(vulcal_asset_pipeline STATIC meta_data.cpp asset_importer.cpp importers/obj_asset_importer.cpp importers/material_asset_importer.cpp)

target_include_directories(vulcal_asset_pipeline PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/..")
target_link_libraries(vulcal_asset_pipeline
    PUBLIC
    vulcal_engine
    tinyobjloader::tinyobjloader
    stb::stb_image
    PRIVATE yaml-cpp::yaml-cpp
)
