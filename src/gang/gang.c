//
// Created by yazan on 5/12/2025.
//

#include <stdlib.h>
#include <stdio.h>
#include <sys/mman.h>
#include <signal.h>
#include <pthread.h>
#include "config.h"
#include "game.h"
#include "gang.h"

#include "shared_mem_utils.h"
Game *shared_game;
Gang *gang;

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

    int gang_id = atoi(argv[2]);
    

    // validate gang ID
    if (gang < 0 || gang >= config.max_gangs) {
        fprintf(stderr, "Invalid gang ID: %d\n", gang);
        exit(EXIT_FAILURE);
    }

    atexit(cleanup);
    signal(SIGINT, handle_sigint);
    shared_game = setup_shared_memory(&config);


    if (shared_game == NULL) {
        fprintf(stderr, "Failed to set up shared memory\n");
        exit(EXIT_FAILURE);
    }
    
    // assign gang struct
    gang = &shared_game->gangs[gang_id];
    gang->gang_id = gang_id;

    // Initialize gang members and create threads for them
    


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