#include "ipc_manager.h"
#include <unistd.h>

// 새로운 메시지 큐를 생성하는 함수
// 메시지 큐는 IPC_PRIVATE를 사용하여 고유한 큐를 생성
int createMessageQueue()
{
    // 키 생성
    key_t key = ftok("msgqueue", 65);
    if (key == -1) {
        perror("ftok failed");
        exit(EXIT_FAILURE);
    }

    // 메시지 큐 생성
    int msgid = msgget(key, 0666 | IPC_CREAT);
    if (msgid == -1) {
        perror("msgget failed");
        exit(EXIT_FAILURE);
    }

    return msgid;
}


void sendMessage(int msgid, Message *msg) {
    if (msgsnd(msgid, msg, sizeof(Message) - sizeof(long), 0) != -1) {
        printf("Message sent with PID, msgid: %d // %d, process_id: %d, read_write: %d va : %d\n",
                msg->pid, msgid, msg->process_id, msg->process_id, msg->va);
    } else {
        perror("Message send failed");
        printf("ERROR ERROR ERROR ERROR ERROR ERROR ERROR ERROR ERROR ERROR\n");
        exit(EXIT_FAILURE);
    }
}


// 메시지 수신
void receiveMessage(int msgid, Message *msg, int type) {
    if (msgrcv(msgid, msg, sizeof(Message) - sizeof(long), type, 0) == -1) {
        perror("Message receive failed");
        exit(EXIT_FAILURE);
    }
}
