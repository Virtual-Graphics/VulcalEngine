add_executable(vulcal_player
    main.cpp
    player_layer.cpp
    player_runtime.cpp
    ../common/application_paths.cpp
)
if(WIN32)
    set_property(TARGET vulcal_player PROPERTY WIN32_EXECUTABLE
        "$<OR:$<CONFIG:Release>,$<CONFIG:RelWithDebInfo>,$<CONFIG:MinSizeRel>>")
endif()
target_compile_definitions(vulcal_player PRIVATE
    "$<$<CONFIG:Debug>:VULCAL_PLAYER_DEFAULT_FILE_LOG>")
target_include_directories(vulcal_player PRIVATE "${PROJECT_SOURCE_DIR}/apps")
target_link_libraries(vulcal_player PRIVATE vulcal_engine)
