#
#add_executable(graphics
#    graphics.c
#    animation.c          # إذا استعملته
#    ../utils/shared_mem_utils.c
#    ../utils/semaphores_utils.c
#)
#
#target_include_directories(graphics
#    PRIVATE
#        ${CMAKE_SOURCE_DIR}/include
#        ${CMAKE_SOURCE_DIR}/include/lib/raylib
#)
#
#list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")
#find_package(raylib 5 CONFIG QUIET)
#
#if(raylib_FOUND)
#    message(STATUS "► graphics: linking against system raylib")
#    target_link_libraries(graphics PRIVATE raylib)
#else()
#    message(STATUS "► graphics: linking against local lib/libraylib.a")
#    target_link_libraries(graphics PRIVATE
#        ${CMAKE_SOURCE_DIR}/lib/libraylib.a
#        m dl pthread rt
#    )
#    if(WIN32)
#        target_link_libraries(graphics PRIVATE opengl32 gdi32 winmm)
#    endif()
#endif()
#
#
#if(TARGET utils)
#    target_link_libraries(graphics PRIVATE utils)
#endif()
#
#if(CMAKE_BUILD_TYPE STREQUAL "Release")
#    target_compile_definitions(graphics PRIVATE ASSETS_PATH="./assets/")
#else()
#    target_compile_definitions(graphics PRIVATE
#        ASSETS_PATH="${CMAKE_SOURCE_DIR}/assets/")
#endif()
#
#
#get_target_property(_incs graphics INCLUDE_DIRECTORIES)
#message(STATUS "► graphics INCLUDE_DIRS = ${_incs}")


add_executable(graphics graphics.c animation.c assets.c)
target_link_libraries(graphics PRIVATE utils "${CMAKE_SOURCE_DIR}/lib/libraylib.a")
target_include_directories(graphics PRIVATE "${CMAKE_SOURCE_DIR}/include/lib/raylib")
target_compile_definitions(graphics PRIVATE ASSETS_PATH="${CMAKE_SOURCE_DIR}/assets")

