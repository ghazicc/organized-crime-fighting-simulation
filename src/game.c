//
// Created by - on 3/26/2025.
//

#include "game.h"
#include <fcntl.h>
#include <sys/stat.h>
#include "config.h"
#include "unistd.h"
#include <stdio.h>
#include <stdlib.h>


int game_init(Game *game, pid_t *processes, Config *cfg) {

    game->elapsed_time = 0;
    game->num_thwarted_plans = 0;
    game->num_successfull_plans = 0;
    game->num_executed_agents = 0;

    // Initialize pointers to dynamic parts
    game->gangs = (Gang*)((char*)game + sizeof(Game));
    game->total_gangs = 0;  // Updated as gangs spawn

    size_t gangs_size = cfg->max_gangs * sizeof(Gang);

    // Initialize each Gang's member array
    for (int i = 0; i < cfg->max_gangs; i++) {
        game->gangs[i].members = (Member*)((char*)game->gangs + gangs_size +
                                        i * cfg->max_gang_size * sizeof(Member));
    }

    char *binary_paths[] = {
        "./police",
        "./gang"
    };

    // police process
    processes[0] = start_process(binary_paths[0], cfg, 0);

    // gang processes
    for(int i = 0; i < cfg->max_gangs; i++) {
        processes[i+1] = start_process(binary_paths[1], cfg, i);
    }
    
    
    return 0;
}


pid_t start_process(const char *binary, Config *cfg, int id) {
    pid_t pid = fork();
    if (pid == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    }

    if (pid == 0) {
        // Now pass two arguments: shared memory fd and GUI pid.
        // Convert fd to string

        // serialize config to a string
        char config_buffer[sizeof(Config)];
        serialize_config(cfg, config_buffer);
    

        if (execl(binary, binary, id, config_buffer, NULL) == -1) {

            printf("%s\n", binary);
            perror("execl failed");
            exit(EXIT_FAILURE);
        }
    }
    return pid;
}


int check_game_conditions(const Game *game, const Config *cfg) {
    // Check if the game has reached its maximum limits

    if (game->num_executed_agents >= cfg->max_executed_agents) {
        return 0;
    }
    if (game->num_successfull_plans >= cfg->max_successful_plans) {
        return 0;
    }
    if (game->num_thwarted_plans >= cfg->max_thwarted_plans) {
        return 0;
    }
    return 1;
}