#ifndef POLICE_REPORT
#define POLICE_REPORT

#include <stdint.h>  // For uint8_t type

// define message queue keys
#define POLICE_GANG_KEY 0x1234
#define MESSAGE_SIZE sizeof(Message) - sizeof(long)

typedef struct {
    long mtype;
    uint8_t mode;
    union {
        float knowledge;
        int agent_id;
    } MessageContent;
} Message;
#endif // BAKERY_MESSAGE_H
