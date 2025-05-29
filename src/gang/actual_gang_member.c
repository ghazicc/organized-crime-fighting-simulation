#include <stdio.h>
#include <stdlib.h>
#include <unistd.h> // For sleep()
#include "actual_gang_member.h"
#include "gang.h" // For Member struct
#include "success_rate.h" // For success rate calculation
#include "config.h"
#include "target_selection.h"

extern ShmPtrs shm_ptrs;
extern int highest_rank_member_id;

void* actual_gang_member_thread_function(void* arg) {
    printf("Gang member thread started\n");
    fflush(stdout);
    Member *member = (Member*)arg;
    Gang *gang = &shm_ptrs.gangs[member->gang_id];
    printf("Gang member %d in gang %d started\n", member->member_id, member->gang_id);
    fflush(stdout);
    
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
    while (1) {
        // Wait for new plan to start
        pthread_mutex_lock(&gang->gang_mutex);
        while (!gang->plan_in_progress && gang->members_ready == 0) {
            printf("Gang %d, Member %d: Waiting for new plan to start\n", 
                   member->gang_id, member->member_id);
            fflush(stdout);
            pthread_mutex_unlock(&gang->gang_mutex);
            sleep(1);
            pthread_mutex_lock(&gang->gang_mutex);
        }
        pthread_mutex_unlock(&gang->gang_mutex);
        
        // Reset member's preparation for new plan
        member->prep_contribution = 0;
        printf("Gang %d, Member %d: Starting preparation for new plan\n", 
               member->gang_id, member->member_id);
        fflush(stdout);
        
        // Preparation phase for this plan
        while (1) {
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
