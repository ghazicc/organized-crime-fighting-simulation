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

    // Target definitions
    Target targets[NUM_TARGETS];

} Game;


typedef struct {
    Game *shared_game;
    Gang *gangs; // Pointer to gangs array, dynamically allocated after Game
    Member **gang_members; // Array of pointers to each gang's member array
} ShmPtrs;

// Still can keep these (but optional now)
pid_t start_process(const char *binary, Config *cfg, int id);
int game_init(Game *game, pid_t *processes, Config *cfg);
void game_destroy(int shm_fd, Game *shared_game);
void game_create(int *shm_fd, Game *shared_game);
int check_game_conditions(const Game *game, const Config *cfg);
void print_with_time1(const Game *game, const char *format, ...);

#endif // GAME_H
