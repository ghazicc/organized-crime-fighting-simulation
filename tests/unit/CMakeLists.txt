create_test(test_config)

target_sources(test_config PRIVATE ${CMAKE_SOURCE_DIR}/src/utils/config.c)

create_test(test_json)
target_link_libraries(test_json PRIVATE json-ting utils)