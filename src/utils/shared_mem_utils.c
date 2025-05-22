#include "shared_mem_utils.h"
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>
#include "gang.h"
#include "random.h"


// Owner function - creates, truncates, and maps shared memory
Game* setup_shared_memory_owner(Config *cfg) {
    printf("OWNER: Setting up shared memory...\n");
    fflush(stdout);
    
    // Allocate shared memory for Game + dynamic arrays
    size_t game_size = sizeof(Game);
    size_t gangs_size = cfg->num_gangs * sizeof(Gang);
    size_t members_size = cfg->num_gangs * cfg->max_gang_size * sizeof(Member);
    size_t total_size = game_size + gangs_size + members_size;
    
    printf("OWNER: Game struct layout: Game size: %zu, Gang: %zu, Member: %zu\n", 
           sizeof(Game), sizeof(Gang), sizeof(Member));
    fflush(stdout);
    
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
    
    // Initialize the memory layout
    printf("OWNER: Initializing memory layout\n");
    fflush(stdout);
    
    // Set up pointers to dynamic parts
    game->gangs = (Gang*)((char*)game + sizeof(Game));
    
    // Set up gang member pointers
    for (int i = 0; i < cfg->num_gangs; i++) {
        game->gangs[i].gang_id = i; // Pre-initialize gang IDs
        game->gangs[i].max_member_count = (int) random_float(cfg->min_gang_size, cfg->max_gang_size);
        game->gangs[i].members = (Member*)((char*)game->gangs +
                                       gangs_size + 
                                       i * cfg->max_gang_size * sizeof(Member));
    }
    
    printf("OWNER: Shared memory layout initialized\n");
    fflush(stdout);
    
    return game;
}

// User function - only maps to existing shared memory
Game* setup_shared_memory_user(Config *cfg) {
    printf("USER: Connecting to shared memory...\n");
    fflush(stdout);
    
    // Calculate sizes
    size_t game_size = sizeof(Game);
    size_t gangs_size = cfg->num_gangs * sizeof(Gang);
    size_t members_size = cfg->num_gangs * cfg->max_gang_size * sizeof(Member);
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
    
    // CRITICAL: We must recreate all the pointers in the shared memory structure
    // because pointers from one process's address space don't make sense in another process
    
    printf("USER: Setting up internal pointers in shared memory\n");
    fflush(stdout);
    
    // Set up gangs pointer
    game->gangs = (Gang*)((char*)game + sizeof(Game));
    printf("USER: Gangs array at offset %ld, address %p\n", 
           (char*)game->gangs - (char*)game, (void*)game->gangs);
    fflush(stdout);
    
    // Set up member pointers for each gang
    for (int i = 0; i < cfg->num_gangs; i++) {
        game->gangs[i].members = (Member*)((char*)game->gangs + 
                                      gangs_size + 
                                      i * cfg->max_gang_size * sizeof(Member));
        printf("USER: Gang %d members at offset %ld, address %p\n",
               i, (char*)game->gangs[i].members - (char*)game, (void*)game->gangs[i].members);
        fflush(stdout);
    }
    
    printf("USER: All internal pointers initialized\n");
    fflush(stdout);
    
    return game;
}

void cleanup_shared_memory(Game *shared_game) {

}


