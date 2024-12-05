#ifndef MSG2_H
#define MSG2_H

#include <stdint.h>
#include "vmm2.h"

#define QUEUE_SIZE 10
#define FRAME_SIZE 4096      // 4KB
//#define NUM_FRAMES 16        // vmm2.h 파일에 존재
#define MAX_VIRT_ADDR 0xFFFFFFFF  // 32비트 주소 공간
#define MAX_CPROC 10

// 메시지 버퍼 구조체 (프로세스 생성 시 사용)
typedef struct msgbuf {
    long mtype;       
    int pid;
    int idx;
    int io_burst;
    int cpu_burst;
} msgbuf;

// 메모리 접근 요청을 위한 메시지 구조체
typedef struct mem_msgbuf {
    long mtype;
    int pid;
    int process_index;
    uint32_t virtual_addresses[10];  // 32비트 주소
} mem_msgbuf;

// 프로세스 구조체
typedef struct Process {
    pid_t pid;
    int idx;
    int cpu_burst;
    int io_burst;
    int remain_cpu_burst;
    int remain_io_burst;
} Process;

// 큐 구조체
typedef struct Queue {
    Process processes[QUEUE_SIZE];
    int front;
    int rear;
    int size;
} Queue;

// 함수 선언
void initQueue(Queue* q);
int isFull(Queue* q);
int isEmpty(Queue* q);
void enqueue(Queue* q, Process* process);
Process* dequeue(Queue* q);
Process* getProcessAtIndex(Queue* q, int index);
void print_qstate(int rtime, Queue* runq, Queue* waitq);

#endif