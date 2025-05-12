#include <stdio.h>
#include <stdlib.h>
#include <unistd.h> // For sleep()
#include "actual_gang_member.h"
#include "gang.h" // For Member struct

void* actual_gang_member_thread_function(void* arg) {
    printf("Gang member thread started\n");
    fflush(stdout);
    Member *member = (Member*)arg;
    printf("Gang member %d in gang %d started\n", member->member_id, member->gang_id);
    fflush(stdout);
    
    // Add a slight delay to see if this thread runs
    sleep(2);
    printf("Gang member thread still running after delay\n");
    fflush(stdout);
    
    return NULL; // Return properly
}
