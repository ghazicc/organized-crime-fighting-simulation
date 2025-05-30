add_executable(gang gang.c actual_gang_member.c success_rate.c
target_selection.c secret_agent_utils.c
information_spreading.c)
target_link_libraries(gang PRIVATE utils)
