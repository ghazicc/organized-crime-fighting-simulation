#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "target_selection.h"
#include "config.h"

// Calculate dot product between two vectors of attributes
float calculate_dot_product(const float *attributes, const double *weights, int size) {
    float result = 0.0f;
    for (int i = 0; i < size; i++) {
        result += attributes[i] * (float)weights[i];
    }
    return result;
}

// Sum all values in an array
float sum_array(const float *array, int size) {
    float total = 0.0f;
    for (int i = 0; i < size; i++) {
        total += array[i];
    }
    return total;
}

int find_highest_ranked_member(Gang *gang, Member *members) {
    if (gang == NULL || gang->max_member_count <= 0 || members == NULL) {
        return -1;
    }

    int highest_rank = -1;
    int highest_ranked_id = -1;

    for (int i = 0; i < gang->max_member_count; i++) {
        if (members[i].rank > highest_rank) {
            highest_rank = members[i].rank;
            highest_ranked_id = i;
        }
    }

    return highest_ranked_id;
}



TargetType select_target(Game *game, Gang *gang, int highest_rank_member_id) {
    if (game == NULL || gang == NULL || highest_rank_member_id < 0 || highest_rank_member_id >= gang->max_member_count) {
        // Default to bank robbery if something's wrong
        return TARGET_BANK_ROBBERY;
    }

    // Get the highest-ranked member
    Member *leader = &gang->members[highest_rank_member_id];
    
    // Initialize random seed
    srand(time(NULL) + gang->gang_id);
    
    // Calculate weighted preferences based on member attributes
    
    float heat;
    float min_heat = 0.0f;
    TargetType selected_targets[NUM_TARGETS];
    TargetType selected_target;

    // Calculate minimum heat for all targets
    for (int t = 0; t < NUM_TARGETS; t++) {
        // Calculate score based on how well the leader's attributes match the target requirements
        heat = game->targets[t].heat * gang->heat[t];
        if(min_heat > heat) {
            min_heat = heat;
        }
    }

    // Select the targets with the minimum heat
    int num_selected = 0;
    for (int t = 0; t < NUM_TARGETS; t++) {
        if (game->targets[t].heat * gang->heat[t] == min_heat) {
            selected_targets[t] = (TargetType)t;
            num_selected++;
        }
    }

    // choose a random target from the selected ones
    if (num_selected > 0) {
        int random_index = rand() % num_selected;
        selected_target = selected_targets[random_index];
    } else {
        selected_target = TARGET_BANK_ROBBERY; // Fallback
    }


    
    printf("Gang %d leader (member %d) selected target: %s (heat: %.2f)\n", 
            gang->gang_id, highest_rank_member_id,
            game->targets[selected_target].name, min_heat);
    fflush(stdout);
    
    return selected_target;
}

void set_preparation_parameters(Gang *gang, TargetType target_type, Config *config) {
    if (gang == NULL) return;
    
    // Store the target type
    gang->target_type = target_type;
    
    // Set preparation time based on target complexity and some randomness
    // More complex targets require more preparation time
    int base_prep_time = 10 + (target_type * 5); // Base time increases with target complexity
    int random_factor = rand() % 10;  // Random factor 0-9
    
    gang->prep_time = base_prep_time + random_factor;
    
    // Set required preparation level based on target complexity
    // More complex targets require higher preparation levels
    int base_prep_level = 50 + (target_type * 10); // Base level increases with complexity
    random_factor = rand() % 20;  // Random factor 0-19
    
    gang->prep_level = base_prep_level + random_factor;
    
    printf("Gang %d preparation parameters set: time=%d, required level=%d\n", 
            gang->gang_id, gang->prep_time, gang->prep_level);
    fflush(stdout);
}

void reset_preparation_levels(Gang *gang) {
    if (gang == NULL) return;
    
    for (int i = 0; i < gang->max_member_count; i++) {
        gang->members[i].prep_contribution = 0;
    }
    
    printf("Gang %d preparation levels reset to 0\n", gang->gang_id);
    fflush(stdout);
}



