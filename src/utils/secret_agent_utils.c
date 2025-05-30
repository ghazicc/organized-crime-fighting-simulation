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

void secret_agent_init(Game* shared_game, Member* member) {
    // Find the actual member in shared memory
    Member* shared_member = &shared_game->gangs[member->gang_id].members[member->member_id];
    // Initialize attributes in shared memory
    shared_member->knowledge = 0.0f;
    shared_member->suspicion = 0.0f;
    shared_member->faithfulness = (float)rand() / RAND_MAX;
    shared_member->discretion = 1.0f + ((float)rand() / RAND_MAX);
    shared_member->shrewdness = 1.0f + ((float)rand() / RAND_MAX);
    shared_member->askers_count = 0;
}

void secret_agent_record_asker(Game* shared_game,Config config, Member* agent, int asker_id) {
  Member* shared_agent = &shared_game->gangs[agent->gang_id].members[agent->member_id];
    if (shared_agent->askers_count < config.max_askers) {
        for (int i = 0;i<shared_agent->askers_count;i++) {
            if (shared_agent->askers[i] == asker_id) {
                return;
            }
        }
        shared_agent->askers[shared_agent->askers_count++] = asker_id;
    }
}

void secret_agent_ask_member(Game* shared_game,Member* agent,Member *target) {
    Member* shared_agent = &shared_game->gangs[agent->gang_id].members[agent->member_id];
    Member* shared_target = &shared_game->gangs[target->gang_id].members[target->member_id];

    if (shared_target->agent_id > 0) {
        float knowledge_change = shared_agent->shrewdness * (shared_target->knowledge - 0.5f);
        shared_agent->knowledge -= knowledge_change;

        if (shared_agent->knowledge < 0.0f) {
            shared_agent->knowledge = 0.0f;
        }
    }else {
            Gang* gang = &shared_game->gangs[shared_target->gang_id];
            int max_rank = 0;
            for (int i = 0; i < gang->max_member_count; i++) {
                if (gang->members[i].is_alive)
                    if (gang->members[i].rank > max_rank) {
                        max_rank = gang->members[i].rank;
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

void conduct_internal_investigation(Config config, Game* shared_game, int gang_id) {
    Gang *gang = &shared_game->gangs[gang_id];
    int max_rank = -1;
    int investigator_idx = -1;

    for (int i = 0; i < gang->max_member_count; i++) {
        if (gang->members[i].is_alive)
            if (gang->members[i].rank > max_rank) {
                max_rank = gang->members[i].rank;
                investigator_idx = i;
            }
    }
    if (investigator_idx == -1) {
        printf("Gang %d has no members to conduct investigation\n", gang_id);
        return;
    }

    Member* investigator = &gang->members[investigator_idx];

    for (int gidx =0;gidx<gang->max_member_count;gidx++) {
        if (gang->members[gidx].is_alive) {
            Member *g = &gang->members[gidx];

            if (g->agent_id>0) {
                float suspicion_increase = investigator->shrewdness*g->suspicion;
                g->suspicion += suspicion_increase;
            }


            for (int j = 0;j<g->askers_count;j++) {
                int asker_id = g->askers[j];
                for (int k = 0;k<gang->max_member_count;k++) {
                    Member *agent_candidate = &gang->members[k];
                    if (agent_candidate->member_id == asker_id) {
                        float suspicion_increase = investigator->shrewdness * (1.0f - g->suspicion);
                        agent_candidate->suspicion += suspicion_increase;
                    }
                }

            }


        }

    }

    for (int i = 0;i<gang->max_member_count;i++) {
        if (gang->members[i].is_alive) {
            Member *m = &gang->members[i];
            if (m->agent_id>0 && m->suspicion > config.suspicion_threshold) {
                shared_game->num_executed_agents++;

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

void secret_agent_handle_police_requests(Member* agent, Game* shared_game, int police_msgid, int police_id,Gang* gang,Config config) {
    Message msg;
    long agent_msgtype = get_agent_msgtype(gang->max_member_count, agent->gang_id, agent->member_id);
    while (1) {
        if (receive_message(police_msgid, &msg, agent_msgtype) == 0) {
            if (msg.mode == 2) { // police requests knowledge
                agent_report_knowledge(agent, shared_game, police_msgid, police_id,gang, config);
            }
        }
        usleep(100000);
    }
}
