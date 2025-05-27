#include "secret_agent_utils.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include "config.h"
#include "game.h"
#include "gang.h"
#include "shared_mem_utils.h"

void secret_agent_init(Game* shared_game, Member* member) {
    // Find the actual member in shared memory
    Member* shared_member = &shared_game->gangs[member->gang_id].members[member->member_id];

    // Initialize attributes in shared memory
    shared_member->knowledge = 0.0f;
    shared_member->suspicion = 0.0f;
    shared_member->faithfulness = (float)rand() / RAND_MAX; // Random value between 0 and 1
    shared_member->discretion = 1.0f + ((float)rand() / RAND_MAX); // Random value between 1 and 2
    shared_member->shrewdness = 1.0f + ((float)rand() / RAND_MAX); // Random value between 1 and 2
    shared_member->askers_count = 0;
}







