//
// Created by yazan on 5/12/2025.
//

#ifndef POLICE_H
#define POLICE_H

#include <pthread.h>
#include <stdbool.h>
#include "config.h"
#include "game.h"

#define MAX_GANGS 20
#define MAX_AGENTS_PER_GANG 20


typedef struct {
    int police_id;
    int gang_id_monitoring;  // Which gang this police officer monitors (same as police_id)
    bool is_active;
    pthread_t thread;

    // Simple array of secret agent IDs managed by this police officer
    int num_agents;
    
    // Knowledge and monitoring
    float knowledge_level;

    pthread_mutex_t officer_mutex;
    int secret_agent_ids[MAX_AGENTS_PER_GANG];

} PoliceOfficer;

typedef struct {
    int num_officers;

    // Arrested gangs array - indexed by gang_id, value is time until release (0 = not arrested)
    pthread_mutex_t arrest_mutex;
    
    // Coordination
    pthread_mutex_t police_mutex;
    bool shutdown_requested;
    
    int arrested_gangs[MAX_GANGS];
    PoliceOfficer officers[MAX_GANGS];

} PoliceForce;

// Function declarations
void* police_officer_thread(void* arg);
void monitor_gang_activity(PoliceOfficer* officer, ShmPtrs *shm_ptr);
void take_police_action(PoliceOfficer* officer, ShmPtrs *shm_ptr);
void arrest_gang_members(PoliceOfficer* officer, ShmPtrs *shm_ptr);
void investigate_gang(PoliceOfficer* officer);
void handle_gang_arrest(PoliceOfficer* officer);
void handle_gang_release(PoliceOfficer* officer);
void process_arrest_timers(PoliceForce* force);
void init_police_force(Config *config);

// Initialization and cleanup
void start_police_operations(void);
void shutdown_police_force(void);
void cleanup_police();

#endif //POLICE_H
