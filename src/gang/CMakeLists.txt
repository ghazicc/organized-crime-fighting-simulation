add_executable(gang gang.c actual_gang_member.c secret_agent.c success_rate.c
target_selection.c information_spreading.c)
target_link_libraries(gang PRIVATE utils)