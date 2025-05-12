#include <stdio.h>
#include <stdlib.h>
#include <unistd.h> // For sleep()
#include "secret_agent.h"
#include "game.h"
#include "gang.h" // For Member struct

void* secret_agent_thread_function(void* arg) {
    printf("Secret agent thread started\n");
    fflush(stdout);
    Member *member = (Member*)arg;
    printf("Secret agent %d in gang %d started\n", member->member_id, member->gang_id);
    fflush(stdout);
    
    // Add a slight delay to see if this thread runs
    sleep(2);
    printf("Secret agent thread still running after delay\n");
    fflush(stdout);
    
    return NULL; // Return properly

    // // Simulate the secret agent's actions
    // while (1) {
    //     // Check if the game is still running
    //     if (!check_game_conditions(shared_game, config)) {
    //         break;
    //     }

    //     // Simulate preparation time
    //     sleep(rand() % (config->max_time_prepare - config->min_time_prepare + 1) + config->min_time_prepare);

    //     // Update preparation contribution
    //     member->prep_contribution += rand() % (config->max_level_prepare - config->min_level_prepare + 1) + config->min_level_prepare;

    //     // Check if the gang member is an agent
    //     if (member->is_agent) {
    //         // Simulate agent actions
    //         sleep(rand() % (config->prison_period + 1));
    //         member->suspicion_level += rand() % 10; // Example of increasing suspicion level
    //     }

    //     // Print status update
    //     print_with_time1(shared_game, "Gang %d, Member %d: Preparation Contribution: %d, Suspicion Level: %.2f\n",
    //                      member->gang_id, member->member_id, member->prep_contribution, member->suspicion_level);
    // }

    // return NULL;
}