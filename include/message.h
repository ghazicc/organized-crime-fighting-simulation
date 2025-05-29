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
long get_agent_msgtype(int MAX_AGENTS, int8_t gang_id, int8_t agent_id);
long get_gang_msgtype(int MAX_GANGS, int8_t gang_id);
long get_police_msgtype(int MAX_AGENTS, int NUM_GANGS, int8_t police_id);

#endif // POLICE_REPORT
