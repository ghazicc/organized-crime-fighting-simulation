#ifndef SECRET_AGENT_UTILS_H
#define SECRET_AGENT_UTILS_H

#include "gang.h"
#include "game.h"
#include <pthread.h>

// List of members who asked the agent
typedef struct {
    int* member_ids;
    int count;
    int capacity;
    pthread_mutex_t mutex;
} AskedByList;

// Function declarations for agent utilities
void secret_agent_init(Member* member);
void secret_agent_ask_member(Member* agent, Member* target, Game* shared_game);
void secret_agent_handle_investigation(Member* agent, Member* investigator);
void secret_agent_communicate_with_police(Member* agent, Game* shared_game);
void secret_agent_record_asker(Member* agent, int member_id);

// AskedByList management functions
AskedByList* init_asked_by_lists(int max_agents);
void cleanup_asked_by_lists(AskedByList* lists, int max_agents);

// Print function for agent activities
void agent_print(Game* game, const char* format, ...);

#endif // SECRET_AGENT_UTILS_H