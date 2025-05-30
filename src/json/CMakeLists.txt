add_library(json-ting STATIC json-config.c)
target_link_libraries(json-ting PUBLIC JSON-C::JSON-C)
target_compile_definitions(json-ting INTERFACE JSON_PATH="${CMAKE_SOURCE_DIR}/config.json")