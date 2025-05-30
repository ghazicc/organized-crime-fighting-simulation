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
pid_t  *processes          = NULL;  // Dynamic allocation based on number of gangs
int    num_processes       = 0;
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

    printf("Number of gangs: %d\n", config.num_gangs);

    // Allocate memory for process IDs (1 police + num_gangs gang processes + 1 graphics)
    num_processes = 1 + config.num_gangs + 1;
    processes = malloc(num_processes * sizeof(pid_t));
    if (processes == NULL) {
        fprintf(stderr, "Failed to allocate memory for process array\n");
        return 1;
    }

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

    // Terminate all child processes (police + all gangs)
    for(int i = 0; i < num_processes; i++) {
        if (processes[i] > 0) {
            printf("Terminating process %d (PID: %d)\n", i, processes[i]);
            fflush(stdout);
            kill(processes[i], SIGINT);
        }
    }
    
    // Free the processes array
    if (processes != NULL) {
        free(processes);
        processes = NULL;
    }
    
    // cleanup_shared_memory(shared_game);
    cleanup_semaphores();
    
    // Unlink semaphores (only main process should do this)
    sem_unlink(GAME_STATS_SEM_NAME);
    sem_unlink(GANG_STATS_SEM_NAME);
    
    printf("Cleanup complete\n");
}
void handle_kill(int signum) {
    exit(0);
}
