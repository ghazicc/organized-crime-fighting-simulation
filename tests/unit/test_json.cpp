//
// Created by yazan on 5/17/2025.
//
#include <gtest/gtest.h>
#include "json/json-config.h"
#include "gang.h"
#include <cstring>
#include <fstream>
#include <string>

class JsonConfigTest : public ::testing::Test {
protected:
    Target targets[NUM_TARGETS] = {};
    const char* test_json_path = "test_config.json";

    void SetUp() override {
        // Initialize targets array
        for (auto & target : targets) {
            memset(&target, 0, sizeof(Target));
        }
    }

    void TearDown() override {
        // Clean up test JSON file
        std::remove(test_json_path);
    }

    void createTestJsonFile(const std::string& content) const {
        std::ofstream file(test_json_path);
        file << content;
        file.close();
    }
};

// Test target type mapping
TEST_F(JsonConfigTest, TargetTypeMapping) {
    EXPECT_EQ(get_target_type_from_name("bank_robbery"), TARGET_BANK_ROBBERY);
    EXPECT_EQ(get_target_type_from_name("jewelry_shop_robbery"), TARGET_JEWELRY_ROBBERY);
    EXPECT_EQ(get_target_type_from_name("drug_trafficking"), TARGET_DRUG_TRAFFICKING);
    EXPECT_EQ(get_target_type_from_name("art_theft"), TARGET_ART_THEFT);
    EXPECT_EQ(get_target_type_from_name("kidnapping"), TARGET_KIDNAPPING);
    EXPECT_EQ(get_target_type_from_name("blackmail"), TARGET_BLACKMAIL);
    EXPECT_EQ(get_target_type_from_name("arms_trafficking"), TARGET_ARMS_TRAFFICKING);
    EXPECT_EQ(get_target_type_from_name("invalid_crime"), -1);
}

// Test attribute type mapping
TEST_F(JsonConfigTest, AttributeTypeMapping) {
    EXPECT_EQ(get_attribute_type_from_name("smartness"), ATTR_SMARTNESS);
    EXPECT_EQ(get_attribute_type_from_name("stealth"), ATTR_STEALTH);
    EXPECT_EQ(get_attribute_type_from_name("strength"), ATTR_STRENGTH);
    EXPECT_EQ(get_attribute_type_from_name("tech_skills"), ATTR_TECH_SKILLS);
    EXPECT_EQ(get_attribute_type_from_name("bravery"), ATTR_BRAVERY);
    EXPECT_EQ(get_attribute_type_from_name("negotiation"), ATTR_NEGOTIATION);
    EXPECT_EQ(get_attribute_type_from_name("networking"), ATTR_NETWORKING);
    EXPECT_EQ(get_attribute_type_from_name("invalid_attr"), -1);
}

// Test loading valid JSON
TEST_F(JsonConfigTest, LoadValidJsonFile) {
    std::string content = R"({
        "targets": {
            "bank_robbery": {
                "smartness": 0.10,
                "stealth": 0.10,
                "strength": 0.10,
                "tech_skills": 0.40,
                "bravery": 0.20,
                "negotiation": 0.05,
                "networking": 0.05
            },
            "art_theft": {
                "smartness": 0.10,
                "stealth": 0.50,
                "strength": 0.05,
                "tech_skills": 0.30,
                "bravery": 0.02,
                "negotiation": 0.02,
                "networking": 0.01
            }
        }
    })";

    createTestJsonFile(content);

    int result = load_targets_from_json(test_json_path, targets);
    EXPECT_EQ(result, 2);

    // Verify bank_robbery values
    EXPECT_STREQ(targets[TARGET_BANK_ROBBERY].name, "bank_robbery");
    EXPECT_EQ(targets[TARGET_BANK_ROBBERY].type, TARGET_BANK_ROBBERY);
    EXPECT_DOUBLE_EQ(targets[TARGET_BANK_ROBBERY].weights[ATTR_SMARTNESS], 0.10);
    EXPECT_DOUBLE_EQ(targets[TARGET_BANK_ROBBERY].weights[ATTR_STEALTH], 0.10);
    EXPECT_DOUBLE_EQ(targets[TARGET_BANK_ROBBERY].weights[ATTR_TECH_SKILLS], 0.40);

    // Verify art_theft values
    EXPECT_STREQ(targets[TARGET_ART_THEFT].name, "art_theft");
    EXPECT_DOUBLE_EQ(targets[TARGET_ART_THEFT].weights[ATTR_STEALTH], 0.50);
}

// Test loading nonexistent file
TEST_F(JsonConfigTest, LoadNonExistentFile) {
    int result = load_targets_from_json("nonexistent.json", targets);
    EXPECT_LT(result, 0);
}
//
// // Test loading malformed JSON
// TEST_F(JsonConfigTest, LoadMalformedJsonFile) {
//     std::string content = R"({
//         "targets": {
//             "bank_robbery": {
//                 "smartness": 0.10,
//             } // Invalid trailing comma
//         }
//     })";
//
//     createTestJsonFile(content);
//     int result = load_targets_from_json(test_json_path, targets);
//     EXPECT_LT(result, 0);
// }

// Test target weight validation
TEST_F(JsonConfigTest, ValidateTargetWeights) {
    Target valid_target;
    valid_target.weights[ATTR_SMARTNESS] = 0.1;
    valid_target.weights[ATTR_STEALTH] = 0.2;
    valid_target.weights[ATTR_STRENGTH] = 0.1;
    valid_target.weights[ATTR_TECH_SKILLS] = 0.2;
    valid_target.weights[ATTR_BRAVERY] = 0.2;
    valid_target.weights[ATTR_NEGOTIATION] = 0.1;
    valid_target.weights[ATTR_NETWORKING] = 0.1;

    EXPECT_TRUE(validate_target_weights(&valid_target));

    Target invalid_target;
    invalid_target.weights[ATTR_SMARTNESS] = 0.2;
    invalid_target.weights[ATTR_STEALTH] = 0.2;
    invalid_target.weights[ATTR_STRENGTH] = 0.2;
    invalid_target.weights[ATTR_TECH_SKILLS] = 0.4;
    invalid_target.weights[ATTR_BRAVERY] = 0.2;
    invalid_target.weights[ATTR_NEGOTIATION] = 0.1;
    invalid_target.weights[ATTR_NETWORKING] = 0.1;

    EXPECT_FALSE(validate_target_weights(&invalid_target));
}

// Test with actual config.json file
TEST_F(JsonConfigTest, LoadActualConfigJson) {
    // Create a copy of the actual config for testing
    std::string content = R"({
        "targets": {
            "bank_robbery": {
                "smartness": 0.10,
                "stealth": 0.10,
                "strength": 0.10,
                "tech_skills": 0.40,
                "bravery": 0.20,
                "negotiation": 0.05,
                "networking": 0.05
            },
            "jewelry_shop_robbery": {
                "smartness": 0.10,
                "stealth": 0.45,
                "strength": 0.10,
                "tech_skills": 0.20,
                "bravery": 0.10,
                "negotiation": 0.03,
                "networking": 0.02
            },
            "drug_trafficking": {
                "smartness": 0.10,
                "stealth": 0.05,
                "strength": 0.05,
                "tech_skills": 0.05,
                "bravery": 0.10,
                "negotiation": 0.35,
                "networking": 0.30
            },
            "art_theft": {
                "smartness": 0.10,
                "stealth": 0.50,
                "strength": 0.05,
                "tech_skills": 0.30,
                "bravery": 0.02,
                "negotiation": 0.02,
                "networking": 0.01
            },
            "kidnapping": {
                "smartness": 0.05,
                "stealth": 0.05,
                "strength": 0.25,
                "tech_skills": 0.05,
                "bravery": 0.30,
                "negotiation": 0.25,
                "networking": 0.05
            },
            "blackmail": {
                "smartness": 0.10,
                "stealth": 0.05,
                "strength": 0.02,
                "tech_skills": 0.18,
                "bravery": 0.05,
                "negotiation": 0.40,
                "networking": 0.20
            },
            "arms_trafficking": {
                "smartness": 0.10,
                "stealth": 0.05,
                "strength": 0.20,
                "tech_skills": 0.05,
                "bravery": 0.10,
                "negotiation": 0.20,
                "networking": 0.30
            }
        }
    })";

    createTestJsonFile(content);
    int result = load_targets_from_json(test_json_path, targets);
    EXPECT_GT(result, 0);

    // Verify bank_robbery matches config.json
    EXPECT_STREQ(targets[TARGET_BANK_ROBBERY].name, "bank_robbery");
    EXPECT_DOUBLE_EQ(targets[TARGET_BANK_ROBBERY].weights[ATTR_SMARTNESS], 0.10);
    EXPECT_DOUBLE_EQ(targets[TARGET_BANK_ROBBERY].weights[ATTR_STEALTH], 0.10);
    EXPECT_DOUBLE_EQ(targets[TARGET_BANK_ROBBERY].weights[ATTR_STRENGTH], 0.10);
    EXPECT_DOUBLE_EQ(targets[TARGET_BANK_ROBBERY].weights[ATTR_TECH_SKILLS], 0.40);
    EXPECT_DOUBLE_EQ(targets[TARGET_BANK_ROBBERY].weights[ATTR_BRAVERY], 0.20);
    EXPECT_DOUBLE_EQ(targets[TARGET_BANK_ROBBERY].weights[ATTR_NEGOTIATION], 0.05);
    EXPECT_DOUBLE_EQ(targets[TARGET_BANK_ROBBERY].weights[ATTR_NETWORKING], 0.05);



    EXPECT_STREQ(targets[TARGET_JEWELRY_ROBBERY].name, "jewelry_shop_robbery");
    EXPECT_DOUBLE_EQ(targets[TARGET_JEWELRY_ROBBERY].weights[ATTR_SMARTNESS], 0.10);
    EXPECT_DOUBLE_EQ(targets[TARGET_JEWELRY_ROBBERY].weights[ATTR_STEALTH], 0.45);
    EXPECT_DOUBLE_EQ(targets[TARGET_JEWELRY_ROBBERY].weights[ATTR_STRENGTH], 0.10);
    EXPECT_DOUBLE_EQ(targets[TARGET_JEWELRY_ROBBERY].weights[ATTR_TECH_SKILLS], 0.20);
    EXPECT_DOUBLE_EQ(targets[TARGET_JEWELRY_ROBBERY].weights[ATTR_BRAVERY], 0.10);
    EXPECT_DOUBLE_EQ(targets[TARGET_JEWELRY_ROBBERY].weights[ATTR_NEGOTIATION], 0.03);
    EXPECT_DOUBLE_EQ(targets[TARGET_JEWELRY_ROBBERY].weights[ATTR_NETWORKING], 0.02);

    // Verify all targets have valid weights
    for (auto & target : targets) {
        if (strlen(target.name) > 0) {
            EXPECT_TRUE(validate_target_weights(&target))
                << "Target " << target.name << " has invalid weights";
        }
    }
}