#ifndef SEMAPHORES_UTILS_H
#define SEMAPHORES_UTILS_H

/* A tiny convenience wrapper around POSIX named semaphores.
 * All functions return NULL / -1 on error and print perror-style diagnostics.
 */
#include <semaphore.h>

/* Create a brand-new named semaphore (O_CREAT | O_EXCL).
 * 'value' is the initial counter.
 */
sem_t *sem_create(const char *name, unsigned value);

/* Open an existing named semaphore for read/write. */
sem_t *sem_open_existing(const char *name);

/* Utility wrapper that retries sem_wait() if it was interrupted by a signal
 * (EINTR).  Returns 0 on success, -1 on real error.
 */
int sem_wait_intr(sem_t *sem);

/* Close the descriptor (does NOT remove the semaphore). */
void sem_close_safe(sem_t *sem);

/* Remove the semaphore from the system (use once, typically by the owner). */
void sem_unlink_safe(const char *name);

#endif /* SEMAPHORES_UTILS_H */
