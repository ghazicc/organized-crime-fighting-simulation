#include "shared_mem_utils.h"
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>
#include "gang.h"
#include "random.h"


// Owner function - creates, truncates, and maps shared memory
Game* setup_shared_memory_owner(Config *cfg, ShmPtrs *shm_ptrs) {
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
    
    // CRITICAL: Zero out all shared memory to ensure clean initialization
    memset(game, 0, total_size);
    printf("OWNER: Zeroed out %zu bytes of shared memory\n", total_size);
    fflush(stdout);
    
    // Initialize Game struct fields
    game->num_successfull_plans = 0;
    game->num_thwarted_plans = 0;
    game->num_executed_agents = 0;
    game->elapsed_time = 0;
    printf("OWNER: Initialized Game struct counters to 0\n");
    fflush(stdout);
    
    // Set up pointers to dynamic parts
    shm_ptrs->gangs = (Gang*)((char*)game + sizeof(Game));
    
    // Allocate array for gang member pointers
    shm_ptrs->gang_members = malloc(cfg->num_gangs * sizeof(Member*));
    if (shm_ptrs->gang_members == NULL) {
        fprintf(stderr, "OWNER: Failed to allocate gang_members array\n");
        exit(EXIT_FAILURE);
    }
    
    // Set up gang member pointers
    for (int i = 0; i < cfg->num_gangs; i++) {
        // Initialize gang fields (memset already zeroed everything, but be explicit)
        shm_ptrs->gangs[i].gang_id = i; 
        shm_ptrs->gangs[i].max_member_count = random_int(cfg->min_gang_size, cfg->max_gang_size);
        shm_ptrs->gangs[i].num_alive_members = shm_ptrs->gangs[i].max_member_count;
        shm_ptrs->gangs[i].num_successful_plans = 0;  // Explicitly set to 0
        shm_ptrs->gangs[i].num_thwarted_plans = 0;    // Explicitly set to 0
        
        // Set up local pointer to this gang's members
        shm_ptrs->gang_members[i] = (Member*)((char*)shm_ptrs->gangs +
                                       gangs_size + 
                                       i * cfg->max_gang_size * sizeof(Member));
        
        printf("OWNER: Gang %d: %d members, success=%d, thwarted=%d, at offset %ld\n", 
               i, shm_ptrs->gangs[i].max_member_count,
               shm_ptrs->gangs[i].num_successful_plans,
               shm_ptrs->gangs[i].num_thwarted_plans,
               (char*)shm_ptrs->gang_members[i] - (char*)game);
    }

    for (int i = 0; i < cfg->num_gangs; i++) {

    }
    
    printf("OWNER: Shared memory layout initialized\n");
    fflush(stdout);
    
    return game;
}

// User function - only maps to existing shared memory
Game* setup_shared_memory_user(Config *cfg, ShmPtrs *shm_ptrs) {
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

    shm_ptrs->shared_game = game;
    // Close file descriptor
    close(shm_fd);
    
    printf("USER: Successfully connected to shared memory at %p\n", (void*)game);
    fflush(stdout);
    
    // CRITICAL: We must recreate all the pointers in the shared memory structure
    // because pointers from one process's address space don't make sense in another process
    
    printf("USER: Setting up internal pointers in shared memory\n");
    fflush(stdout);
    
    // Set up gangs pointer
    shm_ptrs->gangs = (Gang*)((char*)game + sizeof(Game));
    printf("USER: Gangs array at offset %ld, address %p\n", 
           (char*)shm_ptrs->gangs - (char*)game, (void*)shm_ptrs->gangs);
    fflush(stdout);
    
    // Allocate array for gang member pointers
    shm_ptrs->gang_members = malloc(cfg->num_gangs * sizeof(Member*));
    if (shm_ptrs->gang_members == NULL) {
        fprintf(stderr, "USER: Failed to allocate gang_members array\n");
        exit(EXIT_FAILURE);
    }
    
    // Set up member pointers for each gang
    for (int i = 0; i < cfg->num_gangs; i++) {
        shm_ptrs->gang_members[i] = (Member*)((char*)shm_ptrs->gangs + 
                                      gangs_size + 
                                      i * cfg->max_gang_size * sizeof(Member));
        printf("USER: Gang %d members at offset %ld, address %p\n",
               i, (char*)shm_ptrs->gang_members[i] - (char*)game, (void*)shm_ptrs->gang_members[i]);
        fflush(stdout);
    }
    
    printf("USER: All internal pointers initialized\n");
    fflush(stdout);
    
    return game;
}

void cleanup_shared_memory(Game *shared_game) {
    if (shared_game != NULL && shared_game != MAP_FAILED) {
        if (munmap(shared_game, sizeof(Game)) == -1) {
            perror("munmap failed");
        }
    }
    shm_unlink(GAME_SHM_NAME);
}


