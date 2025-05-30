//
// Created by yazan on 5/12/2025.
//

#include <gtest/gtest.h>
#include "config.h"
#include <cstring>
#include <fstream>
#include <string>

class ConfigTest : public ::testing::Test {
protected:
    Config config{};
    const char* test_config_path = "test_config.txt";

    void SetUp() override {
        // Initialize config with invalid values to ensure they're properly overwritten
        config.max_thwarted_plans = -1;
        config.max_successful_plans = -1;
        config.max_executed_agents = -1;
        config.max_agents_per_gang = -1;
        config.min_gangs = -1;
        config.max_gangs = -1;
        config.min_gang_size = -1;
        config.max_gang_size = -1;
        config.suspicion_threshold = -1.0f;
        config.agent_success_rate = -1.0f;
        config.prison_period = -1;
        config.num_ranks = -1;
        config.min_time_prepare = -1;
        config.max_time_prepare = -1;
        config.min_level_prepare = -1;
        config.max_level_prepare = -1;
        config.death_probability = -1.0f;
        config.difficulty_level = -1;
        config.max_difficulty = -1;
        config.timeout_period = -1;
        config.knowledge_threshold = -1;
    }

    void TearDown() override {
        // Clean up test config file if it exists
        std::remove(test_config_path);
    }

    // Helper to create a test config file with specified content
    void createTestConfigFile(const std::string& content) const {
        std::ofstream file(test_config_path);
        file << content;
        file.close();
    }
};

TEST_F(ConfigTest, LoadValidConfigFile) {
    // Create a test config with known values
    std::string content =
        "max_thwarted_plans=4\n"
        "max_successful_plans=5\n"
        "max_executed_agents=6\n"
        "max_agents_per_gang=4\n"
        "min_gangs=3\n"
        "max_gangs=12\n"
        "min_gang_size=4\n"
        "max_gang_size=15\n"
        "suspicion_threshold=0.75\n"
        "agent_success_rate=0.6\n"
        "prison_period=12\n"
        "num_ranks=8\n"
        "min_time_prepare=5\n"
        "max_time_prepare=12\n"
        "min_level_prepare=5\n"
        "max_level_prepare=12\n"
        "death_probability=0.4\n"
        "difficulty_level=0\n"
        "max_difficulty=10\n"
        "max_askers=20\n"
        "timeout_period=5\n"
        "min_prison_period=3\n"
        "max_prison_period=10\n"
        "knowledge_threshold=0.5";

    createTestConfigFile(content);

    // Load the config
    int result = load_config(test_config_path, &config);

    // Check return code
    EXPECT_EQ(result, 0);

    // Verify all values match expected values from the temporary config file
    EXPECT_EQ(config.max_thwarted_plans, 4);
    EXPECT_EQ(config.max_successful_plans, 5);
    EXPECT_EQ(config.max_executed_agents, 6);
    EXPECT_EQ(config.max_agents_per_gang, 4);
    EXPECT_EQ(config.min_gangs, 3);
    EXPECT_EQ(config.max_gangs, 12);
    EXPECT_EQ(config.min_gang_size, 4);
    EXPECT_EQ(config.max_gang_size, 15);
    EXPECT_FLOAT_EQ(config.suspicion_threshold, 0.75f);
    EXPECT_FLOAT_EQ(config.agent_success_rate, 0.6f);
    EXPECT_EQ(config.prison_period, 12);
    EXPECT_EQ(config.num_ranks, 8);
    EXPECT_EQ(config.min_time_prepare, 5);
    EXPECT_EQ(config.max_time_prepare, 12);
    EXPECT_EQ(config.min_level_prepare, 5);
    EXPECT_EQ(config.max_level_prepare, 12);
    EXPECT_FLOAT_EQ(config.death_probability, 0.4f);
    EXPECT_EQ(config.difficulty_level, 0);
    EXPECT_EQ(config.max_difficulty, 10);
    EXPECT_EQ(config.max_askers, 20);
    EXPECT_EQ(config.timeout_period, 5);
    EXPECT_EQ(config.min_prison_period, 3);
    EXPECT_EQ(config.max_prison_period, 10);
    EXPECT_FLOAT_EQ(config.knowledge_threshold, 0.5f);

}

// Test loading a non-existent config file
TEST_F(ConfigTest, LoadNonExistentFile) {
    int result = load_config("non_existent_config.txt", &config);
    EXPECT_EQ(result, -1);
}

// Test parameter validation with valid parameters
TEST_F(ConfigTest, ValidParameterCheck) {
    // Set up valid config values
    config.max_thwarted_plans = 3;
    config.max_successful_plans = 3;
    config.max_executed_agents = 5;
    config.max_agents_per_gang = 3;
    config.min_gangs = 2;
    config.max_gangs = 10;
    config.min_gang_size = 3;
    config.max_gang_size = 10;
    config.suspicion_threshold = 0.8f;
    config.agent_success_rate = 0.5f;
    config.prison_period = 10;
    config.num_ranks = 7;
    config.min_time_prepare = 4;
    config.max_time_prepare = 10;
    config.min_level_prepare = 4;
    config.max_level_prepare = 10;
    config.death_probability = 0.3f;
    config.difficulty_level = 0;
    config.max_difficulty = 10;
    config.timeout_period = 10;
    config.min_prison_period = 3;
    config.max_prison_period = 10;
    config.knowledge_threshold = 0.3;


    int result = check_parameter_correctness(&config);
    EXPECT_EQ(result, 0);
}

// Test parameter validation with invalid parameters
TEST_F(ConfigTest, InvalidParameterCheck) {
    // Test with negative values
    config.max_thwarted_plans = -3;
    config.max_successful_plans = 3;
    config.max_executed_agents = 5;
    // Other values don't matter as validation should fail early

    int result = check_parameter_correctness(&config);
    EXPECT_EQ(result, -1);

    // Test with min > max
    config.max_thwarted_plans = 3; // Fix the previous error
    config.min_gangs = 15;
    config.max_gangs = 10; // min > max, should fail

    result = check_parameter_correctness(&config);
    EXPECT_EQ(result, -1);
}

// Test serialization and deserialization
TEST_F(ConfigTest, SerializeDeserialize) {
    // Set up config with known values
    Config original;
    original.max_thwarted_plans = 41;
    original.max_successful_plans = 204;
    original.max_executed_agents = 53;
    original.max_agents_per_gang = 31;
    original.min_gangs = 23;
    original.max_gangs = 101;
    original.min_gang_size = 36;
    original.max_gang_size = 400;
    original.suspicion_threshold = 0.8f;
    original.agent_success_rate = 0.5f;
    original.prison_period = 10;
    original.num_ranks = 7;
    original.min_time_prepare = 4;
    original.max_time_prepare = 10;
    original.min_level_prepare = 4;
    original.max_level_prepare = 10;
    original.death_probability = 0.3f;
    original.difficulty_level = 1;
    original.max_difficulty = 11;
    original.timeout_period = 6;
    original.min_prison_period = 20;
    original.max_prison_period = 30;
    original.num_gangs = 5;
    original.max_askers = 4;
    original.knowledge_threshold = 0.7;



    // Serialize to buffer
    char buffer[256];
    serialize_config(&original, buffer);

    // Deserialize to new config
    Config deserialized;
    deserialize_config(buffer, &deserialized);

    // Verify all values match
    EXPECT_EQ(deserialized.max_thwarted_plans, original.max_thwarted_plans);
    EXPECT_EQ(deserialized.max_successful_plans, original.max_successful_plans);
    EXPECT_EQ(deserialized.max_executed_agents, original.max_executed_agents);
    EXPECT_EQ(deserialized.max_agents_per_gang, original.max_agents_per_gang);
    EXPECT_EQ(deserialized.min_gangs, original.min_gangs);
    EXPECT_EQ(deserialized.max_gangs, original.max_gangs);
    EXPECT_EQ(deserialized.min_gang_size, original.min_gang_size);
    EXPECT_EQ(deserialized.max_gang_size, original.max_gang_size);
    EXPECT_FLOAT_EQ(deserialized.suspicion_threshold, original.suspicion_threshold);
    EXPECT_FLOAT_EQ(deserialized.agent_success_rate, original.agent_success_rate);
    EXPECT_EQ(deserialized.prison_period, original.prison_period);
    EXPECT_EQ(deserialized.num_ranks, original.num_ranks);
    EXPECT_EQ(deserialized.min_time_prepare, original.min_time_prepare);
    EXPECT_EQ(deserialized.max_time_prepare, original.max_time_prepare);
    EXPECT_EQ(deserialized.min_level_prepare, original.min_level_prepare);
    EXPECT_EQ(deserialized.max_level_prepare, original.max_level_prepare);
    EXPECT_FLOAT_EQ(deserialized.death_probability, original.death_probability);
    EXPECT_EQ(deserialized.difficulty_level, original.difficulty_level);
    EXPECT_EQ(deserialized.max_difficulty, original.max_difficulty);
    EXPECT_EQ(deserialized.timeout_period, original.timeout_period);
    EXPECT_EQ(deserialized.min_prison_period, original.min_prison_period);
    EXPECT_EQ(deserialized.max_prison_period, original.max_prison_period);
    EXPECT_EQ(deserialized.num_gangs, original.num_gangs);
    EXPECT_EQ(deserialized.max_askers, original.max_askers);
    EXPECT_FLOAT_EQ(deserialized.knowledge_threshold, original.knowledge_threshold);
}

// Test handling of unknown keys in config file
TEST_F(ConfigTest, UnknownKeyInConfig) {
    // Create a test config file with an unknown key
    std::string content =
        "max_thwarted_plans=3\n"
        "unknown_key=42\n" // This should cause an error
        "max_successful_plans=3\n";
    createTestConfigFile(content);

    // Attempt to load the config
    int result = load_config(test_config_path, &config);

    // Should return error
    EXPECT_EQ(result, -1);
}

// Test loading with comments and empty lines
TEST_F(ConfigTest, CommentsAndEmptyLines) {
    // Create a test config with comments and empty lines
    std::string content =
        "# This is a comment\n"
        "\n"
        "max_thwarted_plans=3\n"
        "# Another comment\n"
        "\n"
        "max_successful_plans=3\n";
    createTestConfigFile(content);

    // Load the config
    int result = load_config(test_config_path, &config);

    // Should succeed
    EXPECT_EQ(result, -1);
    EXPECT_EQ(config.max_thwarted_plans, 3);
    EXPECT_EQ(config.max_successful_plans, 3);
}