//
// Created by yazan on 5/2/2025.
//

#include "message.h"
#include <sys/ipc.h>
#include <sys/msg.h>


int create_message_queue() {
    int msgid = msgget(POLICE_GANG_KEY, IPC_CREAT | 0666);
    if (msgid == -1) {
        perror("msgget");
        return -1;
    }
    return msgid;
}

int send_message(int msgid, Message *message) {
    if (msgsnd(msgid, message, MESSAGE_SIZE, 0) == -1) {
        perror("msgsnd");
        return -1;
    }
    return 0;
}


