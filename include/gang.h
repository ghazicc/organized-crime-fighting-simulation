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
    int8_t agent_id; // ID of the agent (if any)
    float knowledge; // Knowledge level of the member
    float suspicion; // Suspicion level of the agent
    float faithfulness; // Faithfulness level of the agent
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

typedef enum {
    TARGET_BANK_ROBBERY,
    TARGET_JEWELRY_ROBBERY,
    TARGET_DRUG_TRAFFICKING,
    TARGET_ART_THEFT,
    TARGET_KIDNAPPING,
    TARGET_BLACKMAIL,
    TARGET_ARMS_TRAFFICKING
} TargetType;

#endif //GANG_H
