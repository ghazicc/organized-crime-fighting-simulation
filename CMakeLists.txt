cmake_minimum_required(VERSION 3.11.2)
project(organized_crime C CXX)
set(CMAKE_C_STANDARD 11)

# Option to enable/disable tests (ON by default)
option(BUILD_TESTING "Build tests" ON)

# Common include directories
include_directories(include)

# Configuration path definition
if (CMAKE_RELEASE_BUILD)
    message("Release build")
    add_compile_definitions(CONFIG_PATH="${CMAKE_CURRENT_SOURCE_DIR}/config.txt")
else()
    message("Debug build")
    add_compile_definitions(CONFIG_PATH="${CMAKE_CURRENT_SOURCE_DIR}/config.txt")
endif()

# Add subdirectories for components
add_subdirectory(src)

# Add tests only if BUILD_TESTING is enabled
if(BUILD_TESTING)
    include(cmake/tests.cmake)
    find_package(GTest REQUIRED)
    enable_testing()
    add_subdirectory(tests)
endif()