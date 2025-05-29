//
// Created by yazan on 5/12/2025.
//

#ifndef POLICE_H
#define POLICE_H

#include <pthread.h>
#include <stdbool.h>
#include "game.h"
#include "config.h"

#define MAX_GANGS 10
#define MAX_AGENTS_PER_GANG 10

typedef struct {
    int police_id;
    int gang_id_monitoring;  // Which gang this police officer monitors (same as police_id)
    bool is_active;
    pthread_t thread;

    // Simple array of secret agent IDs managed by this police officer
    int secret_agent_ids[MAX_AGENTS_PER_GANG];
    int num_agents;
    
    // Knowledge and monitoring
    float knowledge_level;
    float intelligence_sharing;
    
    pthread_mutex_t officer_mutex;
} PoliceOfficer;

typedef struct {
    PoliceOfficer officers[MAX_GANGS];
    int num_officers;
    
    // Arrested gangs array - indexed by gang_id, value is time until release (0 = not arrested)
    int arrested_gangs[MAX_GANGS];
    pthread_mutex_t arrest_mutex;
    
    // Coordination
    pthread_mutex_t police_mutex;
    bool shutdown_requested;
    
    // Statistics
    int total_arrests;
    int total_plans_thwarted;
    int total_agents_deployed;
} PoliceForce;

// Function declarations
void* police_officer_thread(void* arg);
void monitor_gang_activity(PoliceOfficer* officer);
void take_police_action(PoliceOfficer* officer);
void arrest_gang_members(PoliceOfficer* officer);
void investigate_gang(PoliceOfficer* officer);
void handle_gang_arrest(PoliceOfficer* officer);
void handle_gang_release(PoliceOfficer* officer);
void process_arrest_timers(PoliceForce* force);

// Initialization and cleanup
void init_police_force(Game *shared_game, Config *config);
void start_police_operations(void);
void shutdown_police_force(void);
void cleanup_police();

#endif //POLICE_H
