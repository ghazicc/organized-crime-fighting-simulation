#include <stdio.h>
#include <string.h>
#include <json-c/json.h>
#include "json/json-config.h"
#include "gang.h"

/**
 * Maps a crime name from JSON to the corresponding TargetType enum
 *
 * @param name Crime name from JSON
 * @return Corresponding TargetType enum value, or -1 if not found
 */
TargetType get_target_type_from_name(const char* name) {
    if (strcmp(name, "bank_robbery") == 0) return TARGET_BANK_ROBBERY;
    if (strcmp(name, "jewelry_shop_robbery") == 0) return TARGET_JEWELRY_ROBBERY;
    if (strcmp(name, "drug_trafficking") == 0) return TARGET_DRUG_TRAFFICKING;
    if (strcmp(name, "art_theft") == 0) return TARGET_ART_THEFT;
    if (strcmp(name, "kidnapping") == 0) return TARGET_KIDNAPPING;
    if (strcmp(name, "blackmail") == 0) return TARGET_BLACKMAIL;
    if (strcmp(name, "arms_trafficking") == 0) return TARGET_ARMS_TRAFFICKING;
    
    return -1; // Unknown target type
}

/**
 * Maps an attribute name from JSON to the corresponding AttributeType enum
 *
 * @param name Attribute name from JSON
 * @return Corresponding AttributeType enum value, or -1 if not found
 */
AttributeType get_attribute_type_from_name(const char* name) {
    if (strcmp(name, "smartness") == 0) return ATTR_SMARTNESS;
    if (strcmp(name, "stealth") == 0) return ATTR_STEALTH;
    if (strcmp(name, "strength") == 0) return ATTR_STRENGTH;
    if (strcmp(name, "tech_skills") == 0) return ATTR_TECH_SKILLS;
    if (strcmp(name, "bravery") == 0) return ATTR_BRAVERY;
    if (strcmp(name, "negotiation") == 0) return ATTR_NEGOTIATION;
    if (strcmp(name, "networking") == 0) return ATTR_NETWORKING;
    
    return -1; // Unknown attribute type
}

/**
 * Loads all targets from the JSON configuration file
 *
 * @param filename Path to JSON configuration file
 * @param targets Array of Target structs to populate (must be at least TARGET_NUM in size)
 * @return 0 on success, negative value on failure
 */
int load_targets_from_json(const char* filename, Target targets[]) {
    json_object *targets_obj;
    int targets_loaded = 0;
    
    // Load and parse the JSON file
    json_object *root_obj = json_object_from_file(filename);
    if (!root_obj) {
        fprintf(stderr, "Error parsing JSON file: %s\n", filename);
        return -1;
    }
    
    // Get the "targets" object from the JSON
    if (!json_object_object_get_ex(root_obj, "targets", &targets_obj)) {
        fprintf(stderr, "Error: No 'targets' object found in JSON file\n");
        json_object_put(root_obj);
        return -2;
    }
    
    // Initialize all targets with zeroed weights (in case some are missing from JSON)
    for (int i = 0; i < TARGET_NUM; i++) {
        targets[i].type = i;
        memset(targets[i].weights, 0, sizeof(double) * NUM_ATTRIBUTES);
    }
    
    // Iterate through each target in the JSON
    json_object_object_foreach(targets_obj, target_name, target_obj) {
        // Get target type from name
        TargetType type = get_target_type_from_name(target_name);
        if (type < 0) {
            fprintf(stderr, "Warning: Unknown target type '%s' in JSON, skipping\n", target_name);
            continue;
        }
        
        // Set target name and type
        strncpy(targets[type].name, target_name, sizeof(targets[type].name) - 1);
        targets[type].name[sizeof(targets[type].name) - 1] = '\0'; // Ensure null termination
        targets[type].type = type;
        
        // Process each attribute weight
        json_object_object_foreach(target_obj, attr_name, attr_value) {
            // Get attribute type from name
            AttributeType attr_type = get_attribute_type_from_name(attr_name);
            if (attr_type < 0) {
                fprintf(stderr, "Warning: Unknown attribute '%s' for target '%s', skipping\n", 
                        attr_name, target_name);
                continue;
            }
            
            // Set attribute weight
            targets[type].weights[attr_type] = json_object_get_double(attr_value);
        }
        
        targets_loaded++;
    }
    
    // Clean up JSON object
    json_object_put(root_obj);
    
    printf("Successfully loaded %d targets from JSON\n", targets_loaded);
    return targets_loaded;
}

/**
 * Validates that target weights sum to approximately 1.0
 *
 * @param target Target to validate
 * @return true if valid, false otherwise
 */
bool validate_target_weights(const Target* target) {
    double sum = 0.0;
    
    for (int i = 0; i < NUM_ATTRIBUTES; i++) {
        sum += target->weights[i];
    }
    
    // Allow small floating point error (within 0.01)
    return (sum >= 0.99 && sum <= 1.01);
}

/**
 * Prints target weights for debugging
 *
 * @param target Target to print
 */
void print_target(const Target* target) {
    printf("Target: %s (Type: %d)\n", target->name, target->type);
    printf("  Weights:\n");
    printf("    Smartness:   %.2f\n", target->weights[ATTR_SMARTNESS]);
    printf("    Stealth:     %.2f\n", target->weights[ATTR_STEALTH]);
    printf("    Strength:    %.2f\n", target->weights[ATTR_STRENGTH]);
    printf("    Tech Skills: %.2f\n", target->weights[ATTR_TECH_SKILLS]);
    printf("    Bravery:     %.2f\n", target->weights[ATTR_BRAVERY]);
    printf("    Negotiation: %.2f\n", target->weights[ATTR_NEGOTIATION]);
    printf("    Networking:  %.2f\n", target->weights[ATTR_NETWORKING]);
    
    double sum = 0.0;
    for (int i = 0; i < NUM_ATTRIBUTES; i++) {
        sum += target->weights[i];
    }
    printf("    Sum: %.2f %s\n", sum, (validate_target_weights(target) ? "✓" : "✗"));
    printf("\n");
}