#ifndef GAME_H
#define GAME_H

#include <fcntl.h>
#include "config.h"
#include "gang.h"


typedef struct Game {

    int num_thwarted_plans;
    int num_successfull_plans;
    int num_executed_agents;
    int elapsed_time;

    Gang *gangs; // Pointer to dynamic array of gangs
    int total_gangs; // Number of gangs currently in the game

} Game;

// Still can keep these (but optional now)
pid_t start_process(const char *binary);
int game_init(Game *game, pid_t *processes);
void game_destroy(int shm_fd, Game *shared_game);
void game_create(int *shm_fd, Game **shared_game);
int check_game_conditions(const Game *game);
void print_with_time1(const Game *game, const char *format, ...);

#endif // GAME_H
