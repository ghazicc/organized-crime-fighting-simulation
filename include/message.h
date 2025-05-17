#ifndef POLICE_REPORT
#define POLICE_REPORT

// define message queue keys
#define POLICE_REPORT_KEY 0xCAFEBABE

typedef struct {
    long mtype; // Message type
    int target_type;
    float suspicion_level;
    int agent_id;
} PoliceReport;


typedef struct {
    long mtype; // Message type
    int target_type;
    int agent_id;
} PoliceAction;





#endif // BAKERY_MESSAGE_H
