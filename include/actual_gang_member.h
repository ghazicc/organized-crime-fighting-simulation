#ifndef ACTUAL_GANG_MEMBER_H
#define ACTUAL_GANG_MEMBER_H

#include "game.h"
#include "gang.h"
#include "target_selection.h"

// External variables needed by the thread function
extern Game *shared_game;
extern int highest_rank_member_id;

// Thread function for gang members
void* actual_gang_member_thread_function(void* arg);

#endif // ACTUAL_GANG_MEMBER_H