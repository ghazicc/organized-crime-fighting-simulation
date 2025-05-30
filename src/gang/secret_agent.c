#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <pthread.h>
#include "secret_agent.h"
#include "game.h"
#include "gang.h"
#include "message.h"
#include "random.h"

#define KNOWLEDGE_THRESHOLD 0.8f
#define REPORT_INTERVAL 5 // Report every 5 seconds

void* secret_agent_thread_function(void* arg) {
// ظprintf("Secret agent %d terminated\n", member->agent_id);
    return NULL;
}