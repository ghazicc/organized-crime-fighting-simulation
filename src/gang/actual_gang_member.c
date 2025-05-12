#include <stdio.h>
#include <stdlib.h>
#include "actual_gang_member.h"

void* actual_gang_member_thread_function(void* arg) {
    printf("Gang member thread started\n");
    // GangMember *member = (GangMember*)arg;
    // Game *shared_game = member->game;
    // Config *config = member->config;

    // // Simulate the gang member's actions
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