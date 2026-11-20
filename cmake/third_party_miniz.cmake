include(FetchContent)
include(GenerateExportHeader)

block()
    if (POLICY CMP0169)
        cmake_policy(SET CMP0169 OLD)
    endif()

    FetchContent_Declare(
        miniz
        GIT_REPOSITORY https://github.com/richgel999/miniz.git
        GIT_TAG        3.1.1
        GIT_SHALLOW    TRUE
        GIT_PROGRESS   TRUE
    )

    FetchContent_GetProperties(miniz)
    if (NOT miniz_POPULATED)
        FetchContent_Populate(miniz)
    endif()
    set(VULCAL_MINIZ_SOURCE_DIR "${miniz_SOURCE_DIR}" PARENT_SCOPE)
endblock()

add_library(vulcal_miniz STATIC
    "${VULCAL_MINIZ_SOURCE_DIR}/miniz.c"
    "${VULCAL_MINIZ_SOURCE_DIR}/miniz_zip.c"
    "${VULCAL_MINIZ_SOURCE_DIR}/miniz_tinfl.c"
    "${VULCAL_MINIZ_SOURCE_DIR}/miniz_tdef.c"
)
set_target_properties(vulcal_miniz PROPERTIES C_STANDARD 90 C_STANDARD_REQUIRED ON)
if (CMAKE_C_COMPILER_ID STREQUAL "GNU")
    target_compile_definitions(vulcal_miniz PRIVATE _GNU_SOURCE)
endif()

set(miniz_generated_dir "${CMAKE_CURRENT_BINARY_DIR}/generated/miniz")
file(MAKE_DIRECTORY "${miniz_generated_dir}")
generate_export_header(vulcal_miniz
    EXPORT_FILE_NAME "${miniz_generated_dir}/miniz_export.h"
    EXPORT_MACRO_NAME MINIZ_EXPORT
)
target_include_directories(vulcal_miniz PUBLIC "${VULCAL_MINIZ_SOURCE_DIR}" "${miniz_generated_dir}")
add_library(vulcal::miniz ALIAS vulcal_miniz)
