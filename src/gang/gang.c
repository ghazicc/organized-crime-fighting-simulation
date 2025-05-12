//
// Created by yazan on 5/12/2025.
//

#include <stdlib.h>
#include <stdio.h>
#include <sys/mman.h>
#include <signal.h>
#include "config.h"

#include "shared_mem_utils.h"
Game *shared_game;

void cleanup();
void handle_sigint(int signum);

int main(int argc, char *argv[]) {
    if(argc != 2) {
        fprintf(stderr, "Usage: %s <config_file>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    
    // Load configuration
    Config config;

    deserialize_config(argv[1], &config);


    atexit(cleanup);
    signal(SIGINT, handle_sigint);
    shared_game = setup_shared_memory(&config);
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