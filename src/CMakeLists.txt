# Define common/shared library for utilities
add_library(utils STATIC
        utils/config.c
        utils/semaphores_utils.c
        utils/shared_mem_utils.c
)
target_include_directories(utils PUBLIC ${CMAKE_SOURCE_DIR}/include)

# Main simulation executable
add_executable(main main.c game.c)
target_link_libraries(main PRIVATE utils)

# Add subdirectories for major components
add_subdirectory(gang)
add_subdirectory(police)
add_subdirectory(graphics)