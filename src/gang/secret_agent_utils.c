#include "secret_agent_utils.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include "config.h"
#include "game.h"
#include "gang.h"
#include "shared_mem_utils.h"
#include "message.h"
#include <unistd.h>
#include <time.h>

// External variables
extern int police_msgq_id;

void secret_agent_init(ShmPtrs* shm_ptrs, Member* member) {
    // Find the actual member in shared memory
    Member* shared_member = &shm_ptrs->gang_members[member->gang_id][member->member_id];
    // Initialize attributes in shared memory
    shared_member->knowledge = 0.0f;
    shared_member->suspicion = 0.0f;
    shared_member->faithfulness = (float)rand() / RAND_MAX;
    shared_member->discretion = 1.0f + ((float)rand() / RAND_MAX);
    shared_member->shrewdness = 1.0f + ((float)rand() / RAND_MAX);
    shared_member->askers_count = 0;
}

void secret_agent_record_asker(ShmPtrs* shm_ptrs,Config config, Member* agent, int asker_id) {
  Member* shared_agent = &shm_ptrs->gang_members[agent->gang_id][agent->member_id];
    if (shared_agent->askers_count < config.max_askers) {
        for (int i = 0;i<shared_agent->askers_count;i++) {
            if (shared_agent->askers[i] == asker_id) {
                return;
            }
        }
        shared_agent->askers[shared_agent->askers_count++] = asker_id;
    }
}

void secret_agent_ask_member(ShmPtrs* shm_ptrs,Member* agent,Member *target) {
    Member* shared_agent = &shm_ptrs->gang_members[agent->gang_id][agent->member_id];
    Member* shared_target = &shm_ptrs->gang_members[target->gang_id][target->member_id];

    if (shared_target->agent_id > 0) {
        float knowledge_change = shared_agent->shrewdness * (shared_target->knowledge - 0.5f);
        shared_agent->knowledge -= knowledge_change;

        if (shared_agent->knowledge < 0.0f) {
            shared_agent->knowledge = 0.0f;
        }
    }else {
            Gang* gang = &shm_ptrs->gangs[shared_target->gang_id];
            int max_rank = 0;
            for (int i = 0; i < gang->max_member_count; i++) {
                if (shm_ptrs->gang_members[gang->gang_id][i].is_alive)
                    if (shm_ptrs->gang_members[gang->gang_id][i].rank > max_rank) {
                        max_rank = shm_ptrs->gang_members[gang->gang_id][i].rank;
                    }
            }

            // Calculate suspicion increase (equation 2)
            float rank_ratio = (float)shared_agent->rank / (float)(max_rank > 0 ? max_rank : 1);
            float suspicion_increase = shared_agent->shrewdness * (1.0f - rank_ratio * shared_agent->discretion);
            shared_agent->suspicion += suspicion_increase;

            float target_knowledge = (float)shared_target->rank / (float)(max_rank > 0 ? max_rank : 1);
            float knowledge_change = shared_agent->shrewdness * (target_knowledge - 0.5f);
            shared_agent->knowledge += knowledge_change;

            if (shared_agent->knowledge < 0.0f) {
                shared_agent->knowledge = 0.0f;
            } else if (shared_agent->knowledge > 1.0f) {
                shared_agent->knowledge = 1.0f;
            }

        }


    }

void conduct_internal_investigation(Config config, ShmPtrs* shm_ptrs, int gang_id) {
    Gang *gang = &shm_ptrs->gangs[gang_id];;
    int max_rank = -1;
    int investigator_idx = -1;

    for (int i = 0; i < gang->max_member_count; i++) {
        if (shm_ptrs->gang_members[gang->gang_id][i].is_alive)
            if (shm_ptrs->gang_members[gang->gang_id][i].rank > max_rank) {
                max_rank = shm_ptrs->gang_members[gang->gang_id][i].rank;
                investigator_idx = i;
            }
    }
    if (investigator_idx == -1) {
        printf("Gang %d has no members to conduct investigation\n", gang_id);
        return;
    }

    Member* investigator = &shm_ptrs->gang_members[gang->gang_id][investigator_idx];

    for (int gidx =0;gidx<gang->max_member_count;gidx++) {
        if (shm_ptrs->gang_members[gang->gang_id][gidx].is_alive) {
            Member *g = &shm_ptrs->gang_members[gang->gang_id][gidx];
            if (g->agent_id>0) {
                float suspicion_increase = investigator->shrewdness*g->suspicion;
                g->suspicion += suspicion_increase;
            }


            for (int j = 0;j<g->askers_count;j++) {
                int asker_id = g->askers[j];
                for (int k = 0;k<gang->max_member_count;k++) {
                    Member *agent_candidate = &shm_ptrs->gang_members[gang->gang_id][k];
                    if (agent_candidate->member_id == asker_id) {
                        float suspicion_increase = investigator->shrewdness * (1.0f - g->suspicion);
                        agent_candidate->suspicion += suspicion_increase;
                    }
                }

            }


        }

    }

    // Execute agents with high suspicion and notify police
    for (int i = 0; i < gang->max_member_count; i++) {
        if (shm_ptrs->gang_members[gang->gang_id][i].is_alive) {
            Member *m = &shm_ptrs->gang_members[gang->gang_id][i];
            if (m->agent_id >= 0 && m->suspicion > config.suspicion_threshold) {
                // Execute the agent
                m->is_alive = false;
                gang->num_alive_members--;
                gang->num_agents--;
                shm_ptrs->shared_game->num_executed_agents++;
                
                printf("Gang %d: Executed agent %d (suspicion: %.2f > threshold: %.2f)\n",
                       gang->gang_id, m->agent_id, m->suspicion, config.suspicion_threshold);
                fflush(stdout);
                
                // Notify police about agent death (assuming police_id matches gang_id for simplicity)
                // In a real implementation, you might need to track which police planted this agent
                int police_id = gang->gang_id; // Simple mapping for now
                extern int police_msgq_id;
                notify_police_agent_death(police_msgq_id, gang->gang_id, m->agent_id, police_id, gang, config);
            }
        }
    }

}

void police_request_agent_knowledge(int police_msgid, Game* shared_game, int8_t gang_id, int8_t agent_id, Config config,Gang* gang) {
    Message msg;
    msg.mtype = get_agent_msgtype(gang->max_member_count, gang_id, agent_id);
    msg.mode = 2; // request knowledge
    send_message(police_msgid, &msg);
}

void agent_report_knowledge(Member* agent, Game* shared_game, int police_msgid, int police_id,Gang* gang,Config config) {
    Message msg;
    msg.mtype = get_police_msgtype(gang->max_member_count, config.max_gangs, police_id);
    msg.mode = 3; // report knowledge
    msg.MessageContent.knowledge = agent->knowledge;
    send_message(police_msgid, &msg);
}

void secret_agent_handle_police_requests(Member* agent, Game* shared_game, int police_msgid, int police_id, Gang* gang, Config config) {
    Message msg;
    long agent_msgtype = get_agent_msgtype(gang->max_member_count, agent->gang_id, agent->member_id);
    
    // Check for police requests (non-blocking)
    if (receive_message_nonblocking(police_msgid, &msg, agent_msgtype) == 0) {
        if (msg.mode == MSG_POLICE_REQUEST) { // police requests knowledge
            printf("Gang %d, Agent %d: Received police request for knowledge\n", 
                   agent->gang_id, agent->member_id);
            fflush(stdout);
            agent_report_knowledge(agent, shared_game, police_msgid, police_id, gang, config);
        }
    }
}

void secret_agent_periodic_communication(ShmPtrs* shm_ptrs, Member* agent, Game* shared_game, int police_msgid, int police_id, Gang* gang, Config config) {
    // Get agent from shared memory
    Member* shared_agent = &shm_ptrs->gang_members[agent->gang_id][agent->member_id];
    
    // Check if knowledge is above threshold for immediate reporting
    if (shared_agent->knowledge > config.knowledge_threshold) {
        printf("Gang %d, Agent %d: Knowledge %.2f above threshold %.2f - reporting gang involvement\n",
               agent->gang_id, agent->member_id, shared_agent->knowledge, config.knowledge_threshold);
        fflush(stdout);
        
        Message msg;
        msg.mtype = get_police_msgtype(gang->max_member_count, config.num_gangs, police_id);
        msg.mode = MSG_POLICE_REPORT;
        msg.MessageContent.knowledge = shared_agent->knowledge;
        
        if (send_message(police_msgid, &msg) == 0) {
            printf("Gang %d, Agent %d: Successfully reported high knowledge to police\n",
                   agent->gang_id, agent->member_id);
            fflush(stdout);
        }
    } else {
        // Periodic report of current knowledge level (lower priority)
        printf("Gang %d, Agent %d: Sending periodic knowledge report %.2f to police\n",
               agent->gang_id, agent->member_id, shared_agent->knowledge);
        fflush(stdout);
        
        Message msg;
        msg.mtype = get_police_msgtype(gang->max_member_count, config.num_gangs, police_id);
        msg.mode = MSG_POLICE_REPORT;
        msg.MessageContent.knowledge = shared_agent->knowledge;
        
        send_message(police_msgid, &msg);
    }
}

void notify_police_agent_death(int police_msgid, int gang_id, int agent_id, int police_id, Gang* gang, Config config) {
    Message msg;
    msg.mtype = get_police_msgtype(gang->max_member_count, config.num_gangs, police_id);
    msg.mode = MSG_AGENT_DEATH;
    msg.MessageContent.agent_id = agent_id;
    
    if (send_message(police_msgid, &msg) == 0) {
        printf("Gang %d: Notified police %d about death of agent %d\n", 
               gang_id, police_id, agent_id);
        fflush(stdout);
    }
}
