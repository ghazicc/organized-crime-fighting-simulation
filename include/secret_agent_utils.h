#ifndef SECRET_AGENT_UTILS_H
#define SECRET_AGENT_UTILS_H

#include "gang.h"
#include "game.h"
#include <pthread.h>



// Function declarations for agent utilities
void secret_agent_init(Member* member);
void secret_agent_ask_member(Member* agent, Member* target, Game* shared_game);
void conduct_internal_investigation(Config config, Game* shared_game, int gang_d);
void secret_agent_communicate_with_police(Member* agent, Game* shared_game);
void secret_agent_record_asker(Member* agent, int member_id);

void init_asked_by_arrays(int max_members);
void cleanup_asked_by_arrays();

// Print function for agent activities
void agent_print(Game* game, const char* format, ...);

#endif // SECRET_AGENT_UTILS_H