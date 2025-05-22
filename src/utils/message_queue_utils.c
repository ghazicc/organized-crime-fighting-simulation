//
// Created by yazan on 5/2/2025.
//

#include "message.h"
#include "stddef.h"
#include <sys/ipc.h>
#include <sys/msg.h>


int create_message_queue(int key) {
    int msgid = msgget(key, IPC_CREAT | 0666);
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

int receive_message(int msgid, Message *message, long mtype) {
    if (msgrcv(msgid, message, MESSAGE_SIZE, mtype, 0) == -1) {
        perror("msgrcv");
        return -1;
    }
    return 0;
}

int delete_message_queue(int msgid) {
    if (msgctl(msgid, IPC_RMID, NULL) == -1) {
        perror("msgctl");
        return -1;
    }
    return 0;
}



