//
// Created by yazan on 5/12/2025.
//

#include "police.h"
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <time.h>
#include "config.h"
#include "shared_mem_utils.h"

#include <unistd.h>

#include "random.h"
#include "semaphores_utils.h"

PoliceForce police_force;
Game *shared_game = NULL;
ShmPtrs shm_ptrs;
Config config;

void cleanup();
void handle_sigint(int signum);

void init_police_force(Config *config) {
    printf("POLICE: Initializing police force with %d officers\n", config->num_gangs);

    // Initialize police force structure
    police_force.num_officers = config->num_gangs;
    police_force.shutdown_requested = false;

    // Initialize message queue
    police_force.msgq_id = create_message_queue(MSG_QUEUE_KEY);
    if (police_force.msgq_id == -1) {
        fprintf(stderr, "POLICE: Failed to create message queue\n");
        exit(EXIT_FAILURE);
    }

    // Initialize mutexes
    pthread_mutex_init(&police_force.police_mutex, NULL);
    pthread_mutex_init(&police_force.arrest_mutex, NULL);

    // Initialize arrested gangs array (0 = not arrested)
    memset(police_force.arrested_gangs, 0, sizeof(int) * config->num_gangs);

    // Initialize each police officer
    for (int i = 0; i < config->num_gangs; i++) {
        PoliceOfficer *officer = &police_force.officers[i];
        officer->police_id = i;
        officer->gang_id_monitoring = i;  // Officer i monitors gang i
        officer->is_active = true;
        officer->num_agents = 0;
        officer->knowledge_level = 0.0f;
        officer->msgq_id = police_force.msgq_id;

        // Initialize officer mutex
        pthread_mutex_init(&officer->officer_mutex, NULL);

        // Initialize agent info array
        for (int j = 0; j < config->max_agents_per_gang; j++) {
            officer->agents[j].agent_id = -1;
            officer->agents[j].knowledge_level = 0.0f;
            officer->agents[j].is_active = false;
            officer->agents[j].last_report_time = 0;
        }

        printf("POLICE: Officer %d assigned to monitor gang %d\n", i, i);
    }

    printf("POLICE: Police force initialized successfully\n");
    
    // Initial sync to shared memory
    sync_police_data_to_shared_memory();
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
    deserialize_config(argv[1], &config);

    int police_department_id = atoi(argv[2]);
    printf("Police Department %d starting with %d gangs to monitor\n",
           police_department_id, config.num_gangs);
    fflush(stdout);

    signal(SIGINT, handle_sigint);
    // Initialize semaphores for this process
    if (init_semaphores() != 0) {
        fprintf(stderr, "Police: Failed to initialize semaphores\n");
        exit(EXIT_FAILURE);
    }

    // Police process is a user of shared memory, not the owner
    shared_game = setup_shared_memory_user(&config, &shm_ptrs);

    init_police_force(&config);

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

    // Initialize random seed for this thread
    srand(time(NULL) + officer->police_id);

    while (officer->is_active && !police_force.shutdown_requested) {
        // Check if gang is arrested
        pthread_mutex_lock(&police_force.arrest_mutex);
        bool gang_arrested = (police_force.arrested_gangs[officer->gang_id_monitoring] > 0);
        pthread_mutex_unlock(&police_force.arrest_mutex);

        if (!gang_arrested) {
            // Try to plant agents if we have fewer than maximum and within attempt limits
            if (officer->num_agents < config.max_agents_per_gang &&
                (rand() % 100) < 40) { // 40% chance to try planting agent (increased from 20%)
                attempt_plant_agent_handshake(officer, &config);
            }

            // Communicate with existing agents
            communicate_with_agents(officer);

            // Take action based on intelligence
            take_police_action(officer, &shm_ptrs);
        } else {
            printf("POLICE: Officer %d - Gang %d is currently arrested\n",
                   officer->police_id, officer->gang_id_monitoring);
        }

        sleep(1);  // Check every 2 seconds
        
        // Sync police data to shared memory for graphics interface
        sync_police_data_to_shared_memory();
    }

    printf("POLICE: Officer %d thread terminating\n", officer->police_id);
    return NULL;
}

void communicate_with_agents(PoliceOfficer* officer) {
    Message msg;
    
    // Check for messages from agents and gang (agent death notifications)
    for (int i = 0; i < officer->num_agents; i++) {
        if (!officer->agents[i].is_active) continue;
        
        long agent_msg_type = get_agent_msgtype(config.max_agents_per_gang,
                                               (int8_t)officer->gang_id_monitoring, 
                                               (int8_t)officer->agents[i].agent_id);
        
        // Try to receive message (non-blocking)
        if (receive_message_nonblocking(officer->msgq_id, &msg, agent_msg_type) == 0) {
            process_agent_message(officer, &msg);
        }
        
        // Request info from agents that haven't reported recently
        time_t current_time = time(NULL);
        if (current_time - officer->agents[i].last_report_time > 10) // 10 seconds timeout
        {
            request_information_from_agent(officer, i);
        }
    }
    
    // Check for agent death notifications from gang
    long police_msg_type = get_police_msgtype(config.max_agents_per_gang, config.num_gangs, (int8_t)officer->police_id);
    if (receive_message_nonblocking(officer->msgq_id, &msg, police_msg_type) == 0) {
        if (msg.mode == MSG_AGENT_DEATH) {
            handle_agent_death_notification(officer, &msg);
        } else if (msg.mode == MSG_POLICE_REPORT) {
            process_agent_message(officer, &msg);
        }
    }
}

void process_agent_message(PoliceOfficer* officer, Message* msg) {
    if (msg->mode != MSG_POLICE_REPORT)
        return;
    
    float knowledge = msg->MessageContent.knowledge;
    
    // Find the agent that sent this message
    AgentInfo *agent = NULL;
    for (int i = 0; i < officer->num_agents; i++) {
        if (officer->agents[i].is_active) {
            // In a real implementation, you'd match by agent_id from the message type
            agent = &officer->agents[i];
            break;
        }
    }
    
    if (!agent) return;
    
    agent->knowledge_level = knowledge;
    agent->last_report_time = time(NULL);
    
    printf("POLICE: Officer %d received report from agent %d, knowledge: %.3f\n",
           officer->police_id, agent->agent_id, knowledge);
    
    // Sync updated knowledge to shared memory
    sync_police_data_to_shared_memory();
    
    // Check if knowledge is below threshold
    if (knowledge < config.knowledge_threshold) {
        // Periodic knowledge update - update officer's knowledge slightly
        officer->knowledge_level += 0.05f;
        if (officer->knowledge_level > 1.0f) officer->knowledge_level = 1.0f;
        
        // Check if all agents have knowledge below threshold
        bool all_below_threshold = true;
        for (int i = 0; i < officer->num_agents; i++) {
            if (officer->agents[i].is_active && 
                officer->agents[i].knowledge_level >= config.knowledge_threshold) {
                all_below_threshold = false;
                break;
            }
        }
        
        if (all_below_threshold && officer->num_agents > 1) {
            // Request info from another agent
            for (int i = 0; i < officer->num_agents; i++) {
                if (officer->agents[i].is_active && &officer->agents[i] != agent) {
                    request_information_from_agent(officer, i);
                    break;
                }
            }
        }
    } else {
        // Knowledge above threshold - gang involvement in criminal activities
        officer->knowledge_level += 0.3f;
        if (officer->knowledge_level > 1.0f) officer->knowledge_level = 1.0f;
        
        printf("POLICE: Officer %d detected criminal activity via agent intelligence\n", 
               officer->police_id);
        
        // Evaluate imprisonment
        evaluate_imprisonment_probability(officer);
    }
}

void evaluate_imprisonment_probability(PoliceOfficer* officer) {
    // Calculate imprisonment probability based on multiple factors
    float base_probability = 0.1f; // 10% base chance
    
    // Factor 1: Officer knowledge level
    float knowledge_factor = officer->knowledge_level * 0.4f; // Up to 40%
    
    // Factor 2: Number of active agents
    float agent_factor = (officer->num_agents / (float)config.max_agents_per_gang) * 0.3f; // Up to 30%
    
    // Factor 3: Agent knowledge levels
    float avg_agent_knowledge = 0.0f;
    int active_agents = 0;
    for (int i = 0; i < officer->num_agents; i++) {
        if (officer->agents[i].is_active) {
            avg_agent_knowledge += officer->agents[i].knowledge_level;
            active_agents++;
        }
    }
    if (active_agents > 0) {
        avg_agent_knowledge /= active_agents;
    }
    float agent_knowledge_factor = avg_agent_knowledge * 0.2f; // Up to 20%
    
    float total_probability = base_probability + knowledge_factor + agent_factor + agent_knowledge_factor;
    
    // Cap at 90%
    if (total_probability > 0.9f) total_probability = 0.9f;
    
    printf("POLICE: Officer %d imprisonment probability for gang %d: %.3f\n",
           officer->police_id, officer->gang_id_monitoring, total_probability);
    
    // Roll for imprisonment
    if (random_float(0, 1) < total_probability) {
        imprison_gang(officer);
    }
}

void imprison_gang(PoliceOfficer* officer) {
    // Send SIGUSR1 signal to gang process
    // Note: In a real implementation, you would need the gang process PID
    // For now, we'll just handle the imprisonment timing
    
    printf("POLICE: Officer %d imprisoning gang %d\n", 
           officer->police_id, officer->gang_id_monitoring);
    
    pthread_mutex_lock(&police_force.arrest_mutex);
    police_force.arrested_gangs[officer->gang_id_monitoring] = random_int(config.min_prison_period, config.max_prison_period); // 7-20 time units
    pthread_mutex_unlock(&police_force.arrest_mutex);
    
    // Mark plan as failed in shared memory
    // Gang *gang = &shm_ptrs.gangs[officer->gang_id_monitoring];
    // pthread_mutex_lock(&gang->gang_mutex);
    // gang->plan_success = -1;
    // gang->plan_in_progress = 0;
    LOCK_GAME_STATS();
    shm_ptrs.shared_game->num_thwarted_plans++;
    UNLOCK_GAME_STATS();
    // pthread_mutex_unlock(&gang->gang_mutex);
    
    printf("POLICE: Gang %d imprisoned for %d time units\n", 
           officer->gang_id_monitoring, police_force.arrested_gangs[officer->gang_id_monitoring]);
}

void take_police_action(PoliceOfficer* officer, ShmPtrs *shm_ptrs) {
    // Decision based on knowledge level
    if (officer->knowledge_level > config.knowledge_threshold) {
        evaluate_imprisonment_probability(officer);
    }
}


void handle_gang_release(PoliceOfficer* officer) {
    int gang_id = officer->gang_id_monitoring;

    printf("POLICE: Gang %d has been released from prison\n", gang_id);

    // Reset gang state in shared memory
    Gang *gang = &shm_ptrs.gangs[officer->gang_id_monitoring];
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
}

void handle_sigint(int signum) {
    printf("POLICE: Received SIGINT, shutting down\n");
    cleanup();
    exit(0);
}

void cleanup() {
    printf("POLICE: Cleaning up resources\n");

    shutdown_police_force();
    cleanup_semaphores();

    if (police_force.msgq_id != -1) {
        delete_message_queue(police_force.msgq_id);
    }

    if (shared_game != NULL && shared_game != MAP_FAILED) {
        if (munmap(shared_game, sizeof(Game)) == -1) {
            perror("munmap failed");
        }
    }
}

bool attempt_plant_agent_handshake(PoliceOfficer* officer, Config* config) {
    if (officer->num_agents >= config->max_agents_per_gang) {
        return false;
    }

    for (int attempt = 0; attempt < MAX_PLANT_ATTEMPTS; attempt++) {
        // Check success rate probability
        float random_value = random_float(0, 1);
        if (random_value > config->agent_success_rate) {
            printf("POLICE: Officer %d failed success rate check (attempt %d/%d)\n", 
                   officer->police_id, attempt + 1, MAX_PLANT_ATTEMPTS);
            continue;
        }

        // Send handshake message to gang
        Message handshake_msg;
        handshake_msg.mtype = get_gang_msgtype(config->max_agents_per_gang, (int8_t)officer->gang_id_monitoring);
        handshake_msg.mode = MSG_HANDSHAKE;
        handshake_msg.MessageContent.police_id = officer->police_id;

        printf("POLICE: Officer %d attempting handshake with gang %d (attempt %d/%d)\n",
               officer->police_id, officer->gang_id_monitoring, attempt + 1, MAX_PLANT_ATTEMPTS);

        if (send_message(officer->msgq_id, &handshake_msg) == 0) {
            // Wait for response from gang
            Message response;
            long response_type = get_police_msgtype(config->max_agents_per_gang, police_force.num_officers, (int8_t)officer->police_id);
            
            // Set timeout for response (e.g., 5 seconds)
            struct timespec timeout_start;
            clock_gettime(CLOCK_REALTIME, &timeout_start);
            
            bool received_response = false;
            for (int timeout_check = 0; timeout_check < 20; timeout_check++) { // 2 seconds total (reduced from 5)
                if (receive_message_nonblocking(officer->msgq_id, &response, response_type) == 0) {
                    received_response = true;
                    break;
                }
                usleep(100000); // 100ms
            }
            
            if (received_response) {
                // Gang responded with agent_id
                int new_agent_id = response.MessageContent.agent_id;

                // Add agent to officer's list
                AgentInfo *agent = &officer->agents[officer->num_agents];
                agent->agent_id = new_agent_id;
                agent->knowledge_level = 0.0f;
                agent->is_active = true;
                agent->last_report_time = time(NULL);
                officer->num_agents++;

                printf("POLICE: Officer %d successfully planted agent %d in gang %d\n", officer->police_id,
                       new_agent_id, officer->gang_id_monitoring);
                       
                // Sync the updated agent data to shared memory
                sync_police_data_to_shared_memory();
                return true;
            }

            printf("POLICE: Officer %d handshake timeout with gang %d (attempt %d/%d)\n", officer->police_id,
                   officer->gang_id_monitoring, attempt + 1, MAX_PLANT_ATTEMPTS);
        } else {
            printf("POLICE: Officer %d failed to send handshake to gang %d (attempt %d/%d)\n",
                   officer->police_id, officer->gang_id_monitoring, attempt + 1, MAX_PLANT_ATTEMPTS);
        }
    }
    
    printf("POLICE: Officer %d failed to plant agent in gang %d after %d attempts\n",
           officer->police_id, officer->gang_id_monitoring, MAX_PLANT_ATTEMPTS);
    return false;
}

void handle_agent_death_notification(PoliceOfficer* officer, Message* msg) {
    int dead_agent_id = msg->MessageContent.agent_id;
    
    printf("POLICE: Officer %d received death notification for agent %d\n",
           officer->police_id, dead_agent_id);
    
    // Find and deactivate the agent
    for (int i = 0; i < officer->num_agents; i++) {
        if (officer->agents[i].agent_id == dead_agent_id && officer->agents[i].is_active) {
            officer->agents[i].is_active = false;
            officer->agents[i].knowledge_level = 0.0f;
            
            printf("POLICE: Officer %d marked agent %d as inactive (dead)\n",
                   officer->police_id, dead_agent_id);
            
            // Sync the updated agent data to shared memory
            sync_police_data_to_shared_memory();
            
            // Compact the agents array to remove inactive agents
            for (int j = i; j < officer->num_agents - 1; j++) {
                officer->agents[j] = officer->agents[j + 1];
            }
            officer->num_agents--;
            
            // Clear the last slot
            officer->agents[officer->num_agents].agent_id = -1;
            officer->agents[officer->num_agents].knowledge_level = 0.0f;
            officer->agents[officer->num_agents].is_active = false;
            officer->agents[officer->num_agents].last_report_time = 0;
            
            break;
        }
    }
}

void request_information_from_agent(PoliceOfficer* officer, int agent_index) {
    if (agent_index >= officer->num_agents || !officer->agents[agent_index].is_active) {
        return;
    }
    
    Message request;
    request.mtype = get_agent_msgtype(config.max_agents_per_gang,
                                     (int8_t)officer->gang_id_monitoring, 
                                     (int8_t)officer->agents[agent_index].agent_id);
    request.mode = MSG_POLICE_REQUEST;
    
    if (send_message(officer->msgq_id, &request) == 0) {
        printf("POLICE: Officer %d requested information from agent %d\n",
               officer->police_id, officer->agents[agent_index].agent_id);
    } else {
        printf("POLICE: Officer %d failed to request information from agent %d\n",
               officer->police_id, officer->agents[agent_index].agent_id);
    }
}

void sync_police_data_to_shared_memory(void) {
    if (shared_game == NULL) return;
    
    // Lock shared memory for writing
    LOCK_GAME_STATS();
    
    // Copy police force data to shared memory
    shared_game->police_force.num_officers = police_force.num_officers;
    shared_game->police_force.shutdown_requested = police_force.shutdown_requested;
    shared_game->police_force.msgq_id = police_force.msgq_id;
    
    // Copy arrested gangs array
    memcpy(shared_game->police_force.arrested_gangs, police_force.arrested_gangs, 
           sizeof(int) * police_force.num_officers);
    
    // Copy officer data
    for (int i = 0; i < police_force.num_officers; i++) {
        PoliceOfficer *local_officer = &police_force.officers[i];
        PoliceOfficer *shared_officer = &shared_game->police_force.officers[i];
        
        shared_officer->police_id = local_officer->police_id;
        shared_officer->gang_id_monitoring = local_officer->gang_id_monitoring;
        shared_officer->is_active = local_officer->is_active;
        shared_officer->num_agents = local_officer->num_agents;
        shared_officer->knowledge_level = local_officer->knowledge_level;
        shared_officer->msgq_id = local_officer->msgq_id;
        
        // Copy agent information
        for (int j = 0; j < local_officer->num_agents; j++) {
            shared_officer->agents[j] = local_officer->agents[j];
        }
        
        // Clear unused agent slots
        for (int j = local_officer->num_agents; j < MAX_AGENTS_PER_GANG; j++) {
            shared_officer->agents[j].agent_id = -1;
            shared_officer->agents[j].knowledge_level = 0.0f;
            shared_officer->agents[j].is_active = false;
            shared_officer->agents[j].last_report_time = 0;
        }
    }
    
    UNLOCK_GAME_STATS();
}