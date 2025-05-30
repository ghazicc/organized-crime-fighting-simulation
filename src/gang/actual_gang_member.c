#include <stdio.h>
#include <stdlib.h>
#include <unistd.h> // For sleep()
#include "actual_gang_member.h"
#include "gang.h" // For Member struct
#include "success_rate.h" // For success rate calculation
#include "config.h"
#include "target_selection.h"
#include "secret_agent_utils.h" // For secret agent functionality
#include "message.h" // For message handling

extern ShmPtrs shm_ptrs;
extern int highest_rank_member_id;
extern volatile int should_terminate; // Flag for clean termination
extern int police_msgq_id; // Message queue for police communication

void* actual_gang_member_thread_function(void* arg) {
    printf("Gang member thread started\n");
    fflush(stdout);
    ThreadArgs *thread_args = (ThreadArgs*)arg;
    Member *member = thread_args->member;
    Config *config = thread_args->config;
    Gang *gang = &shm_ptrs.gangs[member->gang_id];
    printf("Gang member %d in gang %d started\n", member->member_id, member->gang_id);
    fflush(stdout);
    
    // Initialize secret agent attributes if this member is an agent
    if (member->agent_id >= 0) {
        secret_agent_init(&shm_ptrs, member);
        printf("Gang %d, Member %d: Initialized as secret agent with ID %d\n", 
               member->gang_id, member->member_id, member->agent_id);
        fflush(stdout);
    }
    
    // Check if this is the highest-ranked member in the gang
    if (member->member_id == highest_rank_member_id) {
        printf("Gang %d: Member %d is the highest-ranked member (rank %d) - selecting target\n",
               member->gang_id, member->member_id, member->rank);
        fflush(stdout);
        
        // Let the highest-ranked member select a target
        TargetType selected_target = select_target(shm_ptrs.shared_game, gang, shm_ptrs.gang_members[member->gang_id], highest_rank_member_id);
        
        // Set preparation parameters based on the selected target
        set_preparation_parameters(gang, selected_target, NULL); // We'll need to pass config later
        
        // Reset all members' preparation levels
        reset_preparation_levels(gang, shm_ptrs.gang_members[member->gang_id]);
        
        printf("Gang %d: Target selected by highest-ranked member, type: %d, prep time: %d, prep level: %d\n",
               member->gang_id, gang->target_type, gang->prep_time, gang->prep_level);
        fflush(stdout);
    }
    
    printf("Gang %d, Member %d: Starting regular preparation loop\n", 
           member->gang_id, member->member_id);
    fflush(stdout);
    
    // Regular member behavior - loop for multiple plans
    while (!should_terminate) {
        // Wait for new plan to start
        pthread_mutex_lock(&gang->gang_mutex);
        while (!gang->plan_in_progress && gang->members_ready == 0 && !should_terminate) {
            printf("Gang %d, Member %d: Waiting for new plan to start\n", 
                   member->gang_id, member->member_id);
            fflush(stdout);
            pthread_mutex_unlock(&gang->gang_mutex);
            sleep(1);
            pthread_mutex_lock(&gang->gang_mutex);
        }
        pthread_mutex_unlock(&gang->gang_mutex);
        
        // Check termination flag before starting new plan
        if (should_terminate) {
            printf("Gang %d, Member %d: Termination requested, exiting thread\n", 
                   member->gang_id, member->member_id);
            fflush(stdout);
            break;
        }
        
        // Reset member's preparation for new plan
        member->prep_contribution = 0;
        printf("Gang %d, Member %d: Starting preparation for new plan\n", 
               member->gang_id, member->member_id);
        fflush(stdout);
        
        // Preparation phase for this plan
        while (1) {
            // Secret agent specific activities during preparation
            if (member->agent_id >= 0) {
                // Secret agents gather information by asking other gang members
                // Randomly select another gang member to ask about the plan
                if (gang->num_alive_members > 1 && rand() % 4 == 0) { // 25% chance per iteration
                    int target_member_id = rand() % gang->max_member_count;
                    if (target_member_id != member->member_id && 
                        shm_ptrs.gang_members[member->gang_id][target_member_id].is_alive) {
                        
                        Member* target_member = &shm_ptrs.gang_members[member->gang_id][target_member_id];
                        
                        printf("Gang %d, Agent %d: Asking member %d for information\n",
                               member->gang_id, member->member_id, target_member_id);
                        fflush(stdout);
                        
                        // Record this member as having been asked for information
                        // This is needed for internal investigations later
                        secret_agent_record_asker(&shm_ptrs, *config, target_member, member->member_id);
                        
                        // Gather information from the target member
                        secret_agent_ask_member(&shm_ptrs, member, target_member);
                        
                        printf("Gang %d, Agent %d: Information gathering complete, knowledge: %.2f, suspicion: %.2f\n",
                               member->gang_id, member->member_id, 
                               shm_ptrs.gang_members[member->gang_id][member->member_id].knowledge,
                               shm_ptrs.gang_members[member->gang_id][member->member_id].suspicion);
                        fflush(stdout);
                    }
                }
                
                // Handle police communication for secret agents
                if (member->agent_id >= 0) {
                    // Handle police requests for knowledge reporting
                    secret_agent_handle_police_requests(member, shm_ptrs.shared_game, police_msgq_id, 
                                                       member->gang_id, gang, *config);
                    
                    // Send periodic communication to police
                    secret_agent_periodic_communication(&shm_ptrs, member, shm_ptrs.shared_game, 
                                                       police_msgq_id, member->gang_id, gang, *config);
                }
            }
            
            // Simulate member contributing to preparation
            member->prep_contribution += rand() % 10;
            
            printf("Gang %d, Member %d: Preparation contribution now %d\n", 
                   member->gang_id, member->member_id, member->prep_contribution);
            fflush(stdout);
            
            // Sleep for a random time (1-3 seconds)
            sleep(rand() % 3 + 1);
            
            // Check if preparation is complete
            if (member->prep_contribution >= gang->prep_level) {
                printf("Gang %d, Member %d: Reached required preparation level %d\n",
                       member->gang_id, member->member_id, gang->prep_level);
                fflush(stdout);
                
                // Lock the gang mutex to update shared state
                pthread_mutex_lock(&gang->gang_mutex);
                
                // Increment ready members count
                gang->members_ready++;
                printf("Gang %d: Member %d is ready. %d/%d members ready\n", 
                       gang->gang_id, member->member_id, gang->members_ready, gang->num_alive_members);
                fflush(stdout);
                
                // If this is the last member to complete preparation
                if (gang->members_ready == gang->num_alive_members) {
                    printf("Gang %d: All members ready! Signaling preparation complete to main thread\n", gang->gang_id);
                    fflush(stdout);
                    
                    // Signal that all members are ready - the main thread will determine success
                    pthread_cond_broadcast(&gang->prep_complete_cond);
                }
                
                // Wait for the main thread to determine plan success
                if (gang->plan_success == 0) {  // Plan success not determined yet
                    printf("Gang %d: Member %d waiting for main thread to determine plan outcome\n", 
                           gang->gang_id, member->member_id);
                    fflush(stdout);
                    
                    // Wait for plan execution result from main thread
                    pthread_cond_wait(&gang->plan_execute_cond, &gang->gang_mutex);
                }
                
                // React to plan success or failure
                if (gang->plan_success == 1) {
                    printf("Gang %d: Member %d celebrating successful plan!\n", 
                           gang->gang_id, member->member_id);
                } else {
                    printf("Gang %d: Member %d disappointed about failed plan...\n", 
                           gang->gang_id, member->member_id);
                    
                    // Conduct internal investigation if this is the highest-ranked member
                    // and the plan was thwarted (failed)
                    if (member->member_id == highest_rank_member_id) {
                        printf("Gang %d: Plan thwarted! Highest-ranked member %d conducting internal investigation\n",
                               member->gang_id, member->member_id);
                        fflush(stdout);
                        
                        // Unlock mutex before investigation to avoid deadlock
                        pthread_mutex_unlock(&gang->gang_mutex);
                        
                        conduct_internal_investigation(*config, &shm_ptrs, member->gang_id);
                        
                        printf("Gang %d: Internal investigation completed after thwarted plan\n", member->gang_id);
                        fflush(stdout);
                        
                        // Re-lock mutex for cleanup
                        pthread_mutex_lock(&gang->gang_mutex);
                    }
                }
                
                // Unlock the mutex
                pthread_mutex_unlock(&gang->gang_mutex);
                break; // Break out of preparation loop for this plan
            }
        }
        
        // Short rest between plans
        printf("Gang %d, Member %d: Resting before next plan\n", 
               member->gang_id, member->member_id);
        fflush(stdout);
        sleep(1);
    }
    
    return NULL; // Return properly
}
