# Define common/shared library for utilities
add_library(utils SHARED
        config.c
        semaphores_utils.c
        shared_mem_utils.c
        message_queue_utils.c
        random.c
)

target_include_directories(utils PUBLIC ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(utils PUBLIC pthread rt m)