add_executable(vulcal_player
    main.cpp
    player_layer.cpp
    player_runtime.cpp
    ../common/application_paths.cpp
)
target_include_directories(vulcal_player PRIVATE "${PROJECT_SOURCE_DIR}/apps")
target_link_libraries(vulcal_player PRIVATE vulcal_engine)
