//
// Created by yazan on 5/12/2025.
//

#ifndef POLICE_H
#define POLICE_H

#include <pthread.h>
#include <stdbool.h>
#include "config.h"
#include "game.h"
#include "message.h"

#define MAX_GANGS 20
#define MAX_AGENTS_PER_GANG 20
#define KNOWLEDGE_THRESHOLD 0.8f
#define MSG_QUEUE_KEY 0x1234
#define MAX_PLANT_ATTEMPTS 3  // Maximum attempts to plant an agent

// Agent information structure
typedef struct {
    int agent_id;
    float knowledge_level;
    bool is_active;
    time_t last_report_time;
} AgentInfo;

typedef struct {
    int police_id;
    int gang_id_monitoring;  // Which gang this police officer monitors (same as police_id)
    bool is_active;
    pthread_t thread;

    // Message queue communication
    int msgq_id;
    
    // Agent management
    int num_agents;
    AgentInfo agents[MAX_AGENTS_PER_GANG];
    
    // Knowledge and monitoring
    float knowledge_level;

    pthread_mutex_t officer_mutex;

} PoliceOfficer;

typedef struct {
    int num_officers;

    // Arrested gangs array - indexed by gang_id, value is time until release (0 = not arrested)
    pthread_mutex_t arrest_mutex;
    
    // Message queue for communication
    int msgq_id;
    
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
bool attempt_plant_agent_handshake(PoliceOfficer* officer, Config* config);
void handle_agent_death_notification(PoliceOfficer* officer, Message* msg);
void request_information_from_agent(PoliceOfficer* officer, int agent_index);
void communicate_with_agents(PoliceOfficer* officer);
void process_agent_message(PoliceOfficer* officer, Message* msg);
void evaluate_imprisonment_probability(PoliceOfficer* officer);
void imprison_gang(PoliceOfficer* officer);
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
