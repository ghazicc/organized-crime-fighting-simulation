add_executable(gang gang.c actual_gang_member.c secret_agent.c secret_agent_utils.c)
target_link_libraries(gang PRIVATE utils)