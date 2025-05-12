//
// Created by yazan on 5/12/2025.
//

#ifndef GANG_H
#define GANG_H

#include <stdbool.h>
#include <pthread.h>


typedef struct {
    int member_id;  // Unique ID for each member
    int gang_id;    // ID of the gang this member belongs to
    int rank;       // Rank of the member in the gang
    int prep_contribution;
    bool is_agent;
    float suspicion_level; // Suspicion level of the agent
    pthread_t thread; // Thread for the member
} Member;


typedef struct {
    int gang_id;
    int target_type;
    int prep_time;
    int prep_level;
    Member *members;
    int members_count;
} Gang;

#endif //GANG_H
