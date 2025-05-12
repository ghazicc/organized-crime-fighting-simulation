//
// Created by yazan on 5/12/2025.
//

#ifndef POLICE_H
#define POLICE_H
#define MAX_GANGS 10
#include <pthread.h>

typedef struct {
    int jailed_gangs[MAX_GANGS];
    pthread_t monitor_thread;
    pthread_t action_thread;
} Police;
#endif //POLICE_H
