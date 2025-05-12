# Define common/shared library for utilities
add_library(utils STATIC
        config.c
        semaphores_utils.c
        shared_mem_utils.c
        message_queue_utils.c
        random.c
)

target_include_directories(utils PUBLIC ${CMAKE_SOURCE_DIR}/include)