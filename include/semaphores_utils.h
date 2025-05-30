#ifndef SEMAPHORES_UTILS_H
#define SEMAPHORES_UTILS_H
#include <semaphore.h>

// Semaphore names for shared memory synchronization
#define GAME_STATS_SEM_NAME "/game_stats_sem"
#define GANG_STATS_SEM_NAME "/gang_stats_sem" 

// Function declarations
int init_semaphores(void);
void cleanup_semaphores(void);
sem_t* get_game_stats_semaphore(void);
sem_t* get_gang_stats_semaphore(void);

// Helper macros for semaphore operations
#define LOCK_GAME_STATS() sem_wait(get_game_stats_semaphore())
#define UNLOCK_GAME_STATS() sem_post(get_game_stats_semaphore())
#define LOCK_GANG_STATS() sem_wait(get_gang_stats_semaphore())
#define UNLOCK_GANG_STATS() sem_post(get_gang_stats_semaphore())

#endif // SEMAPHORES_UTILS_H