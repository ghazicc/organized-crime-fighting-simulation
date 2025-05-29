add_executable(police police.c police_officer.c)
target_link_libraries(police PRIVATE utils)