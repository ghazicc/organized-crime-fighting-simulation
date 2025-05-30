#ifndef SECRET_AGENT_UTILS_H
#define SECRET_AGENT_UTILS_H

#include "gang.h"
#include "game.h"
#include "config.h"
#include <pthread.h>

// Function declarations for agent utilities
void secret_agent_init(Game* shared_game, Member* member);
void secret_agent_record_asker(Game* shared_game, Config config,Member* agent, int asker_id);
void secret_agent_ask_member(Game* shared_game, Member* agent, Member* target);
void conduct_internal_investigation(Config config, Game* shared_game, int gang_id);

void police_request_agent_knowledge(int police_msgid, Game* shared_game, int8_t gang_id, int8_t agent_id, Config config, Gang* gang);
void agent_report_knowledge(Member* agent, Game* shared_game, int police_msgid, int police_id, Gang* gang, Config config);
void secret_agent_handle_police_requests(Member* agent, Game* shared_game, int police_msgid, int police_id, Gang* gang, Config config);

#endif // SECRET_AGENT_UTILS_H
