#pragma once

#ifdef __cplusplus
extern "C" {
#endif


typedef struct {
    int max_thwarted_plans;
    int max_successful_plans;
    int max_executed_agents;
    int max_agents_per_gang;
    int min_gangs;
    int max_gangs;
    int min_gang_size;
    int max_gang_size;
    float suspicion_threshold;
    float agent_success_rate;
    int prison_period;
    int num_ranks;
    int min_time_prepare;
    int max_time_prepare;
    int min_level_prepare;
    int max_level_prepare;
    float death_probability;
    int difficulty_level;
    int max_difficulty;
    int num_gangs;
} Config;

int load_config(const char *filename, Config *config);
void print_config(Config *config);
int check_parameter_correctness(const Config *config);
void serialize_config(Config *config, char *buffer);
void deserialize_config(const char *buffer, Config *config);

#ifdef __cplusplus
}
#endif

