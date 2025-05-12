//
// Created by yazan on 5/12/2025.
//

#include <stdlib.h>
#include <stdio.h>
#include <sys/mman.h>
#include <signal.h>

#include "shared_mem_utils.h"
Game *shared_game;

void cleanup();
void handle_sigint(int signum);

int main() {

    atexit(cleanup);
    signal(SIGINT, handle_sigint);
    setup_shared_memory(&shared_game);
}


void handle_sigint(int signum) {
    // Cleanup resources
    exit(0);
}

void cleanup() {

    printf("cleaning up gang");
    if (shared_game != NULL && shared_game != MAP_FAILED) {
        if (munmap(shared_game, sizeof(Game)) == -1) {
            perror("munmap failed");
        }
    }

}