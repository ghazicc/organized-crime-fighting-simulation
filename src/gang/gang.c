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
#include "actual_gang_member.h"
#include "secret_agent.h"

#include "shared_mem_utils.h"
Game *shared_game = NULL;
Gang *gang;

void cleanup();
void handle_sigint(int signum);

int main(int argc, char *argv[]) {
    printf("Gang process starting...\n");
    fflush(stdout);

    if(argc != 3) {
        fprintf(stderr, "Usage: %s <serialized_config> <gang_id>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    
    // Load configuration from serialized string
    Config config;

    // Deserialize the config from the provided string
    deserialize_config(argv[1], &config);

    int gang_id = atoi(argv[2]);



    // validate gang ID
    if (gang_id < 0 || gang_id >= config.max_gangs) {
        fprintf(stderr, "Invalid gang ID: %d\n", gang_id);
        exit(EXIT_FAILURE);
    }

    atexit(cleanup);
    signal(SIGINT, handle_sigint);

    // Gang process is a user of shared memory, not the owner
    shared_game = setup_shared_memory_user(&config);

    // Print the base address of shared memory for debugging
    printf("Gang %d: Shared memory mapped at %p\n", gang_id, (void*)shared_game);
    fflush(stdout);

    // Assign gang struct
    gang = &shared_game->gangs[gang_id];

    // Update gang_id in shared memory
    gang->gang_id = gang_id;

    // Initialize members_count if needed
    if (gang->members_count == 0) {
        gang->members_count = config.max_gang_size;
    }


    printf("Gang %d process started...\n", gang_id);
    fflush(stdout);


    // Initialize gang members and create threads for them
    for(int i = 0; i < config.max_gang_size; i++) {
        gang->members[i].gang_id = gang_id;
        gang->members[i].member_id = i;
        gang->members[i].rank = rand() % config.num_ranks;
        gang->members[i].prep_contribution = 0;
        gang->members[i].agent_id = -1;
        gang->members[i].suspicion = 0.0f;

        // CRITICAL: Allocate memory for thread argument to avoid data race
        Member *thread_arg = malloc(sizeof(Member));
        if (thread_arg == NULL) {
            fprintf(stderr, "Failed to allocate memory for thread argument\n");
            exit(EXIT_FAILURE);
        }

        // Copy the member data to the allocated memory
        *thread_arg = gang->members[i];

        // Create thread for each member
        int ret;
        pthread_t thread_id;
        
        printf("Creating gang member thread %d\n", i);
        ret = pthread_create(&thread_id, NULL, actual_gang_member_thread_function, thread_arg);
        

        gang->members[i].thread = thread_id;

        if (ret != 0) {
            fprintf(stderr, "Failed to create thread: %d\n", ret);
        }
        fflush(stdout); // Make sure we see the debug output

    }

    // Wait for all threads to finish
    for(int i = 0; i < config.max_gang_size; i++) {
        pthread_join(gang->members[i].thread, NULL);
    }

    // Cleanup resources
    cleanup();


}


void handle_sigint(int signum) {
    // Cleanup resources
    exit(0);
}

void cleanup() {

    printf("cleaning up gang\n");
    if (shared_game != NULL && shared_game != MAP_FAILED) {
        if (munmap(shared_game, sizeof(Game)) == -1) {
            perror("munmap failed");
        }
    }

}