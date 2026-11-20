add_executable(vulcal_build main.cpp build_pipeline.cpp)
target_include_directories(vulcal_build PRIVATE "${PROJECT_SOURCE_DIR}/apps")
target_link_libraries(vulcal_build PRIVATE vulcal_engine vulcal::miniz)
