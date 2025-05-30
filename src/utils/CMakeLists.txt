# Define common/shared library for utilities
add_library(utils SHARED
        config.c
        semaphores_utils.c
        shared_mem_utils.c
        message_queue_utils.c
        random.c
)

# Use generator expressions for paths to other executables
target_compile_definitions(utils PUBLIC
        GANG_EXECUTABLE="$<TARGET_FILE:gang>"
        POLICE_EXECUTABLE="$<TARGET_FILE:police>"
        CONFIG_PATH="${CMAKE_SOURCE_DIR}/config.txt"
        GRAPHICS_EXECUTABLE="$<TARGET_FILE:graphics>"
)

target_include_directories(utils PUBLIC ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(utils PUBLIC pthread rt m)