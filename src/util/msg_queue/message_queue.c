//
// Created by hochacha on 24. 11. 7.
//

#include "message_queue.h"

#include <stdio.h>
#include <stdlib.h>
#include <sys/msg.h>

int msg_queue_id_sched;
int msg_queue_id_page;
#define MSG_KEY_SCHED 12345
#define MSG_KEY_MEM 54321

int init_msg_queue() {
    msg_queue_id_sched = msgget(MSG_KEY_SCHED, IPC_CREAT | 0666);
    if(msg_queue_id_sched == -1) {
        perror("msgget");
        return -1;
    }

    msg_queue_id_page = msgget(MSG_KEY_MEM, IPC_CREAT | 0666);
    if (msg_queue_id_page == -1){
        perror("msgget");
        return -1;
    }
    return 0;
}

void close_msg_queue() {
    if(msgctl(msg_queue_id_sched, IPC_RMID, 0) == -1) {
        perror("msgctl");
    }
    if(msgctl(msg_queue_id_page, IPC_RMID, 0) == -1) {
        perror("msgctl");
    }
}
