cmake_minimum_required(VERSION 3.11.2)
project(organized_crime C CXX)
set(CMAKE_C_STANDARD 11)


option(BUILD_TESTING "Enable testing" OFF)

# Common include directories
include_directories(include)

message(STATUS "TESTING IS " ${BUILD_TESTING})

# Configuration path definition
add_compile_definitions(CONFIG_PATH="${CMAKE_CURRENT_SOURCE_DIR}/config.txt")

# Add subdirectories for components
add_subdirectory(src)

# Add tests only if BUILD_TESTING is enabled
if(BUILD_TESTING)
    include(cmake/tests.cmake)
    find_package(GTest REQUIRED)
    enable_testing()
    add_subdirectory(tests)
endif()