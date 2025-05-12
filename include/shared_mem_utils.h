#ifndef SHARED_MEM_UTILS_H
#define SHARED_MEM_UTILS_H
#define GAME_SHM_NAME "/game_shared_mem"

#include "game.h"

#define SHARED_MEM "/game_shared_mem"


Game* setup_shared_memory(Config *cfg);
void cleanup_shared_memory(Game *shared_game);

#endif // SHARED_MEM_UTILS_H