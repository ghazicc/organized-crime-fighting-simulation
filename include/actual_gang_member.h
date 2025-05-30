#ifndef ACTUAL_GANG_MEMBER_H
#define ACTUAL_GANG_MEMBER_H

#include "game.h"
#include "config.h"

// Structure to pass both member and config to thread function
typedef struct {
    Member* member;
    Config* config;
} ThreadArgs;

// External variables needed by the thread function
extern Game *shared_game;
extern int highest_rank_member_id;

// Thread function for gang members
void* actual_gang_member_thread_function(void* arg);

#endif // ACTUAL_GANG_MEMBER_H