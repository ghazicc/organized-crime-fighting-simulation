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
#include "random.h"  // For random number generation

Game *shared_game = NULL;
ShmPtrs shm_ptrs;
Gang *gang;
Member *members; // Local pointer to this gang's members
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



    // validate gang ID - check against actual number of gangs, not max possible
    if (gang_id < 0 || gang_id >= config.num_gangs) {
        fprintf(stderr, "Invalid gang ID: %d (num_gangs: %d, max_gangs: %d)\n", gang_id, config.num_gangs, config.max_gangs);
        exit(EXIT_FAILURE);
    }
    
    printf("Gang %d: Validation passed (num_gangs: %d, max_gangs: %d)\n", gang_id, config.num_gangs, config.max_gangs);
    fflush(stdout);

    atexit(cleanup);
    signal(SIGINT, handle_sigint);

    // Initialize random number generator for this process
    init_random();
    printf("Gang %d: Random number generator initialized\n", gang_id);
    fflush(stdout);

    // Gang process is a user of shared memory, not the owner
    shared_game = setup_shared_memory_user(&config, &shm_ptrs);
    shm_ptrs.shared_game = shared_game;

    // Print the base address of shared memory for debugging
    printf("Gang %d: Shared memory mapped at %p\n", gang_id, (void*)shared_game);
    fflush(stdout);

    // Assign gang struct using ShmPtrs
    gang = &shm_ptrs.gangs[gang_id];
    
    // Set up local pointer to this gang's members
    members = shm_ptrs.gang_members[gang_id];
    
    printf("Gang %d: Gang struct at %p, Members array at %p\n", 
           gang_id, (void*)gang, (void*)members);
    fflush(stdout);

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

    printf("gang 0 member count %d\n", shm_ptrs.gangs[0].max_member_count);
    fflush(stdout);
    
    printf("Gang %d: About to initialize %d members\n", gang_id, gang->max_member_count);
    fflush(stdout);
    
    // Initialize gang members
    for(int i = 0; i < gang->max_member_count; i++) {
        printf("Gang %d: Initializing member %d basic properties\n", gang_id, i);
        fflush(stdout);
        
        members[i].gang_id = gang_id;
        members[i].member_id = i;
        members[i].rank = rand() % config.num_ranks;
        members[i].prep_contribution = 0;
        members[i].agent_id = -1;
        members[i].suspicion = 0.0f;
        members[i].is_alive = true;

        // Set up means and standard deviations for attributes
        float means[NUM_ATTRIBUTES] = {
            0.5f,  // ATTR_SMARTNESS - centered around 0.5
            0.5f,  // ATTR_STEALTH - centered around 0.5
            0.5f,  // ATTR_STRENGTH - centered around 0.5
            0.4f,  // ATTR_TECH_SKILLS - slightly lower mean
            0.6f,  // ATTR_BRAVERY - slightly higher mean
            0.5f,  // ATTR_NEGOTIATION - centered around 0.5
            0.5f   // ATTR_NETWORKING - centered around 0.5
        };
        
        float stddevs[NUM_ATTRIBUTES] = {
            0.15f,  // ATTR_SMARTNESS
            0.15f,  // ATTR_STEALTH
            0.15f,  // ATTR_STRENGTH
            0.20f,  // ATTR_TECH_SKILLS - more variance
            0.15f,  // ATTR_BRAVERY
            0.15f,  // ATTR_NEGOTIATION
            0.15f   // ATTR_NETWORKING
        };
        
        // Define correlation matrix between attributes
        // For example, smartness correlates with tech skills, strength with bravery, etc.
        float correlation_matrix[NUM_ATTRIBUTES][NUM_ATTRIBUTES] = {
            // SMARTNESS  STEALTH    STRENGTH   TECH       BRAVERY    NEGOTIATION NETWORKING
            {  0.2f,      0.0f,      0.0f,      0.0f,      0.0f,      0.0f,      0.0f  }, // SMARTNESS
            {  0.1f,      0.2f,      0.0f,      0.0f,      0.0f,      0.0f,      0.0f  }, // STEALTH
            {  0.0f,      0.0f,      0.2f,      0.0f,      0.0f,      0.0f,      0.0f  }, // STRENGTH
            {  0.15f,     0.1f,      0.0f,      0.2f,      0.0f,      0.0f,      0.0f  }, // TECH_SKILLS
            {  0.0f,      0.0f,      0.15f,     0.0f,      0.2f,      0.0f,      0.0f  }, // BRAVERY
            {  0.1f,      0.0f,      0.0f,      0.0f,      0.0f,      0.2f,      0.0f  }, // NEGOTIATION
            {  0.1f,      0.0f,      0.0f,      0.0f,      0.1f,      0.15f,     0.2f  }  // NETWORKING
        };
        
        // Generate attributes using multivariate Gaussian distribution
        printf("Gang %d: Generating correlated attributes for member %d\n", gang_id, i);
        fflush(stdout);
        generate_multivariate_attributes(members[i].attributes, means, stddevs, correlation_matrix);
        
        // Initialize member knowledge for information spreading
        initialize_member_knowledge(&members[i], members[i].rank, config.num_ranks - 1);
        
        printf("Gang %d: Member %d initialized successfully\n", gang_id, i);
        fflush(stdout);
    }
    
    // Initialize gang-level information spreading parameters
    gang->last_info_spread_time = 0;
    gang->info_spread_interval = random_int(3, 8); // Initial random interval
    gang->leader_misinformation_chance = random_float(0.05f, 0.20f); // 5-20% chance of misinformation
    
    printf("Gang %d: Information spreading initialized - interval: %d, leader misinformation chance: %.2f\n",
           gang_id, gang->info_spread_interval, gang->leader_misinformation_chance);
    fflush(stdout);
    
    // Find the member with the highest rank - but don't select target here
    // Target selection will happen in the highest-ranked member's thread
    highest_rank_member_id = find_highest_ranked_member(gang, members);
    if (highest_rank_member_id >= 0) {
        printf("Gang %d highest ranked member is member %d with rank %d\n", 
            gang_id, highest_rank_member_id, members[highest_rank_member_id].rank);
        fflush(stdout);
        
        // make the highest ranked member have the highest rank
        members[highest_rank_member_id].rank = config.num_ranks - 1;
        
        printf("Gang %d: Updated highest ranked member %d to rank %d\n", 
            gang_id, highest_rank_member_id, members[highest_rank_member_id].rank);
        fflush(stdout);
    } else {
        printf("Gang %d has no members to select a target\n", gang_id);
        fflush(stdout);
    }

    // Create threads for all gang members
    for(int i = 0; i < gang->max_member_count; i++) {
        // Pass the member from the local members array
        Member *thread_arg = &members[i];
        
        // Create thread for each member
        int ret;
        pthread_t thread_id;
        
        printf("Creating gang member thread %d\n", i);
        ret = pthread_create(&thread_id, NULL, actual_gang_member_thread_function, thread_arg);
        
        members[i].thread = thread_id;

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
    while (gang->members_ready < gang->num_alive_members) {
        printf("Gang %d: Main thread waiting for members to complete preparation (%d/%d ready)\n", 
               gang_id, gang->members_ready, gang->num_alive_members);
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
    gang->plan_success = determine_plan_success(gang, members, gang->target_type, &config) ? 1 : -1;
    
    printf("Gang %d: Plan %s! Main thread signaling all members\n", 
           gang_id, gang->plan_success == 1 ? "SUCCEEDED" : "FAILED");
    fflush(stdout);
    
    // Update gang statistics
    if (gang->plan_success == 1) {
        gang->num_successful_plans++;
        shm_ptrs.shared_game->num_successfull_plans++;
        printf("Gang %d: Successful plan completed! Total successful plans: %d/%d\n", 
               gang_id, shm_ptrs.shared_game->num_successfull_plans, config.max_successful_plans);
        printf("Gang %d: Notoriety increased after successful plan\n", gang_id);
        gang->notoriety += 0.1f;  // Increase notoriety on success
    } else {
        gang->num_thwarted_plans++;
        shm_ptrs.shared_game->num_thwarted_plans++;
        printf("Gang %d: Plan thwarted! Total thwarted plans: %d/%d\n", 
               gang_id, shm_ptrs.shared_game->num_thwarted_plans, config.max_thwarted_plans);
    }
    
    // Signal all waiting members about the plan execution result
    pthread_cond_broadcast(&gang->plan_execute_cond);
    pthread_mutex_unlock(&gang->gang_mutex);

    // Trigger information spreading after plan execution
    int current_time = shared_game->elapsed_time; // Use game time or implement time tracking
    printf("Gang %d: Triggering information spreading at time %d\n", gang_id, current_time);
    fflush(stdout);
    
    spread_information_in_gang(gang, members, current_time, highest_rank_member_id);
    
    // Wait for all threads to finish
    for(int i = 0; i < gang->max_member_count; i++) {
        pthread_join(members[i].thread, NULL);
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