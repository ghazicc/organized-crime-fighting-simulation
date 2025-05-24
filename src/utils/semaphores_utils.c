#include "semaphores_utils.h"
#include <fcntl.h>      /* O_CREAT, O_EXCL */
#include <stdio.h>
#include <errno.h>
#include <time.h>
#include <unistd.h>

sem_t *sem_create(const char *name, unsigned value)
{
    sem_t *s = sem_open(name, O_CREAT | O_EXCL, 0666, value);
    if (s == SEM_FAILED) {
        perror("sem_create: sem_open");
        return NULL;
    }
    return s;
}

sem_t *sem_open_existing(const char *name)
{
    sem_t *s = sem_open(name, 0);
    if (s == SEM_FAILED) {
        perror("sem_open_existing: sem_open");
        return NULL;
    }
    return s;
}

int sem_wait_intr(sem_t *sem)
{
    int rv;
    while ((rv = sem_wait(sem)) == -1 && errno == EINTR) {
        /* interrupted → retry */
    }
    if (rv == -1) perror("sem_wait");
    return rv;
}

void sem_close_safe(sem_t *sem)
{
    if (!sem) return;
    if (sem_close(sem) == -1) perror("sem_close");
}

void sem_unlink_safe(const char *name)
{
    if (sem_unlink(name) == -1 && errno != ENOENT)
        perror("sem_unlink");
}
