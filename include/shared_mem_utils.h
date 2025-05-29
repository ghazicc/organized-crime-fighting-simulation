#ifndef SHARED_MEM_UTILS_H
#define SHARED_MEM_UTILS_H
#define GAME_SHM_NAME "/game_shared_mem"

#include "game.h"

#define SHARED_MEM "/game_shared_mem"

// For the main process (first to run) - creates and initializes shared memory
Game* setup_shared_memory_owner(Config *cfg, ShmPtrs *shm_ptrs);

// For secondary processes - only maps to existing shared memory
Game* setup_shared_memory_user(Config *cfg, ShmPtrs *shm_ptrs);

void cleanup_shared_memory(Game *shared_game);

#endif // SHARED_MEM_UTILS_H