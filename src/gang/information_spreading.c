//
// Information Spreading System Implementation
// This system simulates how information flows within gang hierarchies
//

#include "gang.h"
#include "random.h"
#include <stdio.h>
#include <string.h>
#include <math.h>

// Initialize knowledge level based on rank
void initialize_member_knowledge(Member* member, int rank, int max_rank) {
    // Higher rank = higher base knowledge
    member->knowledge = calculate_base_knowledge_by_rank(rank, max_rank);
    member->info_count = 0;
    member->misinformation_level = 0.0f;
    
    // Initialize all information packets
    for (int i = 0; i < 5; i++) {
        member->received_info[i].type = INFO_CORRECT;
        member->received_info[i].accuracy = 1.0f;
        member->received_info[i].source_rank = -1;
        member->received_info[i].timestamp = 0;
    }
    
    printf("Gang %d, Member %d: Initialized knowledge %.2f (rank %d/%d)\n", 
           member->gang_id, member->member_id, member->knowledge, rank, max_rank);
}

// Calculate base knowledge level based on rank
float calculate_base_knowledge_by_rank(int rank, int max_rank) {
    if (max_rank <= 0) return 0.5f;
    
    // Higher rank gets exponentially more knowledge
    float rank_ratio = (float)rank / (float)max_rank;
    float base_knowledge = 0.2f + (rank_ratio * rank_ratio) * 0.7f; // 0.2 to 0.9 range
    
    // Add some randomness
    base_knowledge += random_normal(0.0f, 0.1f);
    
    // Clamp to valid range
    if (base_knowledge < 0.1f) base_knowledge = 0.1f;
    if (base_knowledge > 1.0f) base_knowledge = 1.0f;
    
    return base_knowledge;
}

// Main information spreading function
void spread_information_in_gang(Gang* gang, int current_time, int leader_id) {
    // Check if it's time to spread information
    if (current_time - gang->last_info_spread_time < gang->info_spread_interval) {
        return; // Not time yet
    }
    
    printf("Gang %d: Information spreading session at time %d\n", gang->gang_id, current_time);
    
    // Leader spreads information to subordinates
    leader_spread_information(gang, leader_id, current_time);
    
    // Regular information flow from higher to lower ranks
    for (int source = 0; source < gang->num_alive_members; source++) {
        if (!gang->members[source].is_alive) continue;
        
        for (int target = 0; target < gang->num_alive_members; target++) {
            if (!gang->members[target].is_alive || source == target) continue;
            
            // Information flows from higher rank to lower rank
            if (gang->members[source].rank > gang->members[target].rank) {
                // Chance to share information based on rank difference
                int rank_diff = gang->members[source].rank - gang->members[target].rank;
                float share_chance = 0.7f / (1.0f + rank_diff * 0.3f); // Decreases with rank gap
                
                if (random_float(0.0f, 1.0f) < share_chance) {
                    // Share information
                    share_information_between_members(&gang->members[source], &gang->members[target], current_time);
                }
            }
        }
    }
    
    // Update knowledge levels based on received information
    for (int i = 0; i < gang->num_alive_members; i++) {
        if (gang->members[i].is_alive) {
            update_member_knowledge_from_info(&gang->members[i]);
        }
    }
    
    // Set next information spreading time
    gang->last_info_spread_time = current_time;
    gang->info_spread_interval = random_int(5, 15); // Random interval between 5-15 time units
    
    printf("Gang %d: Next information spread in %d time units\n", gang->gang_id, gang->info_spread_interval);
}

// Leader spreads information (may include misinformation)
void leader_spread_information(Gang* gang, int leader_id, int current_time) {
    Member* leader = &gang->members[leader_id];
    
    printf("Gang %d: Leader (Member %d, rank %d) spreading information\n", 
           gang->gang_id, leader_id, leader->rank);
    
    // Determine if leader spreads false information
    bool spread_false_info = random_float(0.0f, 1.0f) < gang->leader_misinformation_chance;
    
    if (spread_false_info) {
        printf("Gang %d: Leader spreading MISINFORMATION!\n", gang->gang_id);
    }
    
    // Share information with all subordinates
    for (int i = 0; i < gang->num_alive_members; i++) {
        if (!gang->members[i].is_alive || i == leader_id) continue;
        
        // Only share with lower-ranked members
        if (gang->members[i].rank < leader->rank) {
            InfoType info_type = spread_false_info ? INFO_FALSE : INFO_CORRECT;
            float accuracy = spread_false_info ? random_float(0.1f, 0.4f) : random_float(0.8f, 1.0f);
            
            add_information_to_member(&gang->members[i], info_type, accuracy, leader->rank, current_time);
            
            printf("Gang %d: Leader shared %s info (accuracy: %.2f) with Member %d\n", 
                   gang->gang_id, 
                   info_type == INFO_FALSE ? "FALSE" : "CORRECT", 
                   accuracy, i);
        }
    }
}

// Share information between two specific members
void share_information_between_members(Member* source, Member* target, int current_time) {
    // Determine information type based on source's knowledge and misinformation level
    InfoType info_type = determine_info_type(source->rank, target->rank, source->misinformation_level);
    
    // Calculate accuracy based on source's knowledge and rank difference
    float rank_factor = 1.0f - (abs(source->rank - target->rank) * 0.1f);
    float accuracy = source->knowledge * rank_factor * (1.0f - source->misinformation_level);
    
    // Clamp accuracy
    if (accuracy < 0.1f) accuracy = 0.1f;
    if (accuracy > 1.0f) accuracy = 1.0f;
    
    add_information_to_member(target, info_type, accuracy, source->rank, current_time);
    
    printf("Gang %d: Member %d (rank %d) shared info with Member %d (rank %d) - Type: %s, Accuracy: %.2f\n",
           source->gang_id, source->member_id, source->rank, 
           target->member_id, target->rank,
           info_type == INFO_CORRECT ? "CORRECT" : (info_type == INFO_FALSE ? "FALSE" : "PARTIAL"),
           accuracy);
}

// Add information packet to a member
void add_information_to_member(Member* member, InfoType type, float accuracy, int source_rank, int timestamp) {
    // Shift existing information (FIFO queue)
    for (int i = 4; i > 0; i--) {
        member->received_info[i] = member->received_info[i-1];
    }
    
    // Add new information at the front
    member->received_info[0].type = type;
    member->received_info[0].accuracy = accuracy;
    member->received_info[0].source_rank = source_rank;
    member->received_info[0].timestamp = timestamp;
    
    if (member->info_count < 5) {
        member->info_count++;
    }
}

// Determine what type of information to share
InfoType determine_info_type(int source_rank, int target_rank, float misinformation_chance) {
    // Higher-ranked members are more likely to share accurate info
    float accuracy_chance = 0.8f + (source_rank * 0.05f) - misinformation_chance;
    
    float rand_val = random_float(0.0f, 1.0f);
    
    if (rand_val < accuracy_chance) {
        return INFO_CORRECT;
    } else if (rand_val < accuracy_chance + 0.15f) {
        return INFO_PARTIAL;
    } else {
        return INFO_FALSE;
    }
}

// Update member's knowledge based on received information
void update_member_knowledge_from_info(Member* member) {
    if (member->info_count == 0) return;
    
    float total_weight = 0.0f;
    float weighted_accuracy = 0.0f;
    float misinformation_accumulation = 0.0f;
    
    // Process all received information
    for (int i = 0; i < member->info_count; i++) {
        InformationPacket* info = &member->received_info[i];
        
        // Weight recent information more heavily
        float time_weight = 1.0f / (1.0f + i * 0.2f);
        
        // Weight information from higher-ranked sources more heavily
        float rank_weight = 1.0f + (info->source_rank * 0.1f);
        
        float total_info_weight = time_weight * rank_weight;
        total_weight += total_info_weight;
        
        if (info->type == INFO_CORRECT) {
            weighted_accuracy += info->accuracy * total_info_weight;
        } else if (info->type == INFO_FALSE) {
            weighted_accuracy += (1.0f - info->accuracy) * total_info_weight * 0.3f; // False info hurts knowledge
            misinformation_accumulation += (1.0f - info->accuracy) * total_info_weight * 0.5f;
        } else { // INFO_PARTIAL
            weighted_accuracy += info->accuracy * total_info_weight * 0.7f; // Partial info is less valuable
        }
    }
    
    if (total_weight > 0.0f) {
        // Calculate new knowledge level
        float info_contribution = weighted_accuracy / total_weight;
        
        // Blend with existing knowledge (80% existing, 20% new info)
        member->knowledge = member->knowledge * 0.8f + info_contribution * 0.2f;
        
        // Update misinformation level
        member->misinformation_level = member->misinformation_level * 0.9f + (misinformation_accumulation / total_weight) * 0.1f;
        
        // Clamp values
        if (member->knowledge < 0.0f) member->knowledge = 0.0f;
        if (member->knowledge > 1.0f) member->knowledge = 1.0f;
        if (member->misinformation_level < 0.0f) member->misinformation_level = 0.0f;
        if (member->misinformation_level > 1.0f) member->misinformation_level = 1.0f;
        
        printf("Gang %d, Member %d: Knowledge updated to %.3f, Misinformation: %.3f\n",
               member->gang_id, member->member_id, member->knowledge, member->misinformation_level);
    }
}
