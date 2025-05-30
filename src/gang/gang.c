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
#include "target_selection.h"
#include "success_rate.h"  // For success rate calculation
#include "shared_mem_utils.h"
#include "semaphores_utils.h"
#include "random.h"  // For random number generation
#include "message.h"  // For message queue communication
#include "secret_agent_utils.h"  // For secret agent functions

Game *shared_game = NULL;
ShmPtrs shm_ptrs;
Gang *gang;
Member *members; // Local pointer to this gang's members
int highest_rank_member_id = -1;
volatile int should_terminate = 0; // Flag for clean termination

void cleanup();
void handle_sigint(int signum);
void handle_police_handshake(int gang_id, const Config* config);

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

    // Initialize semaphores for this process
    if (init_semaphores() != 0) {
        fprintf(stderr, "Gang %d: Failed to initialize semaphores\n", gang_id);
        exit(EXIT_FAILURE);
    }

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

    // Set up message queue for police communication
    police_msgq_id = create_message_queue(POLICE_GANG_KEY);
    if (police_msgq_id == -1) {
        fprintf(stderr, "Gang %d: Failed to create/access message queue\n", gang_id);
        exit(EXIT_FAILURE);
    }
    printf("Gang %d: Message queue initialized (ID: %d)\n", gang_id, police_msgq_id);
    fflush(stdout);

    // Initialize synchronization primitives
    pthread_mutex_init(&gang->gang_mutex, NULL);
    pthread_cond_init(&gang->prep_complete_cond, NULL); // once all members are ready
    pthread_cond_init(&gang->plan_execute_cond, NULL); // gang main thread computes success rate
    
    // Initialize plan status variables
    gang->members_ready = 0;
    gang->plan_success = 0; // 0 = not determined
    gang->plan_in_progress = 1; // Start with first plan in progress


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
    ThreadArgs* thread_args = malloc(gang->max_member_count * sizeof(ThreadArgs));
    for(int i = 0; i < gang->max_member_count; i++) {
        // Set up thread arguments with both member and config
        thread_args[i].member = &members[i];
        thread_args[i].config = &config;
        
        // Create thread for each member
        int ret;
        pthread_t thread_id;
        
        printf("Creating gang member thread %d\n", i);
        ret = pthread_create(&thread_id, NULL, actual_gang_member_thread_function, &thread_args[i]);
        
        members[i].thread = thread_id;

        if (ret != 0) {
            fprintf(stderr, "Failed to create thread: %d\n", ret);
        }
        fflush(stdout); // Make sure we see the debug output
    }



    // Main gang loop - execute multiple plans
    printf("Gang %d: Starting main gang loop for multiple plans\n", gang_id);
    fflush(stdout);
    
    while (!should_terminate) {
        // Handle police handshake messages for agent planting
        handle_police_handshake(gang_id, &config);
        // Reset for next plan
        pthread_mutex_lock(&gang->gang_mutex);
        gang->members_ready = 0;
        gang->plan_success = 0;
        gang->plan_in_progress = 1;
        
        // Reset preparation levels for new plan
        reset_preparation_levels(gang, members);
        printf("Gang %d: Starting new plan preparation\n", gang_id);
        fflush(stdout);
        pthread_mutex_unlock(&gang->gang_mutex);
        
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
        
        // Update gang statistics with semaphore protection
        if (gang->plan_success == 1) {
            // Protect local gang statistics with gang stats semaphore
            LOCK_GANG_STATS();
            gang->num_successful_plans++;
            gang->notoriety += 0.1f;  // Increase notoriety on success
            UNLOCK_GANG_STATS();
            
            // Protect shared game statistics with game stats semaphore
            LOCK_GAME_STATS();
            shm_ptrs.shared_game->num_successfull_plans++;
            int total_successful = shm_ptrs.shared_game->num_successfull_plans;
            UNLOCK_GAME_STATS();
            
            printf("Gang %d: Successful plan completed! Total successful plans: %d/%d\n", 
                   gang_id, total_successful, config.max_successful_plans);
            printf("Gang %d: Notoriety increased after successful plan\n", gang_id);
        } else {
            // Protect local gang statistics with gang stats semaphore
            LOCK_GANG_STATS();
            gang->num_thwarted_plans++;
            UNLOCK_GANG_STATS();
            
            // Protect shared game statistics with game stats semaphore
            LOCK_GAME_STATS();
            shm_ptrs.shared_game->num_thwarted_plans++;
            int total_thwarted = shm_ptrs.shared_game->num_thwarted_plans;
            UNLOCK_GAME_STATS();
            
            printf("Gang %d: Plan thwarted! Total thwarted plans: %d/%d\n", 
                   gang_id, total_thwarted, config.max_thwarted_plans);
            
            // Trigger internal investigation after thwarted plan
            printf("Gang %d: Conducting internal investigation after thwarted plan\n", gang_id);
            fflush(stdout);
            conduct_internal_investigation(config, &shm_ptrs, gang_id);
        }
        
        // Signal all waiting members about the plan execution result
        pthread_cond_broadcast(&gang->plan_execute_cond);
        gang->plan_in_progress = 0;
        pthread_mutex_unlock(&gang->gang_mutex);

        // Trigger information spreading after plan execution
        int current_time = shared_game->elapsed_time; // Use game time or implement time tracking
        printf("Gang %d: Triggering information spreading at time %d\n", gang_id, current_time);
        fflush(stdout);
        
        spread_information_in_gang(gang, members, current_time, highest_rank_member_id);
        
        // Short delay before next plan
        sleep(2);
        
        printf("Gang %d: Planning next operation...\n", gang_id);
        fflush(stdout);
    }
    
    // Wait for all threads to finish (this won't be reached in the loop)
    for(int i = 0; i < gang->max_member_count; i++) {
        pthread_join(members[i].thread, NULL);
    }

    // Cleanup resources
    cleanup();


}

void handle_police_handshake(int gang_id, const Config* config) {
    Message msg;
    long gang_msgtype = get_gang_msgtype(config->max_gang_members, gang_id);
    
    // Check for handshake messages from police (non-blocking)
    if (receive_message_nonblocking(police_msgq_id, &msg, gang_msgtype) == 0) {
        if (msg.mode == MSG_HANDSHAKE) {
            int police_id = msg.MessageContent.police_id;
            printf("Gang %d: Received handshake from police %d\n", gang_id, police_id);
            fflush(stdout);
            
            // Find an available member to convert to agent
            int new_agent_id = -1;
            for (int i = 0; i < gang->max_member_count; i++) {
                if (members[i].is_alive && members[i].agent_id == -1) {
                    // Convert this member to an agent
                    members[i].agent_id = i;
                    new_agent_id = i;
                    gang->num_agents++;
                    
                    // Initialize secret agent attributes
                    secret_agent_init(&shm_ptrs, &members[i]);
                    
                    printf("Gang %d: Member %d converted to secret agent for police %d\n", 
                           gang_id, i, police_id);
                    fflush(stdout);
                    break;
                }
            }
            
            // Send response back to police
            Message response;
            response.mtype = get_police_msgtype(config->max_gang_members, config->num_gangs, police_id);
            response.mode = MSG_HANDSHAKE;
            response.MessageContent.agent_id = new_agent_id;
            
            if (send_message(police_msgq_id, &response) == 0) {
                printf("Gang %d: Sent handshake response to police %d with agent_id %d\n", 
                       gang_id, police_id, new_agent_id);
                fflush(stdout);
            } else {
                printf("Gang %d: Failed to send handshake response to police %d\n", 
                       gang_id, police_id);
                fflush(stdout);
            }
        }
    }
}

void handle_sigint(int signum) {
    // Set termination flag for clean shutdown
    should_terminate = 1;
    printf("Gang process received SIGINT, preparing to terminate...\n");
    fflush(stdout);
    
    // Cleanup resources
    exit(0);
}

void cleanup() {

    printf("cleaning up gang\n");
    cleanup_semaphores();
    
    // Close message queue (but don't delete it - police process manages it)
    if (police_msgq_id != -1) {
        printf("Gang: Closing message queue\n");
        // Note: We don't delete the queue here as it's shared with police
    }
    
    if (shared_game != NULL && shared_game != MAP_FAILED) {
        if (munmap(shared_game, sizeof(Game)) == -1) {
            perror("munmap failed");
        }
    }
}

void gang_init() {

}