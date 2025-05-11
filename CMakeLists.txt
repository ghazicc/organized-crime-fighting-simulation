cmake_minimum_required(VERSION 3.11.2)
project(my_raylib_game C)
set(CMAKE_C_STANDARD 99)

set(LIBRARY_DIR "${CMAKE_CURRENT_LIST_DIR}/lib")

# Add the path to your custom find modules
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")


add_executable(main src/main.c src/utils/config.c src/game.c src/inventory.c src/graphics/assets.c
    src/utils/products_utils.c src/utils/json-config.c src/utils/semaphores_utils.c
        src/utils/random.c  # Add this line
        src/utils/shared_mem_utils.c src/customers/customer_utils.c)



# Setting ASSETS_PATH

if (CMAKE_RELEASE_BUILD)
    message("Release build")
    target_compile_definitions(graphics PUBLIC ASSETS_PATH="./assets") # Set the asset path macro in release mode to a relative path that assumes the assets folder is in the same directory as the game executable
else()
    message("Debug build")
    target_compile_definitions(graphics PUBLIC ASSETS_PATH="${CMAKE_CURRENT_SOURCE_DIR}/assets/") # Set the asset path macro to the absolute path on the dev machine
    target_compile_definitions(main PUBLIC CONFIG_PATH="${CMAKE_CURRENT_SOURCE_DIR}/config.txt") # Set the config path macro to the absolute path on the dev machine
endif()
ts)