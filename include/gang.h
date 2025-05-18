//
// Created by yazan on 5/12/2025.
//

#ifndef GANG_H
#define GANG_H

#include <stdbool.h>
#include <pthread.h>
#include <stdint.h>


typedef enum {
    TARGET_BANK_ROBBERY,
    TARGET_JEWELRY_ROBBERY,
    TARGET_DRUG_TRAFFICKING,
    TARGET_ART_THEFT,
    TARGET_KIDNAPPING,
    TARGET_BLACKMAIL,
    TARGET_ARMS_TRAFFICKING,
    NUM_TARGETS
} TargetType;


// Attribute type enum
typedef enum {
    ATTR_SMARTNESS,
    ATTR_STEALTH,
    ATTR_STRENGTH,
    ATTR_TECH_SKILLS,
    ATTR_BRAVERY,
    ATTR_NEGOTIATION,
    ATTR_NETWORKING,
    NUM_ATTRIBUTES
} AttributeType;

typedef struct {
    int member_id;  // Unique ID for each member
    int gang_id;    // ID of the gang this member belongs to
    int rank;       // Rank of the member in the gang
    int XP;        // Experience points of the member
    int prep_contribution;
    int8_t agent_id; // ID of the agent (if any)
    float knowledge; // Knowledge level of the member
    float suspicion; // Suspicion level of the agent
    float faithfulness; // Faithfulness level of the agent
    pthread_t thread; // Thread for the member
    float attributes[NUM_ATTRIBUTES];
    
} Member;


typedef struct {
    int gang_id;
    int target_type;
    int prep_time;
    int prep_level;
    Member *members;
    int members_count;
    int num_successful_plans; // Number of successful plans
    int num_thwarted_plans; // Number of thwarted plans
    int num_executed_agents; // Number of executed agents
    int num_agents; // Number of agents in the gang
    float notoriety; // Notoriety level of the gang
    float heat[NUM_TARGETS]; // Heat level for each target
} Gang;

// Target struct
typedef struct {
    TargetType type;
    char name[32];
    int heat; // this heat level is different from the gang's heat, it's how hot (pursued) the target is
    double weights[NUM_ATTRIBUTES];
} Target;

#endif //GANG_H
