#ifndef SHARED_MEM_UTILS_H
#define SHARED_MEM_UTILS_H
#define GAME_SHM_NAME "/game_shared_mem"
#define GAME_SEM_NAME "/game_shared_sem"

#include "game.h"
#include "config.h"

// For the main process (first to run) - creates and initializes shared memory
Game* setup_shared_memory_owner(Config *cfg);

// For secondary processes - only maps to existing shared memory
Game* setup_shared_memory_user(Config *cfg);

void cleanup_shared_memory(Game *shared_game);

#endif // SHARED_MEM_UTILS_H