//
// Created by - on 3/26/2025.
//

#include "game.h"
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include "config.h"
#include "unistd.h"

#include "random.h"


#define MAX_PATH 128
int game_init(Game *game, pid_t *processes, Config *cfg) {

    game->elapsed_time = 0;
    game->num_thwarted_plans = 0;
    game->num_successfull_plans = 0;
    game->num_executed_agents = 0;

    // Note: Gang pointers are now handled in shared_mem_utils.c through ShmPtrs

    char *binary_paths[] = {
        POLICE_EXECUTABLE,
        GANG_EXECUTABLE,
        GRAPHICS_EXECUTABLE
    };

    // police process (first in array)
    processes[0] = start_process(binary_paths[0], cfg, -1);
    
    // gang processes
    for(int i = 0; i < cfg->num_gangs; i++) {
        processes[i+1] = start_process(binary_paths[1], cfg, i);
    }

    // graphics process (at the end of the processes array)
    processes[1 + cfg->num_gangs] = start_process(binary_paths[2], cfg, -1);

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
        char config_buffer[MAX_PATH];
        char id_buffer[10];
        snprintf(id_buffer, sizeof(id_buffer), "%d", id);
        serialize_config(cfg, config_buffer);


        if (execl(binary, binary, config_buffer, id_buffer, NULL) == -1) {

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
        printf("GAME OVER: Maximum executed agents reached (%d/%d)\n",
               game->num_executed_agents, cfg->max_executed_agents);
        fflush(stdout);
        return 0;
    }
    if (game->num_successfull_plans >= cfg->max_successful_plans) {
        printf("GAME OVER: Maximum successful gang plans reached (%d/%d)\n",
               game->num_successfull_plans, cfg->max_successful_plans);
        fflush(stdout);
        return 0;
    }
    if (game->num_thwarted_plans >= cfg->max_thwarted_plans) {
        printf("GAME OVER: Maximum thwarted plans reached (%d/%d)\n",
               game->num_thwarted_plans, cfg->max_thwarted_plans);
        fflush(stdout);
        return 0;
    }
    return 1;
}