#ifndef BAKERY_MESSAGE_H
#define BAKERY_MESSAGE_H

// define message queue keys
#define POLICE_REPORT_KEY 0xCAFEBABE

typedef struct {
    long mtype; // Message type
    int target_type;
    float suspicion_level;
    int agent_id;
} PoliceReport;



#endif // BAKERY_MESSAGE_H
