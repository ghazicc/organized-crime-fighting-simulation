#include <stdio.h>
#include <stdlib.h>
#include <unistd.h> // For sleep()
#include "actual_gang_member.h"
#include "gang.h" // For Member struct

void* actual_gang_member_thread_function(void* arg) {
    printf("Gang member thread started\n");
    fflush(stdout);
    Member *member = (Member*)arg;
    Gang *gang = &shared_game->gangs[member->gang_id];
    printf("Gang member %d in gang %d started\n", member->member_id, member->gang_id);
    fflush(stdout);
    
    // Check if this is the highest-ranked member in the gang
    if (member->member_id == highest_rank_member_id) {
        printf("Gang %d: Member %d is the highest-ranked member (rank %d) - selecting target\n",
               member->gang_id, member->member_id, member->rank);
        fflush(stdout);
        
        // Let the highest-ranked member select a target
        TargetType selected_target = select_target(shared_game, gang, member->member_id);
        
        // Set preparation parameters based on the selected target
        set_preparation_parameters(gang, selected_target, NULL); // We'll need to pass config later
        
        // Reset all members' preparation levels
        reset_preparation_levels(gang);
        
        printf("Gang %d: Target selected by highest-ranked member, type: %d, prep time: %d, prep level: %d\n",
               member->gang_id, gang->target_type, gang->prep_time, gang->prep_level);
        fflush(stdout);
    }
    
    // Regular member behavior
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
            break;
        }
    }
    
    return NULL; // Return properly
}
