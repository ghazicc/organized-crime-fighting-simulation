#include "shared_mem_utils.h"
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <stdlib.h>
#include <stdio.h>
#include "gang.h"


// Owner function - creates, truncates, and maps shared memory
Game* setup_shared_memory_owner(Config *cfg) {
    printf("OWNER: Setting up shared memory...\n");
    fflush(stdout);
    
    // Allocate shared memory for Game + dynamic arrays
    size_t game_size = sizeof(Game);
    size_t gangs_size = cfg->max_gangs * sizeof(Gang);
    size_t members_size = cfg->max_gangs * cfg->max_gang_size * sizeof(Member);
    size_t total_size = game_size + gangs_size + members_size;
    
    printf("OWNER: Memory sizes - game: %zu, gangs: %zu, members: %zu, total: %zu\n",
           game_size, gangs_size, members_size, total_size);
    
    // Create new shared memory segment with O_CREAT flag
    int shm_fd = shm_open(GAME_SHM_NAME, O_CREAT | O_RDWR, 0666);
    if (shm_fd == -1) {
        perror("OWNER: shm_open failed");
        exit(EXIT_FAILURE);
    }
    
    // Truncate to set the size
    if (ftruncate(shm_fd, total_size) == -1) {
        perror("OWNER: ftruncate failed");
        close(shm_fd);
        exit(EXIT_FAILURE);
    }
    
    // Map the shared memory
    Game *game = mmap(NULL, total_size, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (game == MAP_FAILED) {
        perror("OWNER: mmap failed");
        close(shm_fd);
        exit(EXIT_FAILURE);
    }
    
    // Close file descriptor as it's no longer needed after mapping
    close(shm_fd);
    
    printf("OWNER: Shared memory created and mapped successfully at %p\n", (void*)game);
    fflush(stdout);
    
    return game;
}

// User function - only maps to existing shared memory
Game* setup_shared_memory_user(Config *cfg) {
    printf("USER: Connecting to shared memory...\n");
    fflush(stdout);
    
    // Calculate sizes
    size_t game_size = sizeof(Game);
    size_t gangs_size = cfg->max_gangs * sizeof(Gang);
    size_t members_size = cfg->max_gangs * cfg->max_gang_size * sizeof(Member);
    size_t total_size = game_size + gangs_size + members_size;
    
    // Open existing shared memory without O_CREAT flag
    int shm_fd = shm_open(GAME_SHM_NAME, O_RDWR, 0666);
    if (shm_fd == -1) {
        perror("USER: shm_open failed");
        exit(EXIT_FAILURE);
    }
    
    // Map the memory
    Game *game = mmap(NULL, total_size, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (game == MAP_FAILED) {
        perror("USER: mmap failed");
        close(shm_fd);
        exit(EXIT_FAILURE);
    }
    
    // Close file descriptor
    close(shm_fd);
    
    printf("USER: Successfully connected to shared memory at %p\n", (void*)game);
    fflush(stdout);
    
    return game;
}

// Legacy function - to be removed once migration is complete
Game* setup_shared_memory(Config *cfg) {
    printf("WARNING: Using deprecated function setup_shared_memory\n");
    fflush(stdout);
    
    // Allocate shared memory for Game + dynamic arrays
    size_t game_size = sizeof(Game);
    size_t gangs_size = cfg->max_gangs * sizeof(Gang);
    size_t members_size = cfg->max_gangs * cfg->max_gang_size * sizeof(Member);
    size_t total_size = game_size + gangs_size + members_size;

    int shm_fd = shm_open(GAME_SHM_NAME, O_CREAT | O_RDWR, 0666);
    if (shm_fd == -1) {
        perror("shm_open");
        exit(EXIT_FAILURE);
    }
    
    // WARNING: This is what causes the issue - we're truncating every time
    ftruncate(shm_fd, total_size);

    Game *game = mmap(NULL, total_size, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (game == MAP_FAILED) {
        perror("mmap");
        close(shm_fd);
        exit(EXIT_FAILURE);
    }
    
    close(shm_fd);
    return game;
}


void cleanup_shared_memory(Game *shared_game) {

}


