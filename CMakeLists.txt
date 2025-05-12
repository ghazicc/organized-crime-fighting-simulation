cmake_minimum_required(VERSION 3.11.2)
project(organized_crime C)
set(CMAKE_C_STANDARD 99)

set(LIBRARY_DIR "${CMAKE_CURRENT_LIST_DIR}/lib")

# Add the path to your custom find modules
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")


add_executable(main src/main.c src/utils/config.c src/game.c src/utils/semaphores_utils.c
src/utils/shared_mem_utils.c)

add_executable(gang src/gang/gang.c src/utils/shared_mem_utils.c src/utils/config.c)
add_executable(police src/police/police.c src/utils/shared_mem_utils.c src/utils/config.c)
#add_executable(graphics src/graphics/graphics.c src/utils/shared_mem_utils.c)
#
#
#target_include_directories(graphics PRIVATE include/lib/raylib)
#target_link_libraries(graphics PRIVATE ${LIBRARY_DIR}/libraylib.a)
include_directories(include)



# Setting ASSETS_PATH

if (CMAKE_RELEASE_BUILD)
    message("Release build")
else()
    message("Debug build")
    target_compile_definitions(main PUBLIC CONFIG_PATH="${CMAKE_CURRENT_SOURCE_DIR}/config.txt") # Set the config path macro to the absolute path on the dev machine
endif()