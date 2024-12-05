// queue.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/time.h>
#include <fcntl.h>
#include "msg2.h"

// 큐 초기화 함수
void initQueue(Queue* q) {
    q->front = 0;
    q->rear = -1;
    q->size = 0;
}

// 큐가 가득 찼는지 확인
int isFull(Queue* q) {
    return q->size == QUEUE_SIZE;
}

// 큐가 비었는지 확인
int isEmpty(Queue* q) {
    return q->size == 0;
}

// 큐에 프로세스 추가
void enqueue(Queue* q, Process* process) {
    if (isFull(q)) {
        printf("[Queue] Error: Queue is full\n");
        return;
    }
    q->rear = (q->rear + 1) % QUEUE_SIZE;
    q->processes[q->rear] = *process;
    q->size++;
}

// 큐에서 프로세스 제거
Process* dequeue(Queue* q) {
    if (isEmpty(q)) {
        printf("[Queue] Error: Queue is empty\n");
        return NULL;
    }
    Process* process = &q->processes[q->front];
    q->front = (q->front + 1) % QUEUE_SIZE;
    q->size--;
    return process;
}

// 특정 인덱스의 프로세스 가져오기
Process* getProcessAtIndex(Queue* q, int index) {
    if (isEmpty(q) || index < 0 || index >= q->size) {
        return NULL;
    }
    int actualIndex = (q->front + index) % QUEUE_SIZE;
    return &q->processes[actualIndex];
}

// 큐 상태 출력
void print_qstate(int rtime, Queue* runq, Queue* waitq) {
    printf("\n┌─────────────────────────────────────────────────────────── System Status at Time %5d ───────────────────────────────────────────────────────────┐\n", rtime);

    // Run Queue 상태 출력
    printf("  RUN QUEUE  │ ");
    if (isEmpty(runq)) {
        printf("Empty");
    } else {
        for (int i = 0; i < runq->size; i++) {
            Process* rproc = getProcessAtIndex(runq, i);
            printf("P%d[CPU:%2d]", rproc->idx, rproc->remain_cpu_burst);
            if (i < runq->size - 1) printf(" → ");
        }
    }
    printf("\n");

    // Wait Queue 상태 출력
    printf("  WAIT QUEUE │ ");
    if (isEmpty(waitq)) {
        printf("Empty");
    } else {
        for (int i = 0; i < waitq->size; i++) {
            Process* wproc = getProcessAtIndex(waitq, i);
            printf("P%d[I/O:%2d]", wproc->idx, wproc->remain_io_burst);
            if (i < waitq->size - 1) printf(" → ");
        }
    }
    printf("\n");
    printf("└───────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────┘\n");
}

// 로그 파일에 상태 기록
void writeToFile(FILE *file, int rtime, int count, Queue* runq, Queue* waitq) {
    if (file == NULL) {
        return;
    }

    fprintf(file, "\n====== Time: %3d (Quantum Count: %d) ======\n", rtime, count);
    
    if (!isEmpty(runq)) {
        Process* current = &runq->processes[runq->front];
        fprintf(file, "Current Process: P%d\n", current->idx);
        fprintf(file, "├── CPU Burst Remaining: %d\n", current->remain_cpu_burst);
        fprintf(file, "└── I/O Burst Total: %d\n", current->io_burst);
    } else {
        fprintf(file, "No process currently running\n");
    }

    fprintf(file, "\nRun Queue Status:\n");
    if (isEmpty(runq)) {
        fprintf(file, "└── Empty\n");
    } else {
        for (int i = 0; i < runq->size; i++) {
            Process* proc = getProcessAtIndex(runq, i);
            if (i == runq->size - 1) {
                fprintf(file, "└── P%d (CPU: %d)\n", proc->idx, proc->remain_cpu_burst);
            } else {
                fprintf(file, "├── P%d (CPU: %d)\n", proc->idx, proc->remain_cpu_burst);
            }
        }
    }

    fprintf(file, "\nWait Queue Status:\n");
    if (isEmpty(waitq)) {
        fprintf(file, "└── Empty\n");
    } else {
        for (int i = 0; i < waitq->size; i++) {
            Process* proc = getProcessAtIndex(waitq, i);
            if (i == waitq->size - 1) {
                fprintf(file, "└── P%d (I/O: %d)\n", proc->idx, proc->remain_io_burst);
            } else {
                fprintf(file, "├── P%d (I/O: %d)\n", proc->idx, proc->remain_io_burst);
            }
        }
    }
    fprintf(file, "\n");
}