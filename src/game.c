//
// Created by - on 3/26/2025.
//

#include "game.h"
#include "inventory.h"
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include "config.h"
#include "unistd.h"
#include <string.h>
#include <stdbool.h>
#include "shared_mem_utils.h"


int game_init(Game *game, pid_t *processes) {

    game->elapsed_time = 0;
    game->num_thwarted_plans = 0;
    game->num_successfull_plans = 0;
    game->num_executed_agents = 0;

    char *binary_paths[] = {
        "./police",
        "./gangs"
    };

    for (int i = 0; i < 5; i++) {
        bool suppress;
        suppress = true;
        if (strcmp(binary_paths[i], "./customer_manager") == 0)
            suppress = false;
        processes[i] = start_process(binary_paths[i], suppress);
    }
    
    return 0;
}


pid_t start_process(const char *binary, bool suppress) {
    pid_t pid = fork();
    if (pid == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    }

    if (pid == 0) {
        // Now pass two arguments: shared memory fd and GUI pid.
        // Convert fd to string

        if (suppress)
            freopen("/dev/null", "w", stdout);

        if (execl(binary, binary, NULL)) {

            printf("%s\n", binary);
            perror("execl failed");
            exit(EXIT_FAILURE);
        }
    }
    return pid;
}


int check_game_conditions(const Game *game) {

    if (game->num_executed_agents >= game->config.max_executed_agents) {
        return 0;
    }
    if (game->num_successfull_plans >= game->config.max_successful_plans) {
        return 0;
    }
    if (game->num_thwarted_plans >= game->config.max_thwarted_plans) {
        return 0;
    }
    return 1;
}