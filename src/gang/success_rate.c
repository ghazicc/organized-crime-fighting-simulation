#include <stdio.h>
#include <stdlib.h>
#include "config.h"
#include "game.h"
#include "gang.h"
#include "random.h"
#include "target_selection.h"

extern Game *shared_game;

// Calculate success rate based on the formula
float calculate_success_rate(Gang *gang, TargetType target_type, Config *config) {
    if (gang == NULL || config == NULL) {
        return 0.0f;
    }
    
    // Constants for the formula
    const int num_ranks = config->num_ranks;        // |R|: Total number of ranks
    const int prep_levels = 100;                   // |P|: Total number of preparation levels (using 100 as max)
    
    
    // Calculate the attribute sum portion: ∑W(T)·Ai / |G|
    float success_rate = 0.0f;
    for (int i = 0; i < gang->max_member_count; i++) {
        // For each member, calculate dot product of their attributes with target weights

        if (!gang->members[i].is_alive) {
            continue;
        }

        // calculate attribute factor
        float dot_product = calculate_dot_product(gang->members[i].attributes, 
                                                  shared_game->targets[target_type].weights, 
                                                  NUM_ATTRIBUTES);
        

        // calculate rank factor
        float rank_factor = (1.0f + (float)gang->members[i].rank / num_ranks);
        // calculate preparation factor
        float prep_factor = (1.0f + (float)gang->members[i].prep_contribution / prep_levels);

        success_rate += dot_product * rank_factor * prep_factor;
                                                  
    }

    float difficulty_factor = (1.0f + (config->difficulty_level * 1.0) / config->max_difficulty);
    
    success_rate /= (gang->max_member_count * difficulty_factor * difficulty_factor); // Average over all members

    // Clamp to 0-100 range
    if (success_rate < 0.0f) success_rate = 0.0f;
    if (success_rate > 100.0f) success_rate = 100.0f;
    
    printf("Gang %d success rate calculation:\n", gang->gang_id);
    printf("  - Final success rate: %.2f%%\n", success_rate);
    fflush(stdout);
    
    return success_rate;
}

// Determine if a plan succeeds based on success rate
bool determine_plan_success(Gang *gang, TargetType target_type, Config *config) {
    float success_rate = calculate_success_rate(gang, target_type, config);
    
    // Generate a random number from 0 to 100
    float random_value = random_float(0, 100);
    
    // The plan succeeds if the random value is less than the success rate
    bool success = random_value < success_rate;
    
    printf("Gang %d plan %s! (Success rate: %.2f%%, Random roll: %.2f)\n", 
           gang->gang_id, success ? "SUCCEEDED" : "FAILED", success_rate, random_value);
    fflush(stdout);
    
    return success;
}
