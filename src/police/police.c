//
// Created by yazan on 5/12/2025.
//

#include <stdlib.h>
#include <stdio.h>
#include <sys/mman.h>
#include <signal.h>
#include <time.h>
#include "config.h"
#include "shared_mem_utils.h"
#include "police.h"
#include "random.h"

PoliceForce police_force;
Game *shared_game = NULL;

void cleanup();
void handle_sigint(int signum);

void init_police_force(Game *shared_game, Config *config) {
    printf("POLICE: Initializing police force with %d officers\n", config->num_gangs);
    
    // Initialize police force structure
    police_force.num_officers = config->num_gangs;
    police_force.shutdown_requested = false;
    police_force.total_arrests = 0;
    police_force.total_plans_thwarted = 0;
    police_force.total_agents_deployed = 0;
    
    // Initialize mutexes
    pthread_mutex_init(&police_force.police_mutex, NULL);
    pthread_mutex_init(&police_force.arrest_mutex, NULL);
    
    // Initialize arrested gangs array (0 = not arrested)
    for (int i = 0; i < MAX_GANGS; i++) {
        police_force.arrested_gangs[i] = 0;
    }
    
    // Initialize each police officer
    for (int i = 0; i < config->num_gangs; i++) {
        PoliceOfficer *officer = &police_force.officers[i];
        officer->police_id = i;
        officer->gang_id_monitoring = i;  // Officer i monitors gang i
        officer->is_active = true;
        officer->shared_game = shared_game;
        officer->config = config;
        officer->num_agents = 0;
        officer->knowledge_level = 0.0f;
        officer->intelligence_sharing = 0.7f; // 70% sharing rate
        
        // Initialize officer mutex
        pthread_mutex_init(&officer->officer_mutex, NULL);
        
        // Initialize secret agent IDs array
        for (int j = 0; j < MAX_AGENTS_PER_GANG; j++) {
            officer->secret_agent_ids[j] = -1; // -1 means no agent
        }
        
        printf("POLICE: Officer %d assigned to monitor gang %d\n", i, i);
    }
    
    printf("POLICE: Police force initialized successfully\n");
}

int main(int argc, char *argv[]) {
    atexit(cleanup);

    printf("Police process starting...\n");
    fflush(stdout);

    if(argc != 3) {
        fprintf(stderr, "Usage: %s <serialized_config> <id>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    
    // Load configuration from serialized string
    Config config;
    deserialize_config(argv[1], &config);

    int police_department_id = atoi(argv[2]);
    printf("Police Department %d starting with %d gangs to monitor\n", 
           police_department_id, config.num_gangs);
    fflush(stdout);

    signal(SIGINT, handle_sigint);

    // Police process is a user of shared memory, not the owner
    shared_game = setup_shared_memory_user(&config);
    
    init_police_force(shared_game, &config);

    printf("Police Department: Initialized successfully\n");
    fflush(stdout);

    start_police_operations();

    printf("Police department shutting down\n");
    fflush(stdout);
    return 0;
}

void start_police_operations(void) {
    printf("POLICE: Starting police operations\n");
    
    // Start officer threads
    for (int i = 0; i < police_force.num_officers; i++) {
        PoliceOfficer *officer = &police_force.officers[i];
        if (pthread_create(&officer->thread, NULL, police_officer_thread, officer) != 0) {
            perror("POLICE: Failed to create officer thread");
            exit(EXIT_FAILURE);
        }
        printf("POLICE: Started thread for officer %d\n", i);
    }
    
    // Start arrest timer processing in main thread
    while (!police_force.shutdown_requested) {
        process_arrest_timers(&police_force);
        sleep(1);  // Check every second
    }
}

void* police_officer_thread(void* arg) {
    PoliceOfficer *officer = (PoliceOfficer*)arg;
    printf("POLICE: Officer %d thread started, monitoring gang %d\n", 
           officer->police_id, officer->gang_id_monitoring);
    
    while (officer->is_active && !police_force.shutdown_requested) {
        monitor_gang_activity(officer);
        
        // Check if gang is arrested
        pthread_mutex_lock(&police_force.arrest_mutex);
        bool gang_arrested = (police_force.arrested_gangs[officer->gang_id_monitoring] > 0);
        pthread_mutex_unlock(&police_force.arrest_mutex);
        
        if (gang_arrested) {
            printf("POLICE: Officer %d - Gang %d is currently arrested\n", 
                   officer->police_id, officer->gang_id_monitoring);
        }
        
        sleep(2);  // Check every 2 seconds
    }
    
    printf("POLICE: Officer %d thread terminating\n", officer->police_id);
    return NULL;
}

void monitor_gang_activity(PoliceOfficer* officer) {
    Gang *gang = &officer->shared_game->gangs[officer->gang_id_monitoring];
    
    // Check for suspicious activity or plan execution
    pthread_mutex_lock(&gang->gang_mutex);
    
    if (gang->plan_in_progress) {
        printf("POLICE: Officer %d detected plan in progress for gang %d\n", 
               officer->police_id, officer->gang_id_monitoring);
        
        // Determine if we should arrest based on knowledge and random chance
        bool should_arrest = false;
        float arrest_probability = 0.1;  // Base probability
        
        // Increase probability based on knowledge level
        arrest_probability += officer->knowledge_level * 0.3;
        
        // Check for secret agents in gang (they provide intelligence)
        for (int i = 0; i < gang->max_member_count; i++) {
            if (gang->members[i].is_alive && gang->members[i].agent_id >= 0) {
                // Check if this agent ID is in our managed list
                for (int j = 0; j < officer->num_agents; j++) {
                    if (officer->secret_agent_ids[j] == gang->members[i].agent_id) {
                        arrest_probability += 0.2;  // Each agent increases chance by 20%
                        break;
                    }
                }
            }
        }
        
        if ((float)rand() / RAND_MAX < arrest_probability) {
            should_arrest = true;
        }
        
        if (should_arrest) {
            printf("POLICE: Officer %d initiating arrest of gang %d\n", 
                   officer->police_id, officer->gang_id_monitoring);
            handle_gang_arrest(officer);
            police_force.total_plans_thwarted++;
        }
    }
    
    pthread_mutex_unlock(&gang->gang_mutex);
}

void handle_gang_arrest(PoliceOfficer* officer) {
    int gang_id = officer->gang_id_monitoring;
    
    pthread_mutex_lock(&police_force.arrest_mutex);
    
    // Set arrest time (from config prison_period)
    police_force.arrested_gangs[gang_id] = officer->config->prison_period;
    police_force.total_arrests++;
    
    pthread_mutex_unlock(&police_force.arrest_mutex);
    
    printf("POLICE: Gang %d arrested for %d time units\n", gang_id, officer->config->prison_period);
    
    // Mark the plan as failed in shared memory
    Gang *gang = &officer->shared_game->gangs[gang_id];
    pthread_mutex_lock(&gang->gang_mutex);
    gang->plan_success = -1;  // Mark as failed
    gang->plan_in_progress = 0;
    gang->num_thwarted_plans++;
    pthread_mutex_unlock(&gang->gang_mutex);
}

void handle_gang_release(PoliceOfficer* officer) {
    int gang_id = officer->gang_id_monitoring;
    
    printf("POLICE: Gang %d has been released from prison\n", gang_id);
    
    // Reset gang state in shared memory
    Gang *gang = &officer->shared_game->gangs[gang_id];
    pthread_mutex_lock(&gang->gang_mutex);
    gang->plan_in_progress = 0;
    gang->plan_success = 0;
    gang->members_ready = 0;
    pthread_mutex_unlock(&gang->gang_mutex);
}

void process_arrest_timers(PoliceForce* force) {
    pthread_mutex_lock(&force->arrest_mutex);
    
    for (int gang_id = 0; gang_id < force->num_officers; gang_id++) {
        if (force->arrested_gangs[gang_id] > 0) {
            force->arrested_gangs[gang_id]--;
            
            if (force->arrested_gangs[gang_id] == 0) {
                // Gang is being released
                pthread_mutex_unlock(&force->arrest_mutex);
                handle_gang_release(&force->officers[gang_id]);
                pthread_mutex_lock(&force->arrest_mutex);
            }
        }
    }
    
    pthread_mutex_unlock(&force->arrest_mutex);
}

void shutdown_police_force(void) {
    printf("POLICE: Shutting down police force\n");
    
    // Signal shutdown
    police_force.shutdown_requested = true;
    
    // Wait for all officer threads to finish
    for (int i = 0; i < police_force.num_officers; i++) {
        PoliceOfficer *officer = &police_force.officers[i];
        if (officer->is_active) {
            pthread_join(officer->thread, NULL);
        }
        
        // Cleanup officer resources
        pthread_mutex_destroy(&officer->officer_mutex);
    }
    
    // Cleanup main resources
    pthread_mutex_destroy(&police_force.police_mutex);
    pthread_mutex_destroy(&police_force.arrest_mutex);
    
    printf("POLICE: Statistics - Arrests: %d, Plans Thwarted: %d, Agents Deployed: %d\n",
           police_force.total_arrests, police_force.total_plans_thwarted, 
           police_force.total_agents_deployed);
}

void handle_sigint(int signum) {
    printf("POLICE: Received SIGINT, shutting down\n");
    cleanup();
    exit(0);
}

void cleanup() {
    printf("POLICE: Cleaning up resources\n");
    
    shutdown_police_force();
    
    if (shared_game != NULL && shared_game != MAP_FAILED) {
        if (munmap(shared_game, sizeof(Game)) == -1) {
            perror("munmap failed");
        }
    }
}