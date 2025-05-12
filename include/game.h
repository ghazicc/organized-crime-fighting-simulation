#ifndef GAME_H
#define GAME_H

#include <stdio.h>
#include <stdarg.h>

#include "config.h"
#include "inventory.h"
#include "oven.h"
#include <stdbool.h>
#include "info.h"

#define MAX_OVENS 10  // Max ovens allowed

typedef struct Game {

    int num_thwarted_plans;
    int num_successfull_plans;
    int num_executed_agents;
    int elapsed_time;

    Config config;
} Game;

// Still can keep these (but optional now)
pid_t start_process(const char *binary, bool suppress);
int game_init(Game *game, pid_t *processes, pid_t *processes_sellers, int shared_mem_fd);
void game_destroy(int shm_fd, Game *shared_game);
void game_create(int *shm_fd, Game **shared_game);
int check_game_conditions(const Game *game);
void print_with_time1(const Game *game, const char *format, ...);

#endif // GAME_H
