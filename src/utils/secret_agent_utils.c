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

void secret_agent_record_asker(Game* shared_game,Member* agent, int asker_id) {
  Member* shared_agent = &shared_game->gangs[agent->gang_id].members[agent->member_id];
    if (shared_agent->askers_count < MAX_ASKERS) {
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
            for (int i = 0; i < gang->members_count; i++) {
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








