
add_subdirectory(utils)

# Main simulation executable
add_executable(main main.c game.c)
target_link_libraries(main PRIVATE utils)

# Use generator expressions for paths to other executables
target_compile_definitions(main PRIVATE
        GANG_EXECUTABLE="$<TARGET_FILE:gang>"
        POLICE_EXECUTABLE="$<TARGET_FILE:police>"
#        GRAPHICS_EXECUTABLE="$<TARGET_FILE:graphics>"
)

# Add subdirectories for major components
add_subdirectory(gang)
add_subdirectory(police)
add_subdirectory(graphics)