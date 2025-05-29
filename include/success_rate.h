#ifndef SUCCESS_RATE_H
#define SUCCESS_RATE_H

#include <stdbool.h>
#include "gang.h"
#include "config.h"

/**
 * Calculate the success rate for a gang's plan
 * 
 * @param gang The gang executing the plan
 * @param members The gang's members array
 * @param target_type The type of target for the plan
 * @param config The game configuration
 * @return The calculated success rate between 0-100%
 */
float calculate_success_rate(Gang *gang, Member *members, TargetType target_type, Config *config);

/**
 * Determine if a plan succeeds based on the calculated success rate
 * 
 * @param gang The gang executing the plan
 * @param members The gang's members array
 * @param target_type The type of target for the plan
 * @param config The game configuration
 * @return true if the plan succeeds, false otherwise
 */
bool determine_plan_success(Gang *gang, Member *members, TargetType target_type, Config *config);

#endif // SUCCESS_RATE_H
