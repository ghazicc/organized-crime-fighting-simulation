#ifndef SECRET_AGENT_UTILS_H
#define SECRET_AGENT_UTILS_H

#include "gang.h"
#include "game.h"
#include <pthread.h>

// Function declarations for agent utilities
void secret_agent_init(Member* member);
void secret_agent_ask_member(Member* agent, Member* target, Game* shared_game);
void conduct_internal_investigation(Config config, Game* shared_game, int gang_id);
void secret_agent_communicate_with_police(Member* agent, Game* shared_game);
void secret_agent_record_asker(Member* agent, int member_id);

void init_asked_by_arrays(int max_members);
void cleanup_asked_by_arrays();

void police_request_agent_knowledge(int police_msgid, Game* shared_game,Config config, int8_t gang_id, int8_t agent_id, Gang* gang);
void agent_report_knowledge(Member* agent, Game* shared_game,Config config, int police_msgid, int police_id,Gang* gang);
void secret_agent_handle_police_requests(Member* agent, Game* shared_game,Config config, int police_msgid, int police_id,Gang* gang);
// Print function for agent activities
void agent_print(Game* game, const char* format, ...);

#endif // SECRET_AGENT_UTILS_H

