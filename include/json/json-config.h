//
// Created by yazan on 5/17/2025.
//
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "gang.h"

TargetType get_target_type_from_name(const char* name);
AttributeType get_attribute_type_from_name(const char* name);
int load_targets_from_json(const char* filename, Target targets[]);
bool validate_target_weights(const Target* target);
void print_target(const Target* target);

#ifdef __cplusplus
    }
#endif