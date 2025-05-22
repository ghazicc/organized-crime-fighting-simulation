//
// Created by yazan on 5/12/2025.
//

#include <stdlib.h>
#include <stdio.h>
#include <sys/mman.h>
#include <signal.h>
#include <pthread.h>
#include <unistd.h>  // For sleep()
#include "config.h"
#include "game.h"
#include "gang.h"
#include "actual_gang_member.h"
#include "secret_agent.h"
#include "target_selection.h"
#include "success_rate.h"  // For success rate calculation
#include "shared_mem_utils.h"

Game *shared_game = NULL;
Gang *gang;
int highest_rank_member_id = -1;

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

    // Initialize synchronization primitives
    pthread_mutex_init(&gang->gang_mutex, NULL);
    pthread_cond_init(&gang->prep_complete_cond, NULL); // once all members are ready
    pthread_cond_init(&gang->plan_execute_cond, NULL); // gang main thread computes success rate
    
    // Initialize plan status variables
    gang->members_ready = 0;
    gang->plan_success = 0; // 0 = not determined
    gang->plan_in_progress = 0;


    printf("Gang %d process started...\n", gang_id);
    fflush(stdout);
    
    // Initialize gang members
    for(int i = 0; i < gang->max_member_count; i++) {
        gang->members[i].gang_id = gang_id;
        gang->members[i].member_id = i;
        gang->members[i].rank = rand() % config.num_ranks;
        gang->members[i].prep_contribution = 0;
        gang->members[i].agent_id = -1;
        gang->members[i].suspicion = 0.0f;
        gang->members[i].is_alive = true;

        // Initialize attributes
        for (int j = 0; j < NUM_ATTRIBUTES; j++) {
            gang->members[i].attributes[j] = (rand() % 100)/100.0; // Random values for attributes
        }
    }
    
    // Find the member with the highest rank - but don't select target here
    // Target selection will happen in the highest-ranked member's thread
    highest_rank_member_id = find_highest_ranked_member(gang);
    if (highest_rank_member_id >= 0) {
        printf("Gang %d highest ranked member is member %d with rank %d\n", 
            gang_id, highest_rank_member_id, gang->members[highest_rank_member_id].rank);
        fflush(stdout);
        
        // make the highest ranked member have the highest rank
        gang->members[highest_rank_member_id].rank = config.num_ranks - 1;
    } else {
        printf("Gang %d has no members to select a target\n", gang_id);
        fflush(stdout);
    }

    // Create threads for all gang members
    for(int i = 0; i < gang->max_member_count; i++) {
        // CRITICAL: Allocate memory for thread argument to avoid data race
        Member *thread_arg = &gang->members[i];
        if (thread_arg == NULL) {
            fprintf(stderr, "Failed to allocate memory for thread argument\n");
            exit(EXIT_FAILURE);
        }

        // Copy the member data to the allocated memory
        // *thread_arg = gang->members[i];

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



    // Main thread waits for preparation and determines plan success
    printf("Gang %d: Main thread waiting for members to complete preparation\n", gang_id);
    fflush(stdout);
    
    // Use condition variable to wait for all members to be ready
    pthread_mutex_lock(&gang->gang_mutex);
    
    // Wait until all members are ready
    while (gang->members_ready < gang->max_member_count) {
        printf("Gang %d: Main thread waiting for members to complete preparation (%d/%d ready)\n", 
               gang_id, gang->members_ready, gang->max_member_count);
        fflush(stdout);
        
        // Wait for the condition that all members are ready
        // This releases the mutex while waiting and reacquires it when signaled
        pthread_cond_wait(&gang->prep_complete_cond, &gang->gang_mutex);
        
        // When pthread_cond_wait returns, we own the mutex again
        // The loop will check the condition again
    }
    
    // At this point we have the mutex locked and all members are ready
    printf("Gang %d: All members ready (%d/%d). Proceeding to calculate success rate.\n", 
           gang_id, gang->members_ready, gang->max_member_count);
    fflush(stdout);
    
    // All members are ready, calculate if the plan succeeds
    printf("Gang %d: Main thread detected all members ready - calculating success rate\n", gang_id);
    fflush(stdout);
    
    // Calculate if the plan succeeds
    gang->plan_success = determine_plan_success(gang, gang->target_type, &config) ? 1 : -1;
    
    printf("Gang %d: Plan %s! Main thread signaling all members\n", 
           gang_id, gang->plan_success == 1 ? "SUCCEEDED" : "FAILED");
    fflush(stdout);
    
    // Update gang statistics
    if (gang->plan_success == 1) {
        gang->num_successful_plans++;
        shared_game->num_successfull_plans++;
        printf("Gang %d: Notoriety increased after successful plan\n", gang_id);
        gang->notoriety += 0.1f;  // Increase notoriety on success
    } else {
        gang->num_thwarted_plans++;
        shared_game->num_thwarted_plans++;
    }
    
    // Signal all waiting members about the plan execution result
    pthread_cond_broadcast(&gang->plan_execute_cond);
    pthread_mutex_unlock(&gang->gang_mutex);

    // Wait for all threads to finish
    for(int i = 0; i < gang->max_member_count; i++) {
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

void gang_init() {

}