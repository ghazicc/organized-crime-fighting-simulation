#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "config.h"
#include "json/json-config.h"
#include "game.h"
#include "shared_mem_utils.h"
#include "semaphores_utils.h"
#include "random.h"


/* globals from your original code --------------------------- */
Game  *shared_game         = NULL;
ShmPtrs shm_ptrs;
pid_t  processes[2];
Config config;

/* ----------------------------------------------------------- */
void handle_alarm(int signum)
{
    shared_game->elapsed_time++;   /* original work            */
    alarm(1);                      /* re‑arm                   */
}

void cleanup_resources(void);
void handle_kill(int);

int main(int argc,char *argv[]) {
    printf("********** Bakery Simulation **********\n\n");
    fflush(stdout);

    // reset_all_semaphores();

    atexit(cleanup_resources);
    
    // Initialize semaphores for inter-process synchronization
    if (init_semaphores() != 0) {
        fprintf(stderr, "Failed to initialize semaphores\n");
        return 1;
    }
    
    // Initialize random number generator
    init_random();
    
    // Load config first
    if (load_config(CONFIG_PATH, &config) == -1) {
        printf("Config file failed\n"); 
        return 1;
    }

    


    // randomize the number of gangs based on the user-defined range

    config.num_gangs = (int) random_float(config.min_gangs, config.max_gangs); 

    printf("Number of gangs: %d\n", config.num_gangs);

    // Main process is the owner of shared memory
    shared_game = setup_shared_memory_owner(&config, &shm_ptrs);
    shm_ptrs.shared_game = shared_game;
    
    // Load targets AFTER initializing shared memory
    if (load_targets_from_json(JSON_PATH, shared_game->targets) == -1) {
        printf("Json file failed");
        return 1;
    }

    signal(SIGALRM,handle_alarm);
    signal(SIGINT ,handle_kill);

    game_init(shared_game, processes, &config);
    alarm(1);               /* start 1‑second timer */

    /* empty polling loop – stays as before */
    while (check_game_conditions(shared_game, &config)){ /* nothing */ }

    return 0;  /* cleanup_resources is run automatically */
}

/* ---- unchanged cleanup / signal handlers ------------------ */
void cleanup_resources() {
    printf("Cleaning up resources...\n"); fflush(stdout);

    for(int i=0;i<2;i++) kill(processes[i],SIGINT);
    cleanup_shared_memory(shared_game);
    cleanup_semaphores();
    
    // Unlink semaphores (only main process should do this)
    sem_unlink(GAME_STATS_SEM_NAME);
    sem_unlink(GANG_STATS_SEM_NAME);
    
    printf("Cleanup complete\n");
}
void handle_kill(int signum) {
    exit(0);
}
