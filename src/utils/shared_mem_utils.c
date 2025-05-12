#include "shared_mem_utils.h"
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <stdlib.h>
#include <stdio.h>
#include "gang.h"


Game* setup_shared_memory(Config *cfg) {
    // Allocate shared memory for Game + dynamic arrays
    size_t game_size = sizeof(Game);
    size_t gangs_size = cfg->max_gangs * sizeof(Gang);
    size_t members_size = cfg->max_gangs * cfg->max_gang_size * sizeof(Member);

    int shm_fd = shm_open(GAME_SHM_NAME, O_CREAT | O_RDWR, 0666);
    ftruncate(shm_fd, game_size + gangs_size + members_size);

    Game *game = mmap(NULL, game_size + gangs_size + members_size, 
                    PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);

    if (game == MAP_FAILED) {
        perror("mmap");
        exit(EXIT_FAILURE);
    }

    return game;

}


void cleanup_shared_memory(Game *shared_game) {

}


