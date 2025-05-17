add_subdirectory(utils)
add_subdirectory(json)
# Main simulation executable
add_executable(main main.c game.c)
target_link_libraries(main PRIVATE utils JSON-C::JSON-C)
# Use generator expressions for paths to other executables
target_compile_definitions(main PRIVATE
        GANG_EXECUTABLE="$<TARGET_FILE:gang>"
        POLICE_EXECUTABLE="$<TARGET_FILE:police>"
        CONFIG_PATH="${CMAKE_SOURCE_DIR}/config.txt"
#        GRAPHICS_EXECUTABLE="$<TARGET_FILE:graphics>"
)

target_link_libraries(main PRIVATE json-ting)

# Add subdirectories for major components
add_subdirectory(gang)
add_subdirectory(police)
add_subdirectory(graphics)