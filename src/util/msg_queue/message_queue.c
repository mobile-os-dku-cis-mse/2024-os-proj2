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
    // 기존 메시지 큐가 있다면 삭제
    msgctl(msgget(MSG_KEY_SCHED, 0666), IPC_RMID, NULL);
    msgctl(msgget(MSG_KEY_MEM, 0666), IPC_RMID, NULL);

    // 새로운 메시지 큐 생성
    msg_queue_id_sched = msgget(MSG_KEY_SCHED, IPC_CREAT | 0666);
    if(msg_queue_id_sched == -1) {
        perror("msgget");
        return -1;
    }

    msg_queue_id_page = msgget(MSG_KEY_MEM, IPC_CREAT | 0666);
    if (msg_queue_id_page == -1){
        perror("msgget");
        // 첫 번째 큐도 삭제
        msgctl(msg_queue_id_sched, IPC_RMID, NULL);
        return -1;
    }


    // 큐가 비어있는지 확인하기 위한 디버그 출력
    struct msqid_ds buf;
    if (msgctl(msg_queue_id_sched, IPC_STAT, &buf) != -1) {
        printf("[Init] Scheduler queue - messages: %lu\n", buf.msg_qnum);
    }
    if (msgctl(msg_queue_id_page, IPC_STAT, &buf) != -1) {
        printf("[Init] Page queue - messages: %lu\n", buf.msg_qnum);
    }

    return 0;
}

int get_msg_queue_id(){
    // 새로운 메시지 큐 생성
    msg_queue_id_sched = msgget(MSG_KEY_SCHED, IPC_CREAT | 0666);
    if(msg_queue_id_sched == -1) {
        perror("msgget");
        return -1;
    }

    msg_queue_id_page = msgget(MSG_KEY_MEM, IPC_CREAT | 0666);
    if (msg_queue_id_page == -1){
        perror("msgget");
        // 첫 번째 큐도 삭제
        msgctl(msg_queue_id_sched, IPC_RMID, NULL);
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
