
add_executable(graphics graphics.c animation.c assets.c)
target_link_libraries(graphics PRIVATE utils "${CMAKE_SOURCE_DIR}/lib/libraylib.a")
target_include_directories(graphics PRIVATE "${CMAKE_SOURCE_DIR}/include/lib/raylib")
target_compile_definitions(graphics PRIVATE ASSETS_PATH="${CMAKE_SOURCE_DIR}/assets/")




