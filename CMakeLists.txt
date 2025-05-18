cmake_minimum_required(VERSION 3.11.2)
project(organized_crime C CXX)
set(CMAKE_C_STANDARD 11)

include(cmake/FindJSON-C.cmake)
# Use the standard package finding mechanism
find_package(JSON-C REQUIRED)

option(BUILD_TESTING "Enable testing" OFF)

message(STATUS "TESTING IS " ${BUILD_TESTING})

include_directories(include)

# Add subdirectories for components
add_subdirectory(src)

# Add tests only if BUILD_TESTING is enabled
if(BUILD_TESTING)
    include(cmake/tests.cmake)
    find_package(GTest REQUIRED)
    enable_testing()
    add_subdirectory(tests)
endif()