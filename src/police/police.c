//
// Created by yazan on 5/12/2025.
//

#include <stdlib.h>
#include <stdio.h>
#include <sys/mman.h>
#include <signal.h>
#include <unistd.h>  // For sleep()
#include "config.h"
#include "random.h"  // For random number generation

#include "shared_mem_utils.h"
Game *shared_game = NULL;
ShmPtrs shm_ptrs;

void cleanup();
void handle_sigint(int signum);

int main(int argc, char *argv[]) {

    atexit(cleanup);

    printf("Police process starting...\n");
    fflush(stdout);

    if(argc != 3) {
        fprintf(stderr, "Usage: %s <serialized_config> <id>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    
    // Load configuration from serialized string
    Config config;

    deserialize_config(argv[1], &config);

    signal(SIGINT, handle_sigint);

    // Initialize random number generator for this process
    init_random();
    printf("Police: Random number generator initialized\n");
    fflush(stdout);

    // Police process is a user of shared memory, not the owner
    shared_game = setup_shared_memory_user(&config, &shm_ptrs);
    shm_ptrs.shared_game = shared_game;
    
    printf("Police process connected to shared memory successfully\n");
    fflush(stdout);
    
    // Simple police main loop - just monitor for now
    while (1) {
        sleep(5); // Police monitors every 5 seconds
        printf("Police: Monitoring gang activities...\n");
        fflush(stdout);
        
        // TODO: Add police investigation logic here
    }
}


void handle_sigint(int signum) {
    // Cleanup resources
    exit(0);
}

void cleanup() {

    printf("cleaning up police\n");
    if (shared_game != NULL && shared_game != MAP_FAILED) {
        if (munmap(shared_game, sizeof(Game)) == -1) {
            perror("munmap failed");
        }
    }

}