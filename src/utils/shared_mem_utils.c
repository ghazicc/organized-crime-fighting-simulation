#include "shared_mem_utils.h"
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <stdlib.h>
#include <stdio.h>
#include "gang.h"


void setup_shared_memory(Game *shared_game, Config *cfg) {
    // Allocate shared memory for Game + dynamic arrays
    size_t game_size = sizeof(Game);
    size_t gangs_size = cfg->max_gangs * sizeof(Gang);
    size_t members_size = cfg->max_gangs * cfg->max_gang_size * sizeof(Member);

    int shm_fd = shm_open("/crime_sim_shm", O_CREAT | O_RDWR, 0666);
    ftruncate(shm_fd, game_size + gangs_size + members_size);

    Game *game = mmap(NULL, game_size + gangs_size + members_size, 
                    PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);

    // Initialize pointers to dynamic parts
    game->gangs = (Gang*)((char*)game + sizeof(Game));
    game->total_gangs = 0;  // Updated as gangs spawn

    // Initialize each Gang's member array
    for (int i = 0; i < cfg->max_gangs; i++) {
        game->gangs[i].members = (Member*)((char*)game->gangs + gangs_size + 
                                        i * cfg->max_gang_size * sizeof(Member));
    }

}


void cleanup_shared_memory(Game *shared_game) {

}


