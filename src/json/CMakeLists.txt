add_library(json-ting OBJECT json-config.c)
target_compile_definitions(json-ting INTERFACE JSON_PATH="${CMAKE_SOURCE_DIR}/config.json")