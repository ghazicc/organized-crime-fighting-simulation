#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>
#include "assets.h"
#include "config.h"
#include "game.h"
#include "queue.h"
#include "shared_mem_utils.h"
#include "semaphores_utils.h"

/* globals from your original code --------------------------- */
Game  *shared_game         = NULL;
pid_t  processes[6];
pid_t *processes_sellers   = NULL;
int    shm_fd              = -1;
queue_shm *queue           = NULL;

/* ----------------------------------------------------------- */
void handle_alarm(int signum)
{
    shared_game->elapsed_time++;   /* original work            */
    alarm(1);                      /* re‑arm                   */
}

void cleanup_resources(void);
void handle_kill(int);

int main(int argc,char *argv[])
{
    printf("********** Bakery Simulation **********\n\n");
    fflush(stdout);

    reset_all_semaphores();

    atexit(cleanup_resources);

    shm_fd = setup_shared_memory(&shared_game);

    signal(SIGALRM,handle_alarm);
    signal(SIGINT ,handle_kill);

    if (load_config(CONFIG_PATH,&shared_game->config)==-1){
        printf("Config file failed\n"); return 1;
    }

    game_init(shared_game,processes,processes_sellers,shm_fd);

    alarm(1);               /* start 1‑second timer */

    /* empty polling loop – stays as before */
    while (check_game_conditions(shared_game)){ /* nothing */ }

    /* wait for graphics process (index 0 in your array) */
    int status_graphics;
    waitpid(processes[0], &status_graphics, 0);

    if (WIFEXITED(status_graphics))
        printf("Graphics child exited with code %d\n", WEXITSTATUS(status_graphics));
    else if (WIFSIGNALED(status_graphics))
        printf("Graphics child killed by signal %d\n", WTERMSIG(status_graphics));

    return 0;  /* cleanup_resources is run automatically */
}

/* ---- unchanged cleanup / signal handlers ------------------ */
void cleanup_resources()
{
    printf("Cleaning up resources...\n"); fflush(stdout);

    for(int i=0;i<6;i++) kill(processes[i],SIGINT);
    cleanup_shared_memory(shared_game);
    shm_unlink(CUSTOMER_QUEUE_SHM_NAME);
    free(processes_sellers);
    printf("Cleanup complete\n");
}
void handle_kill(int signum){ exit(0); }
