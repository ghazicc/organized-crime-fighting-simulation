#include "semaphores_utils.h"
#include <fcntl.h>      /* O_CREAT, O_EXCL */
#include <stdio.h>
#include <stdlib.h>     /* For exit() and EXIT_FAILURE */
#include <errno.h>
#include <unistd.h>

static sem_t *game_stats_sem = NULL;
static sem_t *gang_stats_sem = NULL;

// Initialize semaphores for inter-process synchronization
int init_semaphores(void) {
    // Create/open semaphore for game statistics (shared across all processes)
    game_stats_sem = sem_open(GAME_STATS_SEM_NAME, O_CREAT, 0666, 1);
    if (game_stats_sem == SEM_FAILED) {
        perror("Failed to create game stats semaphore");
        return -1;
    }
    
    // Create/open semaphore for gang statistics (shared across gang processes)
    gang_stats_sem = sem_open(GANG_STATS_SEM_NAME, O_CREAT, 0666, 1);
    if (gang_stats_sem == SEM_FAILED) {
        perror("Failed to create gang stats semaphore");
        sem_close(game_stats_sem);
        return -1;
    }
    
    printf("Semaphores initialized successfully\n");
    fflush(stdout);
    return 0;
}

// Cleanup semaphores
void cleanup_semaphores(void) {
    if (game_stats_sem != NULL && game_stats_sem != SEM_FAILED) {
        sem_close(game_stats_sem);
        game_stats_sem = NULL;
    }
    
    if (gang_stats_sem != NULL && gang_stats_sem != SEM_FAILED) {
        sem_close(gang_stats_sem);
        gang_stats_sem = NULL;
    }
    
    printf("Semaphores cleaned up\n");
    fflush(stdout);
}

// Get game statistics semaphore
sem_t* get_game_stats_semaphore(void) {
    if (game_stats_sem == NULL || game_stats_sem == SEM_FAILED) {
        if (init_semaphores() != 0) {
            exit(EXIT_FAILURE);
        }
    }
    return game_stats_sem;
}

// Get gang statistics semaphore
sem_t* get_gang_stats_semaphore(void) {
    if (gang_stats_sem == NULL || gang_stats_sem == SEM_FAILED) {
        if (init_semaphores() != 0) {
            exit(EXIT_FAILURE);
        }
    }
    return gang_stats_sem;
}

