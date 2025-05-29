#ifndef TARGET_SELECTION_H
#define TARGET_SELECTION_H

#include "gang.h"
#include "game.h"

/**
 * Calculate the dot product between two vectors
 * 
 * @param attributes First vector (array of float values)
 * @param weights Second vector (array of double values)
 * @param size The size of both vectors
 * @return The dot product as a float
 */
float calculate_dot_product(const float *attributes, const double *weights, int size);

/**
 * Sum all values in an array
 * 
 * @param array The array of values to sum
 * @param size The size of the array
 * @return The sum as a float
 */
float sum_array(const float *array, int size);

/**
 * Find the member with the highest rank in the gang
 * 
 * @param gang The gang to search in
 * @return The ID of the member with the highest rank, or -1 if no members
 */
int find_highest_ranked_member(Gang *gang, Member *members);

/**
 * Select a new target for the gang based on attributes of the highest-ranked member
 * 
 * @param game The game state
 * @param gang The gang that's selecting a target
 * @param highest_rank_member_id The ID of the highest-ranked member
 * @return The selected target type
 */
TargetType select_target(Game *game, Gang *gang, int highest_rank_member_id);

/**
 * Set the preparation parameters for a gang's target
 * 
 * @param gang The gang to set parameters for
 * @param target_type The selected target type
 * @param config The game configuration
 */
void set_preparation_parameters(Gang *gang, TargetType target_type, Config *config);

/**
 * Reset all gang members' preparation contributions to 0
 * 
 * @param gang The gang to reset preparation levels for
 */
void reset_preparation_levels(Gang *gang);

#endif // TARGET_SELECTION_H
