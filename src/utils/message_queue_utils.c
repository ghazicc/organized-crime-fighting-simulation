//
// Created by yazan on 5/2/2025.
//

#include "message.h"
#include "stddef.h"
#include <sys/ipc.h>
#include <sys/msg.h>
#include <stdio.h>


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

long get_agent_msgtype(const int MAX_AGENTS, const int8_t gang_id, const int8_t agent_id) {
    return (MAX_AGENTS + 1) * gang_id + agent_id + 1;
}

long get_gang_msgtype(const int MAX_AGENTS, const int8_t gang_id) {
    return MAX_AGENTS * gang_id + MAX_AGENTS + 1;
}

long get_police_msgtype(const int MAX_AGENTS, const int NUM_GANGS, const int8_t police_id) {
    return (MAX_AGENTS + 1) * NUM_GANGS + police_id + 1;
}

