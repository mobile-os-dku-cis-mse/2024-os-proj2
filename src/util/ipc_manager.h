#ifndef IPC_MSG_H
#define IPC_MSG_H

#include <sys/ipc.h>
#include <sys/msg.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <stdint.h> // uint32_t 사용

// 메시지 구조체 정의
typedef struct Message {
    long type;            // 메시지 타입
    pid_t pid;            // 프로세스 ID
    int process_id;
    char value;
    int read_write;        // read : 0 / write : 1
    int status;
    uint32_t va;

} Message;

// 메시지 큐 생성 함수
int createMessageQueue();

// 메시지 전송 함수
void sendMessage(int msgid, Message *msg);

// 메시지 수신 함수
void receiveMessage(int msgid, Message *msg, int type);

#endif // IPC_MSG_H
