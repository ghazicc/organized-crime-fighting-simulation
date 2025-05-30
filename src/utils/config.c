#include "config.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

// Function to load configuration settings from a specified file
int load_config(const char *filename, Config *config) {
    // Attempt to open the config file in read mode
    FILE *file = fopen(filename, "r");
    if (!file) {
        perror("Error opening config file"); // Print an error message if file opening fails
        return -1; // Return error code
    }

    // Initialize all configuration values to default or invalid values to indicate uninitialized state
    config->agent_success_rate = -1;
    config->death_probability = -1;
    config->max_thwarted_plans = -1;
    config->max_successful_plans = -1;
    config->max_executed_agents = -1;
    config->max_agents_per_gang = -1;
    config->min_gangs = -1;
    config->max_gangs = -1;
    config->min_gang_size = -1;
    config->max_gang_size = -1;
    config->suspicion_threshold = -1;
    config->prison_period = -1;
    config->num_ranks = -1;
    config->min_time_prepare = -1;
    config->max_time_prepare = -1;
    config->min_level_prepare = -1;
    config->max_level_prepare = -1;
    config->difficulty_level = -1;
    config->max_difficulty = -1;
    config->timeout_period = -1;
    config->max_askers = -1;  // Initialize max_askers
    config->min_prison_period = -1;
    config->max_prison_period = -1;

    // Buffer to hold each line from the configuration file
    char line[256];
    while (fgets(line, sizeof(line), file)) {
        // Ignore comments and empty lines
        if (line[0] == '#' || line[0] == '\n') continue;

        // Parse each line as a key-value pair
        char key[50];
        float value;
        if (sscanf(line, "%40[^=]=%f", key, &value) == 2) {

            // Set corresponding config fields based on the key
            if (strcmp(key, "max_thwarted_plans") == 0) config->max_thwarted_plans = (int)value;
            else if (strcmp(key, "max_successful_plans") == 0) config->max_successful_plans = (int)value;
            else if (strcmp(key, "max_executed_agents") == 0) config->max_executed_agents = (int)value;
            else if (strcmp(key, "max_agents_per_gang") == 0) config->max_agents_per_gang = (int)value;
            else if (strcmp(key, "min_gangs") == 0) config->min_gangs = (int)value;
            else if (strcmp(key, "max_gangs") == 0) config->max_gangs = (int)value;
            else if (strcmp(key, "min_gang_size") == 0) config->min_gang_size = (int)value;
            else if (strcmp(key, "max_gang_size") == 0) config->max_gang_size = (int)value;
            else if (strcmp(key, "suspicion-threshold") == 0 || strcmp(key, "suspicion_threshold") == 0)
                config->suspicion_threshold = value;
            else if (strcmp(key, "knowledge-threshold") == 0 || strcmp(key, "knowledge_threshold") == 0)
                config->knowledge_threshold = value;
            else if (strcmp(key, "agent_success_rate") == 0) config->agent_success_rate = value;
            else if (strcmp(key, "prison_period") == 0) config->prison_period = (int)value;
            else if (strcmp(key, "num_ranks") == 0) config->num_ranks = (int)value;
            else if (strcmp(key, "min_time_prepare") == 0) config->min_time_prepare = (int)value;
            else if (strcmp(key, "max_time_prepare") == 0) config->max_time_prepare = (int)value;
            else if (strcmp(key, "min_level_prepare") == 0) config->min_level_prepare = (int)value;
            else if (strcmp(key, "max_level_prepare") == 0) config->max_level_prepare = (int)value;
            else if (strcmp(key, "death_probability") == 0) config->death_probability = value;
            else if (strcmp(key, "difficulty_level") == 0) config->difficulty_level = (int)value;
            else if (strcmp(key, "max_difficulty") == 0) config->max_difficulty = (int)value;
            else if (strcmp(key, "max_askers") == 0) config->max_askers = (int)value;  // Added max_askers
            else if (strcmp(key, "max_gang_members") == 0) config->max_gang_members = (int)value;  // Added max_gang_members
            else if (strcmp(key, "min_prison_period") == 0) config->min_prison_period = (int)value;
            else if (strcmp(key, "max_prison_period") == 0) config->max_prison_period = (int)value;

            else if (strcmp(key, "timeout_period") == 0) config->timeout_period = (int)value;
            else {
                fprintf(stderr, "Unknown key: %s\n", key);
                fclose(file);
                return -1;
            }
        }
    }

    fclose(file); // Close the config file

    // Debug print if __CLI is defined
#ifdef __DEBUG
    printf("Config values: \n MIN_ENERGY: %d\n"
           "MAX_ENERGY: %d\n MAX_SCORE: %d\n"
           "MAX_TIME: %d\n NUM_ROUNDS: %d\n"
           "MIN_RATE_DECAY: %d\n MAX_RATE_DECAY: %d\n"
           , config->MIN_ENERGY,
           config->MAX_ENERGY,
           config->MAX_SCORE,
           config->MAX_TIME,
           config->NUM_ROUNDS,
           config->MIN_RATE_DECAY,
           config->MAX_RATE_DECAY);


fflush(stdout);
#endif


    // Check if all required parameters are set
    return check_parameter_correctness(config);
}

void print_config(Config *config) {
    printf("Configuration:\n");
    printf("max_thwarted_plans: %d\n", config->max_thwarted_plans);
    printf("max_successful_plans: %d\n", config->max_successful_plans);
    printf("max_executed_agents: %d\n", config->max_executed_agents);
    printf("max_agents_per_gang: %d\n", config->max_agents_per_gang);
    printf("min_gangs: %d\n", config->min_gangs);
    printf("max_gangs: %d\n", config->max_gangs);
    printf("min_gang_size: %d\n", config->min_gang_size);
    printf("max_gang_size: %d\n", config->max_gang_size);
    printf("suspicion_threshold: %f\n", config->suspicion_threshold);
    printf("agent_success_rate: %f\n", config->agent_success_rate);
    printf("prison_period: %d\n", config->prison_period);
    printf("num_ranks: %d\n", config->num_ranks);
    printf("min_time_prepare: %d\n", config->min_time_prepare);
    printf("max_time_prepare: %d\n", config->max_time_prepare);
    printf("min_level_prepare: %d\n", config->min_level_prepare);
    printf("max_level_prepare: %d\n", config->max_level_prepare);
    printf("death_probability: %f\n", config->death_probability);
    printf("difficulty_level: %d\n", config->difficulty_level);
    printf("max_difficulty: %d\n", config->max_difficulty);
    printf("max_askers: %d\n", config->max_askers);  // Print max_askers
    printf("timeout_period: %d\n", config->timeout_period);
    printf("min_prison_period: %d\n", config->min_prison_period);
    printf("max_prison_period: %d\n", config->max_prison_period);
    fflush(stdout);
}

int check_parameter_correctness(const Config *config) {
    // Check that all integer parameters are non-negative
    if (config->max_thwarted_plans < 0 || config->max_successful_plans < 0 ||
        config->max_executed_agents < 0 || config->max_agents_per_gang < 0 ||
        config->min_gangs < 0 || config->max_gangs < 0 || config->min_gang_size < 0 ||
        config->prison_period < 0 || config->num_ranks < 0 ||
        config->min_time_prepare < 0 || config->max_time_prepare < 0 ||
        config->min_level_prepare < 0 || config->max_level_prepare < 0 ||
        config->max_gang_size < 0 || config->difficulty_level < 0 || config->max_difficulty < 0 ||
        config->max_askers < 0 ||  // Check max_askers
        config->max_gang_size < 0 || config->difficulty_level < 0 || config->max_difficulty < 0
        || config->timeout_period < 0 || config->min_prison_period < 0 ||
        config->max_prison_period < 0) {
        fprintf(stderr, "Integer values must be greater than or equal to 0\n");
        return -1;
    }

    // Check that float parameters are non-negative
    if (config->suspicion_threshold < 0 || config->agent_success_rate < 0 ||
        config->death_probability < 0) {
        fprintf(stderr, "Float values must be greater than or equal to 0\n");
        return -1;
    }

    // Logical consistency checks for minimum and maximum pairs
    if (config->min_gangs > config->max_gangs) {
        fprintf(stderr, "min_gangs cannot be greater than max_gangs\n");
        return -1;
    }

    if (config->min_gang_size > config->max_gang_size) {
        fprintf(stderr, "min_gang_size cannot be greater than max_gang_size\n");
        return -1;
    }

    if (config->min_time_prepare > config->max_time_prepare) {
        fprintf(stderr, "min_time_prepare cannot be greater than max_time_prepare\n");
        return -1;
    }

    if (config->min_level_prepare > config->max_level_prepare) {
        fprintf(stderr, "min_level_prepare cannot be greater than max_level_prepare\n");
        return -1;
    }

    return 0;
}

void serialize_config(Config *config, char *buffer) {
    sprintf(buffer, "%d %d %d %d %d %d %d %d %f %f %f %d %d %d %d %d %d %f %d %d %d %d %d %d %d %d",
            config->max_thwarted_plans,
            config->max_successful_plans,
            config->max_executed_agents,
            config->max_agents_per_gang,
            config->min_gangs,
            config->max_gangs,
            config->min_gang_size,
            config->max_gang_size,
            config->suspicion_threshold,
            config->knowledge_threshold,
            config->agent_success_rate,
            config->prison_period,
            config->num_ranks,
            config->min_time_prepare,
            config->max_time_prepare,
            config->min_level_prepare,
            config->max_level_prepare,
            config->death_probability,
            config->difficulty_level,
            config->max_difficulty,
            config->num_gangs,
            config->max_askers,
            config->timeout_period,
            config->min_prison_period,
            config->max_prison_period,
            config->max_gang_members
    );
}

void deserialize_config(const char *buffer, Config *config) {
    sscanf(buffer, "%d %d %d %d %d %d %d %d %f %f %f %d %d %d %d %d %d %f %d %d %d %d %d %d %d %d",
            &config->max_thwarted_plans,
            &config->max_successful_plans,
            &config->max_executed_agents,
            &config->max_agents_per_gang,
            &config->min_gangs,
            &config->max_gangs,
            &config->min_gang_size,
            &config->max_gang_size,
            &config->suspicion_threshold,
            &config->knowledge_threshold,
            &config->agent_success_rate,
            &config->prison_period,
            &config->num_ranks,
            &config->min_time_prepare,
            &config->max_time_prepare,
            &config->min_level_prepare,
            &config->max_level_prepare,
            &config->death_probability,
            &config->difficulty_level,
            &config->max_difficulty,
            &config->num_gangs,
            &config->max_askers,
            &config->timeout_period,
            &config->min_prison_period,
            &config->max_prison_period,
            &config->max_gang_members
            );
}

