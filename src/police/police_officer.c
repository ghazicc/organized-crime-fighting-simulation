//
// Police officer behavior implementation - simplified without coordination
//

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include "police.h"
#include "random.h"
#include "game.h"

extern PoliceForce police_force;
ShmPtrs shm_ptr;

void* police_officer_thread(void* arg) {
    PoliceOfficer* officer = (PoliceOfficer*)arg;
    
    printf("Police Officer %d started monitoring Gang %d\n", 
           officer->police_id, officer->gang_id_monitoring);
    fflush(stdout);
    
    // Initialize random seed for this thread
    srand(time(NULL) + officer->police_id);
    
    while (!police_force.shutdown_requested && officer->is_active) {
        // Monitor gang activity
        monitor_gang_activity(officer, &shm_ptr);
        
        // Take action based on knowledge level
        if (officer->knowledge_level > 0.5f) {
            take_police_action(officer, &shm_ptr);
        }
        
        // Sleep for a random time (1-5 seconds) before next check
        sleep(rand() % 5 + 1);
    }
    
    printf("Police Officer %d stopping monitoring of Gang %d\n", 
           officer->police_id, officer->gang_id_monitoring);
    fflush(stdout);
    
    return NULL;
}

void monitor_gang_activity(PoliceOfficer* officer, ShmPtrs *shm_ptr) {
    Gang* gang = &shm_ptr->gangs[officer->gang_id_monitoring];
    
    // Check if a gang exists and has members
    if (gang->num_alive_members <= 0) {
        return;
    }
    
    pthread_mutex_lock(&police_force.police_mutex);
    
    // Increase knowledge based on gang's notoriety and activity
    float knowledge_increase = 0.0f;
    
    // Higher notoriety makes gang easier to detect
    knowledge_increase += gang->notoriety * 0.1f;
    
    // Active planning increases detection chance
    if (gang->plan_in_progress) {
        knowledge_increase += 0.15f;
        printf("Police Officer %d: Detected Gang %d planning activity\n", 
               officer->police_id, officer->gang_id_monitoring);
        fflush(stdout);
    }
    
    // Check for successful plans (increases heat)
    if (gang->num_successful_plans > 0) {
        knowledge_increase += gang->num_successful_plans * 0.05f;
    }
    
    // Check for secret agents in the gang (managed by this officer)
    Member **members = &shm_ptr->gang_members[officer->gang_id_monitoring];
    for (int i = 0; i < gang->max_member_count; i++) {
        if (members[i]->is_alive && members[i]->agent_id >= 0) {
            // Check if this agent is managed by this officer
            for (int j = 0; j < officer->num_agents; j++) {
                if (officer->secret_agent_ids[j] == members[i]->agent_id) {
                    knowledge_increase += 0.2f; // Agents provide significant intelligence
                    printf("Police Officer %d: Receiving intel from Agent %d in Gang %d\n", 
                           officer->police_id, members[i]->agent_id, officer->gang_id_monitoring);
                    fflush(stdout);
                    break;
                }
            }
        }
    }
    
    // Add some random element to knowledge gathering
    knowledge_increase += (rand() % 100) / 1000.0f; // 0.0 to 0.099
    
    officer->knowledge_level += knowledge_increase;
    
    // Cap knowledge at 1.0
    if (officer->knowledge_level > 1.0f) {
        officer->knowledge_level = 1.0f;
    }
    
    if (knowledge_increase > 0.05f) {
        printf("Police Officer %d: Knowledge of Gang %d increased to %.3f\n", 
               officer->police_id, officer->gang_id_monitoring, officer->knowledge_level);
        fflush(stdout);
    }
    
    pthread_mutex_unlock(&police_force.police_mutex);
}

void take_police_action(PoliceOfficer* officer, ShmPtrs *shm_ptrs) {
    Gang* gang = &shm_ptrs->gangs[officer->gang_id_monitoring];
    
    if (gang->num_alive_members <= 0) {
        return;
    }
    
    // Decide on action based on knowledge level
    float action_threshold = (rand() % 100) / 100.0f;
    
    if (officer->knowledge_level > 0.8f && action_threshold < 0.4f) {
        arrest_gang_members(officer, shm_ptrs);
    } else if (officer->knowledge_level > 0.6f && action_threshold < 0.3f) {
        investigate_gang(officer);
    } else if (officer->knowledge_level > 0.5f && action_threshold < 0.2f) {
        investigate_gang(officer);
    }
}

void arrest_gang_members(PoliceOfficer* officer, ShmPtrs *shm_ptrs) {
    Gang* gang = &shm_ptrs->gangs[officer->gang_id_monitoring];
    
    printf("Police Officer %d: Attempting arrests in Gang %d\n", 
           officer->police_id, officer->gang_id_monitoring);
    fflush(stdout);
    
    pthread_mutex_lock(&gang->gang_mutex);
    
    int arrests_made = 0;
    float arrest_success_rate = officer->knowledge_level * 0.7f; // Max 70% success rate

    Member **members = &shm_ptrs->gang_members[officer->gang_id_monitoring];
    for (int i = 0; i < gang->max_member_count; i++) {
        if (members[i]->is_alive && members[i]->agent_id < 0) { // Don't arrest agents
            if ((rand() % 100) / 100.0f < arrest_success_rate) {
                members[i]->is_alive = false;
                gang->num_alive_members--;
                arrests_made++;
                printf("Police Officer %d: Arrested Gang %d Member %d\n", 
                       officer->police_id, officer->gang_id_monitoring, i);
                fflush(stdout);
            }
        }
    }
    
    pthread_mutex_unlock(&gang->gang_mutex);
    
    if (arrests_made > 0) {
        printf("Police Officer %d: Successfully arrested %d members from Gang %d\n",
               officer->police_id, arrests_made, officer->gang_id_monitoring);
        fflush(stdout);
    } else {
        printf("Police Officer %d: Arrest operation failed for Gang %d\n", 
               officer->police_id, officer->gang_id_monitoring);
        fflush(stdout);
    }
}

void investigate_gang(PoliceOfficer* officer) {
    printf("Police Officer %d: Investigating Gang %d\n", 
           officer->police_id, officer->gang_id_monitoring);
    
    // Investigation increases knowledge slightly
    officer->knowledge_level += 0.05f;
    if (officer->knowledge_level > 1.0f) {
        officer->knowledge_level = 1.0f;
    }
    
    printf("Police Officer %d: Investigation complete, knowledge level: %.3f\n", 
           officer->police_id, officer->knowledge_level);
    fflush(stdout);
}