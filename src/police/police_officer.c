//
// Police officer behavior implementation - message queue communication only
//

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include "police.h"
#include "game.h"
#include "message.h"

extern PoliceForce police_force;

void monitor_gang_activity(PoliceOfficer* officer, ShmPtrs *shm_ptr) {
    // This function is now obsolete - all monitoring happens through message queues
    // Keeping for compatibility but functionality moved to communicate_with_agents()
    printf("Police Officer %d: Using message queue communication only\n", officer->police_id);
}

void arrest_gang_members(PoliceOfficer* officer, ShmPtrs *shm_ptrs) {
    // This function is replaced by imprison_gang() which handles gang-level arrests
    // Keeping for compatibility
    printf("Police Officer %d: Gang-level imprisonment handled by imprison_gang()\n", 
           officer->police_id);
    imprison_gang(officer);
}