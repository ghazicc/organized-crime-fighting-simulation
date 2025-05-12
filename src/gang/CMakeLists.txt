add_executable(gang gang.c actual_gang_member.c secret_agent.c)
target_link_libraries(gang PRIVATE utils)