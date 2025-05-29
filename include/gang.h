//
// Created by yazan on 5/12/2025.
//

#ifndef GANG_H
#define GANG_H

#include <stdbool.h>
#include <pthread.h>
#include <stdint.h>

// Information spreading system
typedef enum {
    INFO_CORRECT,
    INFO_FALSE,
    INFO_PARTIAL
} InfoType;

typedef struct {
    InfoType type;
    float accuracy;     // 0.0 to 1.0, how accurate the information is
    int source_rank;    // Rank of the member who shared this info
    int timestamp;      // When this info was shared
} InformationPacket;


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
    bool is_alive;  // Whether the member is alive or not
    int gang_id;    // ID of the gang this member belongs to
    int rank;       // Rank of the member in the gang
    int XP;        // Experience points of the member
    int prep_contribution;
    int8_t agent_id; // ID of the agent (if any)
    float knowledge; // Knowledge level of the member (0.0 to 1.0)
    float suspicion; // Suspicion level of the agent
    float faithfulness; // Faithfulness level of the agent
    pthread_t thread; // Thread for the member
    float attributes[NUM_ATTRIBUTES];
    
    // Information spreading system
    InformationPacket received_info[5]; // Last 5 pieces of information received
    int info_count;                     // Number of information packets received
    float misinformation_level;         // How much false info this member has (0.0 to 1.0)
} Member;


typedef struct {
    int gang_id;
    int target_type;
    int prep_time;
    int prep_level;
    // Note: members pointer is NOT stored in shared memory - it's calculated locally in each process
    int num_alive_members; // Number of alive members in the gang
    int max_member_count; // max number of members
    int num_successful_plans; // Number of successful plans
    int num_thwarted_plans; // Number of thwarted plans
    int num_executed_agents; // Number of executed agents
    int num_agents; // Number of agents in the gang
    float notoriety; // Notoriety level of the gang
    float heat[NUM_TARGETS]; // Heat level for each target
    
    // Information spreading system
    int last_info_spread_time;           // Last time information was spread
    int info_spread_interval;            // Random interval for spreading info
    float leader_misinformation_chance;  // Chance leader spreads false info
    
    // Synchronization variables
    pthread_mutex_t gang_mutex;          // Mutex for accessing gang data
    pthread_cond_t prep_complete_cond;   // Condition variable to signal preparation completion
    pthread_cond_t plan_execute_cond;    // Condition variable to signal plan execution
    int members_ready;                   // Count of members who have completed preparation
    int plan_success;                    // Whether the plan succeeded (0=not determined, 1=success, -1=failure)
    int plan_in_progress;                // Whether a plan is currently in progress
} Gang;

// Target struct
typedef struct {
    TargetType type;
    char name[32];
    int heat; // this heat level is different from the gang's heat, it's how hot (pursued) the target is
    double weights[NUM_ATTRIBUTES];
} Target;

// Information spreading function declarations
void initialize_member_knowledge(Member* member, int rank, int max_rank);
void spread_information_in_gang(Gang* gang, int current_time, int leader_id);
void leader_spread_information(Gang* gang, int leader_id, int current_time);
void update_member_knowledge_from_info(Member* member);
InfoType determine_info_type(int source_rank, int target_rank, float misinformation_chance);
float calculate_base_knowledge_by_rank(int rank, int max_rank);
void share_information_between_members(Member* source, Member* target, int current_time);
void add_information_to_member(Member* member, InfoType type, float accuracy, int source_rank, int timestamp);

#endif //GANG_H
