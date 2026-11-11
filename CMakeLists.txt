cmake_minimum_required(VERSION 3.28)
project(VulcalEngine VERSION 0.0.5 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

include(cmake/dependencies.cmake)

option(BUILD_CORE "Build the Engine Core" ON)
option(BUILD_APPS "Build the Engine Apps" ON)
option(BUILD_TESTS "Build the Engine Tests" ON)

if (BUILD_CORE)
    add_subdirectory(vulcal_engine)
endif()

if (BUILD_TESTS)
    enable_testing()
    include(cmake/third_party_googletest.cmake)
    add_subdirectory(tests)
endif()

if (BUILD_APPS)
    add_subdirectory(apps)
endif()
