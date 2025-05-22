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

int create_message_queue(int key);
int send_message(int msgid, Message *message);
int receive_message(int msgid, Message *message, long mtype);
int delete_message_queue(int msgid);

#endif // BAKERY_MESSAGE_H
